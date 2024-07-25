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

#include <cstdint>

#include <Eigen/Geometry>

#include <celengine/star.h>
#include <celengine/octree.h>

namespace celestia::engine
{

using StarOctree = StaticOctree<Star, float>;
using StarHandler = OctreeProcessor<Star, float>;

// This class searches the octree for objects that are likely to be visible
// to a viewer with the specified obsPosition and limitingFactor.  The
// octreeProcessor is invoked for each potentially visible object --no object with
// a property greater than limitingFactor will be processed, but
// objects that are outside the view frustum may be.  Frustum tests are performed
// only at the node level to optimize the octree traversal, so an exact test
// (if one is required) is the responsibility of the callback method.
class StarOctreeVisibleObjectsProcessor
{
public:
    using PlaneType = Eigen::Hyperplane<float, 3>;

    StarOctreeVisibleObjectsProcessor(StarHandler*,
                                      const StarOctree::PointType&,
                                      util::array_view<PlaneType>,
                                      float);

    bool checkNode(const StarOctree::PointType&, float, float);
    void process(const Star&) const;

private:
    StarHandler* m_starHandler;
    StarOctree::PointType m_obsPosition;
    util::array_view<PlaneType> m_frustumPlanes;
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
