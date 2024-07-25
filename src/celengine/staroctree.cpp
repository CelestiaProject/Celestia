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

#include <celastro/astro.h>
#include <celcompat/numbers.h>
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
                                                                     util::array_view<PlaneType> frustumPlanes,
                                                                     float limitingFactor) :
    m_starHandler(starHandler),
    m_obsPosition(obsPosition),
    m_frustumPlanes(frustumPlanes),
    m_limitingFactor(limitingFactor)
{
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
        const PlaneType& plane = m_frustumPlanes[i];
        float r = size * plane.normal().cwiseAbs().sum();
        if (plane.signedDistance(center) < -r)
            return false;
    }

    // Compute the distance to node; this is equal to the distance to
    // the cellCenterPos of the node minus the boundingRadius of the node, scale * SQRT3.
    float minDistance = (m_obsPosition - center).norm() - size * numbers::sqrt3_v<float>;

    float distanceModulus = astro::distanceModulus(minDistance);
    if (minDistance > 0.0 && (factor + distanceModulus) > m_limitingFactor)
        return false;

    // Dimmest absolute magnitude to process
    m_dimmest = minDistance > 0 ? (m_limitingFactor - distanceModulus) : 1000;

    return true;
}

void
StarOctreeVisibleObjectsProcessor::process(const Star& obj) const
{
    if (obj.getAbsoluteMagnitude() > m_dimmest)
        return;

    float distance = (m_obsPosition - obj.getPosition()).norm();
    float appMag   = obj.getApparentMagnitude(distance);

    if (appMag <= m_limitingFactor || (distance < MAX_STAR_ORBIT_RADIUS && obj.getOrbit()))
        m_starHandler->process(obj, distance, appMag);
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
    float nodeDistance = (m_obsPosition - center).norm() - size * numbers::sqrt3_v<float>;
    return nodeDistance <= m_boundingRadius;
}

void
StarOctreeCloseObjectsProcessor::process(const Star& obj) const
{
    StarOctree::PointType offset = m_obsPosition - obj.getPosition();
    if (offset.squaredNorm() < m_radiusSquared)
    {
        float distance = offset.norm();
        float appMag   = obj.getApparentMagnitude(distance);

        m_starHandler->process(obj, distance, appMag);
    }
}

} // end namespace celestia::engine
