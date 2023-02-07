// dsorenderer.cpp
//
// Copyright (C) 2001-2020, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celengine/dsodb.h>
#include <celengine/deepskyobj.h>
#include <celrender/galaxyrenderer.h>
#include <celrender/globularrenderer.h>
#include <celrender/nebularenderer.h>
#include <celrender/openclusterrenderer.h>
#include "render.h"
#include "dsorenderer.h"


namespace
{
// The parameter 'enhance' adjusts the DSO brightness as viewed from "inside"
// (e.g. MilkyWay as seen from Earth). It provides an enhanced apparent core
// brightness appMag  ~ absMag - enhance. 'enhance' thus serves to uniformly
// enhance the too low sprite luminosity at close distance.
constexpr double enhance = 4.0;
constexpr double pc10 = 32.6167; // 10 parsecs
constexpr float CubeCornerToCenterDistance = 1.7320508075688772f;

enum class DeepSkyObjectType
{
    Galaxy,
    Globular,
    Nebula,
    OpenCluster
};

DeepSkyObjectType
GetDSOType(const DeepSkyObject* dso)
{
    if (!strcmp(dso->getObjTypeName(), "galaxy"))
        return DeepSkyObjectType::Galaxy;

    if (!strcmp(dso->getObjTypeName(), "globular"))
        return DeepSkyObjectType::Globular;

    if (!strcmp(dso->getObjTypeName(), "nebula"))
        return DeepSkyObjectType::Nebula;

    if (!strcmp(dso->getObjTypeName(), "opencluster"))
        return DeepSkyObjectType::OpenCluster;
}

float
brightness(float avgAbsMag, float absMag, float appMag, float brightnessCorr, float faintestMag)
{
    // Input: display looks satisfactory for 0.2 < brightness < O(1.0)
    // Ansatz: brightness = a - b * appMag(distanceToDSO), emulating eye sensitivity...
    // determine a,b such that
    // a - b * absMag = absMag / avgAbsMag ~ 1; a - b * faintestMag = 0.2.
    // The 2nd eq. guarantees that the faintest galaxies are still visible.

    float r = absMag / avgAbsMag;
    float b = r - (r - 0.2f) * (absMag - appMag) / (absMag - faintestMag);

    // obviously, brightness(appMag = absMag) = r and
    // brightness(appMag = faintestMag) = 0.2, as desired.

    return std::max(0.0f, b * brightnessCorr);
}

} // anonymous namespace

DSORenderer::DSORenderer() :
    ObjectRenderer<DeepSkyObject*, double>(DSO_OCTREE_ROOT_SIZE)
{
}

void DSORenderer::process(DeepSkyObject* const &dso,
                          double distanceToDSO,
                          float absMag)
{
    if (distanceToDSO > distanceLimit || !dso->isVisible())
        return;

    Eigen::Vector3f relPos = (dso->getPosition() - obsPos).cast<float>();
    Eigen::Vector3f center = orientationMatrixT * relPos;

    // Test the object's bounding sphere against the view frustum. If we
    // avoid this stage, overcrowded octree cells may hit performance badly:
    // each object (even if it's not visible) would be sent to the OpenGL
    // pipeline.
    double dsoRadius = dso->getBoundingSphereRadius();
    if (frustum.testSphere(center, (float) dsoRadius) == celmath::Frustum::Outside)
        return;

    float appMag;
    if (distanceToDSO >= pc10)
        appMag = (float) astro::absToAppMag((double) absMag, distanceToDSO);
    else
        appMag = absMag + (float) (enhance * tanh(distanceToDSO/pc10 - 1.0));

    if ((renderFlags & dso->getRenderMask()) != 0)
    {
        dsosProcessed++;

        float nearZ = 0.0f, farZ = 0.0f;
        if (dsoRadius < 1000.0)
        {
            // Small objects may be prone to clipping; give them special
            // handling.  We don't want to always set the projection
            // matrix, since that could be expensive with large galaxy
            // catalogs.
            nearZ = static_cast<float>(distanceToDSO / 2.0);
            farZ = static_cast<float>(distanceToDSO + dsoRadius * 2.0 * CubeCornerToCenterDistance);
            auto minZ = static_cast<float>(dsoRadius * 0.001);
            if (nearZ < minZ)
            {
                nearZ = minZ;
                farZ = nearZ * 10000.0f;
            }
        }

        float b = 2.3f * (faintestMag - 4.75f) / renderer->getFaintestAM45deg(); // brightnesCorr
        switch (GetDSOType(dso))
        {
        case DeepSkyObjectType::Galaxy:
            // -19.04f == average over 10937 galaxies in galaxies.dsc.
            b = brightness(-19.04f, absMag, appMag, b, faintestMag);
            galaxyRenderer->add(reinterpret_cast<Galaxy*>(dso), relPos, b, nearZ, farZ);
            break;
        case DeepSkyObjectType::Globular:
            // -6.86f == average over 150 globulars in globulars.dsc.
            b = brightness(-6.86f, absMag, appMag, b, faintestMag);
            globularRenderer->add(reinterpret_cast<Globular*>(dso), relPos, b, nearZ, farZ);
            break;
        case DeepSkyObjectType::Nebula:
            b = brightness(avgAbsMag, absMag, appMag, b, faintestMag);
            nebulaRenderer->add(reinterpret_cast<Nebula*>(dso), relPos, b, nearZ, farZ);
            break;
        case DeepSkyObjectType::OpenCluster:
            b = brightness(avgAbsMag, absMag, appMag, b, faintestMag);
            openClusterRenderer->add(reinterpret_cast<OpenCluster*>(dso), relPos, b, nearZ, farZ);
            break;
        }
    } // renderFlags check

    // Only render those labels that are in front of the camera:
    // Place labels for DSOs brighter than the specified label threshold brightness
    //
    unsigned int labelMask = dso->getLabelMask();

    if ((labelMask & labelMode) != 0)
    {
        Color labelColor;
        float appMagEff = 6.0f;
        float step = 6.0f;
        float symbolSize = 0.0f;
        celestia::MarkerRepresentation* rep = nullptr;

        // Use magnitude based fading for galaxies, and distance based
        // fading for nebulae and open clusters.
        switch (labelMask)
        {
        case Renderer::NebulaLabels:
            rep = &renderer->nebulaRep;
            labelColor = Renderer::NebulaLabelColor;
            appMagEff = astro::absToAppMag(-7.5f, (float)distanceToDSO);
            symbolSize = (float)(dso->getRadius() / distanceToDSO) / pixelSize;
            step = 6.0f;
            break;
        case Renderer::OpenClusterLabels:
            rep = &renderer->openClusterRep;
            labelColor = Renderer::OpenClusterLabelColor;
            appMagEff = astro::absToAppMag(-6.0f, (float)distanceToDSO);
            symbolSize = (float)(dso->getRadius() / distanceToDSO) / pixelSize;
            step = 4.0f;
            break;
        case Renderer::GalaxyLabels:
            labelColor = Renderer::GalaxyLabelColor;
            appMagEff = appMag;
            step = 6.0f;
            break;
        case Renderer::GlobularLabels:
            labelColor = Renderer::GlobularLabelColor;
            appMagEff = appMag;
            step = 3.0f;
            break;
        default:
            // Unrecognized object class
            labelColor = Color::White;
            appMagEff = appMag;
            step = 6.0f;
            break;
        }

        if (appMagEff < labelThresholdMag)
        {
            // introduce distance dependent label transparency.
            float distr = std::min(1.0f, step * (labelThresholdMag - appMagEff) / labelThresholdMag);
            labelColor.alpha(distr * labelColor.alpha());

            renderer->addBackgroundAnnotation(rep,
                                              dsoDB->getDSOName(dso, true),
                                              labelColor,
                                              relPos,
                                              Renderer::LabelHorizontalAlignment::Start,
                                              Renderer::LabelVerticalAlignment::Center,
                                              symbolSize);
        }
    }     // labels enabled
}
