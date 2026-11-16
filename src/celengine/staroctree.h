// C++ Interface: staroctree
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

#pragma once

#include <array>
#include <cstdint>
#include <span>

#include <Eigen/Geometry>

#include <celengine/star.h>
#include <celengine/octree.h>

namespace celestia::engine
{

using StarOctree = StaticOctree<Star, float>;
// Callbacks receive distance in light years and apparent magnitude.
using StarHandler = OctreeProcessor<Star, float>;

// Frustum tests are node-level; callbacks may receive stars outside the view.
// Nearby orbiting stars may exceed the magnitude limit so their precise
// distances can be checked by the callback.
class StarOctreeVisibleObjectsProcessor
{
public:
    using PlaneType = Eigen::Hyperplane<float, 3>;

    StarOctreeVisibleObjectsProcessor(StarHandler*,
                                      const StarOctree::PointType&,
                                      std::span<const PlaneType, 5>,
                                      float);

    bool checkNode(const StarOctree::PointType&, float, float);
    void process(const Star&) const;

private:
    StarHandler* m_starHandler;
    StarOctree::PointType m_obsPosition;
    std::span<const PlaneType, 5> m_frustumPlanes;
    std::array<float, 5> m_projectedRadiusFactors;
    float m_limitingFactor;

    float m_dimmest{ 1000.0f };
};

class StarOctreeCloseObjectsProcessor
{
public:
    StarOctreeCloseObjectsProcessor(StarHandler*,
                                    const StarOctree::PointType&,
                                    float);

    bool checkNode(const StarOctree::PointType&, float, float) const;
    void process(const Star&) const;

private:
    StarHandler* m_starHandler;
    StarOctree::PointType m_obsPosition;
    float m_boundingRadius;
    float m_radiusSquared;
};

} // end namespace celestia::engine
