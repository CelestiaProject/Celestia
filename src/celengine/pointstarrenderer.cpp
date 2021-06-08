// pointstarrenderer.cpp
//
// Copyright (C) 2001-2019, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celengine/starcolors.h>
#include <celengine/star.h>
#include <celengine/univcoord.h>
#include "pointstarvertexbuffer.h"
#include "render.h"
#include "pointstarrenderer.h"

using namespace std;
using namespace Eigen;

// Convert a position in the universal coordinate system to astrocentric
// coordinates, taking into account possible orbital motion of the star.
static Vector3d astrocentricPosition(const UniversalCoord& pos,
                                     const Star& star,
                                     double t)
{
    return pos.offsetFromKm(star.getPosition(t));
}

PointStarRenderer::PointStarRenderer() :
    ObjectRenderer<Star, float>(StarDistanceLimit)
{
}

void PointStarRenderer::process(const Star& star, float distance, float appMag)
{
    nProcessed++;

    Vector3f starPos = star.getPosition();

    // Calculate the difference at double precision *before* converting to float.
    // This is very important for stars that are far from the origin.
    Vector3f relPos = (starPos.cast<double>() - obsPos).cast<float>();
    float    orbitalRadius = star.getOrbitalRadius();
    bool     hasOrbit = orbitalRadius > 0.0f;

    if (distance > distanceLimit)
        return;

    // A very rough check to see if the star may be visible: is the star in
    // front of the viewer? If the star might be close (relPos.x^2 < 0.1) or
    // is moving in an orbit, we'll always regard it as potentially visible.
    // TODO: consider normalizing relPos and comparing relPos*viewNormal against
    // cosFOV--this will cull many more stars than relPos*viewNormal, at the
    // cost of a normalize per star.
    if (relPos.dot(viewNormal) > 0.0f || relPos.x() * relPos.x() < 0.1f || hasOrbit)
    {
#ifdef HDR_COMPRESS
        Color starColorFull = colorTemp->lookupColor(star.getTemperature());
        Color starColor(starColorFull.red()   * 0.5f,
                        starColorFull.green() * 0.5f,
                        starColorFull.blue()  * 0.5f);
#else
        Color starColor = colorTemp->lookupColor(star.getTemperature());
#endif
        float discSizeInPixels = 0.0f;
        float orbitSizeInPixels = 0.0f;

        if (hasOrbit)
            orbitSizeInPixels = orbitalRadius / (distance * pixelSize);

        // Special handling for stars less than one light year away . . .
        // We can't just go ahead and render a nearby star in the usual way
        // for two reasons:
        //   * It may be clipped by the near plane
        //   * It may be large enough that we should render it as a mesh
        //     instead of a particle
        // It's possible that the second condition might apply for stars
        // further than one light year away if the star is huge, the fov is
        // very small and the resolution is high.  We'll ignore this for now
        // and use the most inexpensive test possible . . .
        if (distance < 1.0f || orbitSizeInPixels > 1.0f)
        {
            // Compute the position of the observer relative to the star.
            // This is a much more accurate (and expensive) distance
            // calculation than the previous one which used the observer's
            // position rounded off to floats.
            Vector3d hPos = astrocentricPosition(observer->getPosition(),
                                                 star,
                                                 observer->getTime());
            relPos = hPos.cast<float>() * -astro::kilometersToLightYears(1.0f);
            distance = relPos.norm();

            // Recompute apparent magnitude using new distance computation
            appMag = star.getApparentMagnitude((float)distance);

            discSizeInPixels = star.getRadius() / astro::lightYearsToKilometers(distance) / pixelSize;
            ++nClose;
        }

        // Stars closer than the maximum solar system size are actually
        // added to the render list and depth sorted, since they may occlude
        // planets.
        if (distance > SolarSystemMaxDistance)
        {
#ifdef USE_HDR
            float satPoint = saturationMag;
            float alpha = exposure*(faintestMag - appMag)/(faintestMag - saturationMag + 0.001f);
#else
            float satPoint = faintestMag - (1.0f - brightnessBias) / brightnessScale; // TODO: precompute this value
            float alpha = (faintestMag - appMag) * brightnessScale + brightnessBias;
#endif
#ifdef DEBUG_HDR_ADAPT
            minMag = max(minMag, appMag);
            maxMag = min(maxMag, appMag);
            minAlpha = min(minAlpha, alpha);
            maxAlpha = max(maxAlpha, alpha);
            ++total;
            if (alpha > above)
            {
                ++countAboveN;
            }
#endif

            if (useScaledDiscs)
            {
                float discSize = size;
                if (alpha < 0.0f)
                {
                    alpha = 0.0f;
                }
                else if (alpha > 1.0f)
                {
                    float discScale = min(MaxScaledDiscStarSize, (float) pow(2.0f, 0.3f * (satPoint - appMag)));
                    discSize *= discScale;

                    float glareAlpha = min(0.5f, discScale / 4.0f);
                    glareVertexBuffer->addStar(relPos, Color(starColor, glareAlpha), discSize * 3.0f);

                    alpha = 1.0f;
                }
                starVertexBuffer->addStar(relPos, Color(starColor, alpha), discSize);
            }
            else
            {
                if (alpha < 0.0f)
                {
                    alpha = 0.0f;
                }
                else if (alpha > 1.0f)
                {
                    float discScale = min(100.0f, satPoint - appMag + 2.0f);
                    float glareAlpha = min(GlareOpacity, (discScale - 2.0f) / 4.0f);
                    glareVertexBuffer->addStar(relPos, Color(starColor, glareAlpha), 2.0f * discScale * size);
#ifdef DEBUG_HDR_ADAPT
                    maxSize = max(maxSize, 2.0f * discScale * size);
#endif
                }
                starVertexBuffer->addStar(relPos, Color(starColor, alpha), size);
            }

            ++nRendered;

            // Place labels for stars brighter than the specified label threshold brightness
            if (((labelMode & Renderer::StarLabels) != 0) && appMag < labelThresholdMag)
            {
                Vector3f starDir = relPos.normalized();
                if (starDir.dot(viewNormal) > cosFOV)
                {
                    float distr = min(1.0f, 3.5f * (labelThresholdMag - appMag)/labelThresholdMag);
                    Color color = Color(Renderer::StarLabelColor, distr * Renderer::StarLabelColor.alpha());
                    renderer->addBackgroundAnnotation(nullptr,
                                                      starDB->getStarName(star, true),
                                                      color,
                                                      relPos);
                    nLabelled++;
                }
            }
        }
        else
        {
            Matrix3f viewMat = observer->getOrientationf().toRotationMatrix();
            Vector3f viewMatZ = viewMat.row(2);

            RenderListEntry rle;
            rle.renderableType = RenderListEntry::RenderableStar;
            rle.star = &star;

            // Objects in the render list are always rendered relative to
            // a viewer at the origin--this is different than for distant
            // stars.
            float scale = astro::lightYearsToKilometers(1.0f);
            rle.position = relPos * scale;
            rle.centerZ = rle.position.dot(viewMatZ);
            rle.distance = rle.position.norm();
            rle.radius = star.getRadius();
            rle.discSizeInPixels = discSizeInPixels;
            rle.appMag = appMag;
            rle.isOpaque = true;
            renderList->push_back(rle);

            if ((labelMode & Renderer::StarLabels) != 0)
            {
                // Position the label slightly in front of the object along a line from
                // object center to viewer.
                Vector3f pos = rle.position;
                pos = pos * (1.0f - star.getRadius() * 1.01f / pos.norm());

                renderer->addSortedAnnotation(nullptr,
                                              starDB->getStarName(star, true),
                                              Renderer::StarLabelColor,
                                              pos);
            }
        }
    }
}
