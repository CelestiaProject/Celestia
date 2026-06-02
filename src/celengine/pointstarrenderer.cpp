// pointstarrenderer.cpp
//
// Copyright (C) 2001-2019, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "pointstarrenderer.h"

#include <algorithm>
#include <cmath>

#include <celastro/astro.h>
#include <celcompat/numbers.h>
#include <celengine/glsupport.h>
#include <celengine/starcolors.h>
#include <celengine/star.h>
#include <celengine/univcoord.h>
#include <celrender/psfglowlargerenderer.h>
#include "observer.h"
#include "pointstarvertexbuffer.h"
#include "psfstarvertexbuffer.h"
#include "render.h"

#include <fmt/printf.h>

using namespace std;
using namespace Eigen;

namespace astro = celestia::astro;
namespace util = celestia::util;
namespace render = celestia::render;
using render::PsfStarVertexBuffer;
using render::psfGreenNormalization;

// Convert a position in the universal coordinate system to astrocentric
// coordinates, taking into account possible orbital motion of the star.
static Vector3d astrocentricPosition(const UniversalCoord& pos,
                                     const Star& star,
                                     double t)
{
    return pos.offsetFromKm(star.getPosition(t));
}

PointStarRenderer::PointStarRenderer() :
    ObjectRenderer(StarDistanceLimit)
{
}

void PointStarRenderer::process(const Star& star, float distance, float appMag)
{
    if (distance > distanceLimit)
        return;

    Vector3f starPos = star.getPosition();

    // Calculate the difference at double precision *before* converting to float.
    // This is very important for stars that are far from the origin.
    Vector3f relPos = (starPos.cast<double>() - obsPos).cast<float>();
    float    orbitalRadius = star.getOrbitalRadius();
    bool     hasOrbit = orbitalRadius > 0.0f;

    // A very rough check to see if the star may be visible: is the star in
    // front of the viewer? If the star might be close (relPos.x^2 < 0.1) or
    // is moving in an orbit, we'll always regard it as potentially visible.
    // TODO: consider normalizing relPos and comparing relPos*viewNormal against
    // cosFOV--this will cull many more stars than relPos*viewNormal, at the
    // cost of a normalize per star.
    if (relPos.dot(viewNormal) > 0.0f || relPos.x() * relPos.x() < 0.1f || hasOrbit)
    {
        Color starColor = colorTemp->lookupColor(star.getTemperature());
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
        // further than a solar system size if the star is huge, the fov is
        // very small and the resolution is high.  We'll ignore this for now
        // and use the most inexpensive test possible . . .
        if (distance < SolarSystemMaxDistance || orbitSizeInPixels > 1.0f)
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
            appMag = star.getApparentMagnitude(distance);

            discSizeInPixels = star.getRadius() / astro::lightYearsToKilometers(distance) / pixelSize;
        }

        // Stars closer than the maximum solar system size are actually
        // added to the render list and depth sorted, since they may occlude
        // planets.
        if (distance > SolarSystemMaxDistance)
        {
            if (starStyle == StarStyle::PointSpreadFunction)
            {
                // Linear-radiance PSF renderer.  The shader receives a
                // per-vertex peak radiance (cone volume = 1/3 base area
                // x peak):  peakRad = exposure * 3 * irradiance / (pi * r^2).
                // Irradiance is in the Vega-normalised system; the constant
                // SI conversion factor is absorbed by the framebuffer
                // exposure.

                // User-controlled brightness multiplier.  Independent of
                // the magnitude limit; default 1.0 gives visibility down
                // to ~mag 7 with r=2 on an sRGB framebuffer.
                float exposureFactor = std::max(exposure, 1.0e-6f);

                float irradiance = astro::magToIrradiance(appMag);
                // pointRadius is the LOGICAL size in points; gl_PointSize
                // in the shader scales it by pointScale to physical pixels.
                // Computing peakRadiance from the logical radius keeps
                // per-pixel-area brightness invariant across DPIs and lets
                // the glow's gl_PointSize = 2*r*pointScale scale linearly.
                float r          = std::max(pointRadius, 1.0e-3f);
                float peakRad    = exposureFactor * 3.0f * irradiance
                                   / (celestia::numbers::pi_v<float> * r * r);

                float minPeak = (celestia::gl::sRGBRendering
                                    ? astro::LOWEST_IRRADIATION_SRGB
                                    : astro::LOWEST_IRRADIATION)
                                * astro::PSF_PEAK_GATE_FACTOR;

                // Normalise the star colour against its green channel
                // (with a saturation floor) so the brightest channel
                // can't blow out the per-vertex UByte attribute.  The
                // 1/green factor gets folded into peakRadiance so the
                // shader's color * peak still equals the original
                // unnormalised (color * peak).
                float greenScale = 1.0f;
                Color linearStarColor = psfGreenNormalization(starColor, 0.1f, greenScale);
                float peakRadCol = peakRad * greenScale;

                // Point (cone) contribution
                if (peakRadCol > minPeak && psfPointBuffer != nullptr)
                    psfPointBuffer->addStar(relPos, linearStarColor, peakRadCol);

                // Glow (eye-PSF) contribution, additive on top of the point cone
                if (peakRadCol > 1.0f && psfGlowBuffer != nullptr && optimization > 0.0f)
                {
                    // Soft-clip very bright stars so the eye-PSF radius
                    // stays bounded.  Without this, a single bright star
                    // (or a high faintest-magnitude setting) produces a
                    // runaway bloom that washes out the whole frame.
                    float glowPeak = peakRadCol;
                    if (maxIrradiance > 0.0f)
                    {
                        glowPeak = (1.0f - 1.0f / (peakRadCol / maxIrradiance + 1.0f))
                                   * maxIrradiance;
                    }

                    // If the resulting gl_PointSize would exceed the driver
                    // cap, fall back to the clip-space billboard renderer
                    // so the bloom keeps growing past GL_POINT_SIZE_MAX
                    // (otherwise maxIrradiance=0 silently appears capped).
                    float a        = (r > 0.0f) ? (optimization / r) : 0.0f;
                    float rGlowLog = std::pow(glowPeak, 0.4f) / std::max(a, 1.0e-6f);
                    float sizePhys = 2.0f * rGlowLog * pointScale;

                    if (sizePhys > static_cast<float>(celestia::gl::maxPointSize)
                        && psfGlowLargeRenderer != nullptr
                        && psfProj != nullptr && psfModelView != nullptr)
                    {
                        // Flush the in-flight PSF buffers first; the
                        // billboard renderer binds its own program and
                        // would otherwise strand a half-filled buffer.
                        renderer->starPipelineOwner().flush();
                        Matrices mvp { psfProj, psfModelView };
                        psfGlowLargeRenderer->render(relPos,
                                                     linearStarColor,
                                                     glowPeak,
                                                     pointRadius,
                                                     optimization,
                                                     pointScale,
                                                     sizePhys,
                                                     mvp);
                    }
                    else
                    {
                        psfGlowBuffer->addStar(relPos, linearStarColor, glowPeak);
                    }
                }
            }
            else
            {
                float pointSize, alpha, glareSize, glareAlpha;
                float size = BaseStarDiscSize * static_cast<float>(renderer->getScreenDpi()) / 96.0f;
                renderer->calculatePointSize(appMag,
                                             size,
                                             pointSize,
                                             alpha,
                                             glareSize,
                                             glareAlpha);

                if (glareSize != 0.0f)
                    glareVertexBuffer->addStar(relPos, Color(starColor, glareAlpha), glareSize);
                if (pointSize != 0.0f)
                    starVertexBuffer->addStar(relPos, Color(starColor, alpha), pointSize);
            }

            // Place labels for stars brighter than the specified label threshold brightness
            if (util::is_set(labelMode, RenderLabels::StarLabels) && appMag < labelThresholdMag)
            {
                Vector3f starDir = relPos.normalized();
                if (starDir.dot(viewNormal) > cosFOV)
                {
                    float distr = min(1.0f, 3.5f * (labelThresholdMag - appMag)/labelThresholdMag);
                    Color color = Color(renderer->colors.StarLabel, distr * renderer->colors.StarLabel.alpha());
                    renderer->addBackgroundAnnotation(nullptr,
                                                      starDB->getStarName(star, true),
                                                      color,
                                                      relPos);
                }
            }
        }
        else
        {
            Matrix3f viewMat = renderer->getCameraOrientationf().toRotationMatrix();
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

            if (util::is_set(labelMode, RenderLabels::StarLabels))
            {
                // Position the label slightly in front of the object along a line from
                // object center to viewer.
                Vector3f pos = rle.position;
                pos = pos * (1.0f - star.getRadius() * 1.01f / pos.norm());

                renderer->addSortedAnnotation(nullptr,
                                              starDB->getStarName(star, true),
                                              renderer->colors.StarLabel,
                                              pos);
            }
        }
    }
}
