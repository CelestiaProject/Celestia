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

#include <celcompat/numbers.h>

using namespace Eigen;

namespace astro = celestia::astro;
namespace numbers = celestia::numbers;

// Maximum permitted orbital radius for stars, in light years. Orbital
// radii larger than this value are not guaranteed to give correct
// results. The problem case is extremely faint stars (such as brown
// dwarfs.) The distance from the viewer to star's barycenter is used
// rough estimate of the brightness for the purpose of culling. When the
// star is very faint, this estimate may not work when the star is
// far from the barycenter. Thus, the star octree traversal will always
// render stars with orbits that are closer than MAX_STAR_ORBIT_RADIUS.
static const float MAX_STAR_ORBIT_RADIUS = 1.0f;

// total specialization of the StaticOctree template process*() methods for stars:
template<>
void StarOctree::processVisibleObjects(StarHandler& processor,
                                       const Vector3f& obsPosition,
                                       const Hyperplane<float, 3>* frustumPlanes,
                                       float limitingFactor,
                                       float scale) const
{
    // See if this node lies within the view frustum

    // Test the cubic octree node against each one of the five
    // planes that define the infinite view frustum.
    for (unsigned int i = 0; i < 5; ++i)
    {
        const Hyperplane<float, 3>& plane = frustumPlanes[i];
        float r = scale * plane.normal().cwiseAbs().sum();
        if (plane.signedDistance(_cellCenterPos) < -r)
            return;
    }

    // Compute the distance to node; this is equal to the distance to
    // the cellCenterPos of the node minus the boundingRadius of the node, scale * SQRT3.
    float minDistance = (obsPosition - _cellCenterPos).norm() - scale * numbers::sqrt3_v<float>;

    // Process the objects in this node
    float dimmest = minDistance > 0 ? astro::appToAbsMag(limitingFactor, minDistance) : 1000;

    for (std::uint32_t i = 0; i < _nObjects; ++i)
    {
        const Star& obj = _firstObject[i];

        if (obj.getAbsoluteMagnitude() < dimmest)
        {
            float distance    = (obsPosition - obj.getPosition()).norm();
            float appMag      = obj.getApparentMagnitude(distance);

            if (appMag < limitingFactor || (distance < MAX_STAR_ORBIT_RADIUS && obj.getOrbit()))
                processor.process(obj, distance, appMag);
        }
    }

    // See if any of the objects in child nodes are potentially included
    // that we need to recurse deeper.
    if (!_children || (minDistance > 0 && astro::absToAppMag(_exclusionFactor, minDistance) > limitingFactor))
        return;

    // Recurse into child nodes
    for (const auto& child : *_children)
    {
        if (child)
            child->processVisibleObjects(processor,
                                         obsPosition,
                                         frustumPlanes,
                                         limitingFactor,
                                         scale * 0.5f);
    }
}

template<>
void
StarOctree::processCloseObjects(StarHandler& processor,
                                const Vector3f& obsPosition,
                                float boundingRadius,
                                float scale) const
{
    // Compute the distance to node; this is equal to the distance to
    // the cellCenterPos of the node minus the boundingRadius of the node, scale * SQRT3.
    float nodeDistance = (obsPosition - _cellCenterPos).norm() - scale * numbers::sqrt3_v<float>;

    if (nodeDistance > boundingRadius)
        return;

    // At this point, we've determined that the cellCenterPos of the node is
    // close enough that we must check individual objects for proximity.

    // Compute distance squared to avoid having to sqrt for distance
    // comparison.
    float radiusSquared    = boundingRadius * boundingRadius;

    // Check all the objects in the node.
    for (std::uint32_t i = 0; i < _nObjects; ++i)
    {
        Star& obj = _firstObject[i];

        if ((obsPosition - obj.getPosition()).squaredNorm() < radiusSquared)
        {
            float distance    = (obsPosition - obj.getPosition()).norm();
            float appMag      = obj.getApparentMagnitude(distance);

            processor.process(obj, distance, appMag);
        }
    }

    if (!_children)
        return;

    // Recurse into child nodes
    for (const auto& child : *_children)
    {
        if (child)
        {
            child->processCloseObjects(processor,
                                       obsPosition,
                                       boundingRadius,
                                       scale * 0.5f);
        }
    }
}
