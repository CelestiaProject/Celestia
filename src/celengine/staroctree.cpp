// staroctree.cpp
//
// Description:
//
// Copyright (C) 2005-2009, Celestia Development Team
// Original version by Toti <root@totibox>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "staroctree.h"

#include <cmath>
#include <limits>
#include <numbers>

#include <celastro/astro.h>
#include <celmath/mathlib.h>

namespace celestia::engine
{

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

} // end unnamed namespace

// The version of cppcheck used by Codacy doesn't seem to detect the field initializer

StarOctreeVisibleObjectsProcessor::StarOctreeVisibleObjectsProcessor(StarHandler* starHandler, // cppcheck-suppress uninitMemberVar
                                                                     const StarOctree::PointType& obsPosition,
                                                                     std::span<const PlaneType, 5> frustumPlanes,
                                                                     float minimumIrradiance) :
    m_starHandler(starHandler),
    m_obsPosition(obsPosition),
    m_frustumPlanes(frustumPlanes),
    m_minimumIrradiance(minimumIrradiance),
    m_limitingMagnitude(minimumIrradiance > 0.0f
                            ? std::nextafter(astro::irradianceToMag(minimumIrradiance), std::numeric_limits<float>::infinity())
                            : std::numeric_limits<float>::infinity())
{
    for (int i = 0; i < 5; ++i)
        m_projectedRadiusFactors[i] = m_frustumPlanes[i].normal().cwiseAbs().sum();
}

bool
StarOctreeVisibleObjectsProcessor::checkNode(const StarOctree::PointType& center,
                                             float size,
                                             float factor)
{
    // See if this node lies within the view frustum

    // Test the cubic octree node against each one of the five
    // planes that define the infinite view frustum.
    for (unsigned int i = 0; i < 5; ++i)
    {
        if (m_frustumPlanes[i].signedDistance(center) < -m_projectedRadiusFactors[i] * size)
            return false;
    }

    // Compute the distance to node; this is equal to the distance to
    // the cellCenterPos of the node minus the boundingRadius of the node, scale * SQRT3.
    // Node factors remain catalog absolute magnitudes; compare their maximum
    // possible irradiance with the same threshold used for individual stars.
    if (float minDistance = (m_obsPosition - center).norm() - size * std::numbers::sqrt3_v<float>;
        !(minDistance <= 0.0f
          || astro::absMagToIrradiance(factor, astro::lightYearsToKilometers(minDistance)) >= m_minimumIrradiance))
        return false;

    m_dimmest = std::numeric_limits<float>::infinity();
    if (m_minimumIrradiance > 0.0f)
    {
        // Clamp in catalog coordinates so the lower bound uses the same
        // floating-point subtraction as the final star distance.
        Eigen::Vector3f halfSize = Eigen::Vector3f::Constant(size);
        Eigen::Vector3f nearestPoint = m_obsPosition.cwiseMax(center - halfSize).cwiseMin(center + halfSize);
        if (float minStarDistance = (m_obsPosition - nearestPoint).norm();
            minStarDistance > MAX_STAR_ORBIT_RADIUS)
        {
            // The magnitude limit is rounded outward at construction; round the
            // subtraction outward too before validating against the irradiance limit.
            float dimmest = std::nextafter(m_limitingMagnitude - astro::distanceModulus(minStarDistance),
                                           std::numeric_limits<float>::infinity());
            // Keep rounded cutoff cases on the final irradiance path. Only reject
            // fainter magnitudes if the prefilter boundary is already below it.
            if (astro::absMagToIrradiance(dimmest, astro::lightYearsToKilometers(minStarDistance)) < m_minimumIrradiance)
                m_dimmest = dimmest;
        }
    }

    return true;
}

void
StarOctreeVisibleObjectsProcessor::process(const Star& obj) const
{
    if (obj.getAbsoluteMagnitude() > m_dimmest && obj.getExtinction() >= 0.0f)
        return;

    float distance = (m_obsPosition - obj.getPosition()).norm();
    float irradiance = obj.getApparentIrradiance(distance);

    if (irradiance >= m_minimumIrradiance || (distance < MAX_STAR_ORBIT_RADIUS && obj.getOrbit()))
        m_starHandler->process(obj, distance, irradiance);
}

StarOctreeCloseObjectsProcessor::StarOctreeCloseObjectsProcessor(StarHandler* starHandler,
                                                                 const StarOctree::PointType& obsPosition,
                                                                 float boundingRadius) :
    m_starHandler(starHandler),
    m_obsPosition(obsPosition),
    m_boundingRadius(boundingRadius),
    m_radiusSquared(math::square(boundingRadius))
{
}

bool
StarOctreeCloseObjectsProcessor::checkNode(const StarOctree::PointType& center,
                                           float size,
                                           float /* factor */) const
{
    // Compute the distance to node; this is equal to the distance to
    // the cellCenterPos of the node minus the boundingRadius of the node, scale * SQRT3.
    float nodeDistance = (m_obsPosition - center).norm() - size * std::numbers::sqrt3_v<float>;
    return nodeDistance <= m_boundingRadius;
}

void
StarOctreeCloseObjectsProcessor::process(const Star& obj) const
{
    StarOctree::PointType offset = m_obsPosition - obj.getPosition();
    if (offset.squaredNorm() < m_radiusSquared)
    {
        float distance = offset.norm();
        float irradiance = obj.getApparentIrradiance(distance);

        m_starHandler->process(obj, distance, irradiance);
    }
}

} // end namespace celestia::engine
