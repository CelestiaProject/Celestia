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

#include <Eigen/Geometry>

#include <celastro/astro.h>
#include <celmath/mathlib.h>
#include "observer.h"
#include "pointstarvertexbuffer.h"
#include "render.h"
#include "renderlistentry.h"
#include "star.h"
#include "starcolors.h"

namespace astro = celestia::astro;
namespace math = celestia::math;

namespace
{

// Maximum permitted orbital radius for stars, in light years. Orbital
// radii larger than this value are not guaranteed to give correct
// results. The problem case is extremely faint stars (such as brown
// dwarfs.) The distance from the viewer to star's barycenter is used
// rough estimate of the brightness for the purpose of culling. When the
// star is very faint, this estimate may not work when the star is
// far from the barycenter. Thus, the star octree traversal will always
// render stars with orbits that are closer than MAX_STAR_ORBIT_RADIUS.
constexpr float MAX_STAR_ORBIT_RADIUS = 1.0f;

// Convert a position in the universal coordinate system to astrocentric
// coordinates, taking into account possible orbital motion of the star.
Eigen::Vector3d
astrocentricPosition(const UniversalCoord& pos,
                     const Star& star,
                     double t)
{
    return pos.offsetFromKm(star.getPosition(t));
}

// Calculate the maximum field of view (from top left corner to bottom right) of
// a frustum with the specified aspect ratio (width/height) and vertical field of
// view. We follow the convention used elsewhere and use units of degrees for
// the field of view angle.
float
calcMaxFOV(float fovY_degrees, float aspectRatio)
{
    float l = 1.0f / std::tan(math::degToRad(fovY_degrees * 0.5f));
    return math::radToDeg(std::atan(std::sqrt(aspectRatio * aspectRatio + 1.0f) / l)) * 2.0f;
}

} // end unnamed namespace

PointStarRenderer::PointStarRenderer(const Observer* _observer,
                                     Renderer* _renderer,
                                     const StarDatabase* _starDB,
                                     float _faintestMag,
                                     float _labelThresholdMag) :
    ObjectRenderer<float>(_observer->getPosition(),
                          _observer->getOrientationf(),
                          math::degToRad(_renderer->fov),
                          _renderer->getAspectRatio(),
                          StarDistanceLimit,
                          _faintestMag),
    observerCoord(_observer->getPosition()),
    observerTime(_observer->getTime()),
    viewNormal(_renderer->getCameraOrientationf().conjugate() * -Eigen::Vector3f::UnitZ()),
    viewMatZ(_renderer->getCameraOrientationf().toRotationMatrix().row(2)),
    renderer(_renderer),
    renderList(&_renderer->renderList),
    starVertexBuffer(_renderer->pointStarVertexBuffer),
    glareVertexBuffer(_renderer->glareVertexBuffer),
    starDB(_starDB),
    colorTemp(&_renderer->starColors),
    labelMode(_renderer->getLabelMode()),
    solarSystemMaxDistance(_renderer->SolarSystemMaxDistance),
    cosFOV(std::cos(math::degToRad(calcMaxFOV(_renderer->fov, _renderer->getAspectRatio())) * 0.5f)),
    pixelSize(_renderer->pixelSize),
    size(BaseStarDiscSize * static_cast<float>(_renderer->getScreenDpi()) / 96.0f),
    labelThresholdMag(_labelThresholdMag)
{
}

void
PointStarRenderer::setupVertexBuffers(Texture* starTexture,
                                      Texture* glareTexture,
                                      float pointScale,
                                      bool usePoints) const
{
    starTexture->bind();
    starVertexBuffer->setTexture(starTexture);
    starVertexBuffer->setPointScale(pointScale);
    glareVertexBuffer->setTexture(glareTexture);
    glareVertexBuffer->setPointScale(pointScale);

    PointStarVertexBuffer::enable();
    glareVertexBuffer->startSprites();
    if (usePoints)
        starVertexBuffer->startBasicPoints();
    else
        starVertexBuffer->startSprites();
}

void
PointStarRenderer::finish() const
{
    starVertexBuffer->finish();
    glareVertexBuffer->finish();
}

void
PointStarRenderer::process(const Star& star) const
{
    if (star.getAbsoluteMagnitude() > absMagLimit)
        return;

    // Calculate the difference at double precision *before* converting to float.
    // This is very important for stars that are far from the origin.
    Eigen::Vector3f starPos = star.getPosition();
    Eigen::Vector3f relPos = (starPos.cast<double>() - observerPos).cast<float>();
    float distanceSquared = relPos.squaredNorm();
    if (distanceSquared > distanceLimitSquared)
        return;

    float distance = std::sqrt(distanceSquared);
    float appMag = star.getApparentMagnitude(distance);

    float orbitalRadius = star.getOrbitalRadius();
    bool hasOrbit = orbitalRadius > 0.0f;
    if (!(hasOrbit && distance < MAX_STAR_ORBIT_RADIUS) && appMag > faintestMag)
        return;

    // A very rough check to see if the star may be visible: is the star in
    // front of the viewer? If the star might be close (relPos.x^2 < 0.1) or
    // is moving in an orbit, we'll always regard it as potentially visible.
    // TODO: consider normalizing relPos and comparing relPos*viewNormal against
    // cosFOV--this will cull many more stars than relPos*viewNormal, at the
    // cost of a normalize per star.
    if (!hasOrbit && relPos.dot(viewNormal) <= 0.0f || relPos.x() * relPos.x() > 0.1f)
        return;

    Color starColor = colorTemp->lookupColor(star.getTemperature());
    float discSizeInPixels = 0.0f;
    float orbitSizeInPixels = hasOrbit ? (orbitalRadius / (distance * pixelSize)) : 0.0f;

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
    if (distance < solarSystemMaxDistance || orbitSizeInPixels > 1.0f)
    {
        // Compute the position of the observer relative to the star.
        // This is a much more accurate (and expensive) distance
        // calculation than the previous one which used the observer's
        // position rounded off to floats.
        Eigen::Vector3d hPos = astrocentricPosition(observerCoord,
                                                    star,
                                                    observerTime);
        relPos = hPos.cast<float>() * -astro::kilometersToLightYears(1.0f);
        distance = relPos.norm();

        // Recompute apparent magnitude using new distance computation
        appMag = star.getApparentMagnitude(distance);

        discSizeInPixels = star.getRadius() / astro::lightYearsToKilometers(distance) / pixelSize;
    }

    // Stars closer than the maximum solar system size are actually
    // added to the render list and depth sorted, since they may occlude
    // planets.
    if (distance > solarSystemMaxDistance)
    {
        float pointSize, alpha, glareSize, glareAlpha;
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

        // Place labels for stars brighter than the specified label threshold brightness
        if (((labelMode & Renderer::StarLabels) != 0) && appMag < labelThresholdMag)
        {
            Eigen::Vector3f starDir = relPos.normalized();
            if (starDir.dot(viewNormal) > cosFOV)
            {
                float distr = std::min(1.0f, 3.5f * (labelThresholdMag - appMag) / labelThresholdMag);
                Color color = Color(Renderer::StarLabelColor, distr * Renderer::StarLabelColor.alpha());
                renderer->addBackgroundAnnotation(nullptr,
                                                  starDB->getStarName(star, true),
                                                  color,
                                                  relPos);
            }
        }
    }
    else
    {
        RenderListEntry& rle = renderList->emplace_back();
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

        if ((labelMode & Renderer::StarLabels) != 0)
        {
            // Position the label slightly in front of the object along a line from
            // object center to viewer.
            Eigen::Vector3f pos = rle.position;
            pos = pos * (1.0f - star.getRadius() * 1.01f / pos.norm());

            renderer->addSortedAnnotation(nullptr,
                                          starDB->getStarName(star, true),
                                          Renderer::StarLabelColor,
                                          pos);
        }
    }
}
