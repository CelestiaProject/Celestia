// dsorenderer.cpp
//
// Copyright (C) 2001-2020, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "dsorenderer.h"

#include <celrender/galaxyrenderer.h>
#include <celrender/globularrenderer.h>
#include <celrender/nebularenderer.h>
#include <celrender/openclusterrenderer.h>
#include "dsodb.h"
#include "deepskyobj.h"
#include "galaxy.h"
#include "globular.h"
#include "nebula.h"
#include "opencluster.h"
#include "render.h"

namespace astro = celestia::astro;
namespace math = celestia::math;
namespace util = celestia::util;

namespace
{
// Retain the legacy close-range core enhancement for extended objects.
constexpr double enhance = 4.0;
constexpr float CubeCornerToCenterDistance = 1.7320508075688772f;

} // anonymous namespace

double
celestia::engine::detail::dsoIrradianceScale(double distanceLightYears)
{
    if (distanceLightYears >= astro::LY_PER_10PARSEC)
    {
        double ratio = astro::LY_PER_10PARSEC / distanceLightYears;
        return ratio * ratio;
    }
    return std::exp(-enhance * std::tanh(distanceLightYears / astro::LY_PER_10PARSEC - 1.0) / astro::LN_MAG);
}

float
celestia::engine::detail::dsoDisplayBrightness(double logReferenceIrradiance, double irradianceScale,
                                               double averageLogIrradiance, double minimumLogIrradiance, float correction)
{
    // Keep the empirical logarithmic display curve, including its 0.2 floor
    // at the visibility limit. Relative irradiance avoids overflow for
    // uncalibrated catalog objects carrying the -1000 magnitude sentinel.
    double referenceBrightness = logReferenceIrradiance / averageLogIrradiance;
    double brightness = referenceBrightness
        + (referenceBrightness - 0.2) * std::log(irradianceScale) / (logReferenceIrradiance - minimumLogIrradiance);
    return std::max(0.0f, static_cast<float>(brightness * correction));
}

DSORenderer::DSORenderer() : ObjectRenderer(DSO_OCTREE_ROOT_SIZE)
{
}

void DSORenderer::process(const std::unique_ptr<DeepSkyObject>& dso, //NOSONAR
                          double distanceToDSO,
                          float irradiance)
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
    if (frustum.testSphere(center, (float) dsoRadius) == math::FrustumAspect::Outside)
        return;

    double irradianceScale = celestia::engine::detail::dsoIrradianceScale(distanceToDSO);
    double logReferenceIrradiance = dso->getLogReferenceIrradiance();

    if (util::is_set(renderFlags, dso->getRenderMask()))
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

        auto brightness = [&](double average) {
            return celestia::engine::detail::dsoDisplayBrightness(logReferenceIrradiance, irradianceScale, average,
                                                                  minimumLogIrradiance, brightnessCorrection);
        };
        switch (dso->getObjType())
        {
        case DeepSkyObjectType::Galaxy:
            // -19.04f == average over 10937 galaxies in galaxies.dsc.
            galaxyRenderer->add(static_cast<const Galaxy*>(dso.get()), relPos, brightness(19.04 / astro::LN_MAG), nearZ, farZ);
            break;
        case DeepSkyObjectType::Globular:
            // -6.86f == average over 150 globulars in globulars.dsc.
            globularRenderer->add(static_cast<const Globular*>(dso.get()), relPos, brightness(6.86 / astro::LN_MAG), nearZ, farZ);
            break;
        case DeepSkyObjectType::Nebula:
            nebulaRenderer->add(static_cast<const Nebula*>(dso.get()), relPos, brightness(averageLogIrradiance), nearZ, farZ);
            break;
        case DeepSkyObjectType::OpenCluster:
            openClusterRenderer->add(static_cast<const OpenCluster*>(dso.get()), relPos, brightness(averageLogIrradiance), nearZ, farZ);
            break;
        default:
            // Unsupported DSO
            break;
        }
    } // renderFlags check

    // Only render those labels that are in front of the camera:
    // Place labels for DSOs brighter than the specified label threshold brightness
    //
    RenderLabels labelMask = dso->getLabelMask();

    if (util::is_set(labelMode, labelMask))
    {
        Color labelColor;
        double labelIrradiance = irradiance;
        if (distanceToDSO < astro::LY_PER_10PARSEC)
            labelIrradiance *= irradianceScale;
        float step = 6.0f;
        float symbolSize = 0.0f;
        const celestia::engine::MarkerRepresentation* rep = nullptr;

        // Nebula and open-cluster labels retain their distance-based reference
        // brightness, independently of the object's catalog photometry.
        switch (labelMask)
        {
        case RenderLabels::NebulaLabels:
            if (distanceToDSO < 0.0)
                return;
            rep = &renderer->nebulaRep;
            labelColor = renderer->colors.NebulaLabel;
            {
                static const float reference = astro::magToIrradiance(-7.5f);
                double ratio = astro::LY_PER_10PARSEC / distanceToDSO;
                labelIrradiance = reference * ratio * ratio;
            }
            symbolSize = (float)(dso->getRadius() / distanceToDSO) / pixelSize;
            step = 6.0f;
            break;
        case RenderLabels::OpenClusterLabels:
            if (distanceToDSO < 0.0)
                return;
            rep = &renderer->openClusterRep;
            labelColor = renderer->colors.OpenClusterLabel;
            {
                static const float reference = astro::magToIrradiance(-6.0f);
                double ratio = astro::LY_PER_10PARSEC / distanceToDSO;
                labelIrradiance = reference * ratio * ratio;
            }
            symbolSize = (float)(dso->getRadius() / distanceToDSO) / pixelSize;
            step = 4.0f;
            break;
        case RenderLabels::GalaxyLabels:
            labelColor = renderer->colors.GalaxyLabel;
            step = 6.0f;
            break;
        case RenderLabels::GlobularLabels:
            labelColor = renderer->colors.GlobularLabel;
            step = 3.0f;
            break;
        default:
            // Unrecognized object class
            labelColor = Color::White;
            step = 6.0f;
            break;
        }

        if (labelIrradiance > labelThresholdIrradiance)
        {
            double opacity = step * labelResponseScale * std::log(labelIrradiance / labelThresholdIrradiance);
            float distr = static_cast<float>(std::min(1.0, opacity));
            labelColor.alpha(distr * labelColor.alpha());

            renderer->addBackgroundAnnotation(rep,
                                              dsoDB->getDSOName(dso.get(), true),
                                              labelColor,
                                              relPos,
                                              Renderer::LabelHorizontalAlignment::Start,
                                              Renderer::LabelVerticalAlignment::Center,
                                              symbolSize);
        }
    }     // labels enabled
}
