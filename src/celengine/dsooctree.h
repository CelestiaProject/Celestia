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

#pragma once

#include <cstdint>
#include <memory>

#include <Eigen/Geometry>

#include <celutil/array_view.h>
#include "deepskyobj.h"
#include "octree.h"

namespace celestia::engine
{

using DSOOctree = StaticOctree<std::unique_ptr<DeepSkyObject>, double>;
using DSOHandler = OctreeProcessor<std::unique_ptr<DeepSkyObject>, double>;

// This class searches the octree for objects that are likely to be visible
// to a viewer with the specified obsPosition and limitingFactor.  The
// octreeProcessor is invoked for each potentially visible object --no object with
// a property greater than limitingFactor will be processed, but
// objects that are outside the view frustum may be.  Frustum tests are performed
// only at the node level to optimize the octree traversal, so an exact test
// (if one is required) is the responsibility of the callback method.
class DSOOctreeVisibleObjectsProcessor
{
public:
    using PlaneType = Eigen::Hyperplane<double, 3>;

    DSOOctreeVisibleObjectsProcessor(DSOHandler*,
                                     const DSOOctree::PointType&,
                                     util::array_view<PlaneType>,
                                     float);

    bool checkNode(const DSOOctree::PointType&, double, float);
    void process(const std::unique_ptr<DeepSkyObject>&) const; //NOSONAR

private:
    DSOHandler* m_dsoHandler;
    DSOOctree::PointType m_obsPosition;
    util::array_view<PlaneType> m_frustumPlanes;
    float m_limitingFactor;

    float m_dimmest{ 1000.0f };
};

class DSOOctreeCloseObjectsProcessor
{
public:
    DSOOctreeCloseObjectsProcessor(DSOHandler*,
                                   const DSOOctree::PointType&,
                                   double);

    bool checkNode(const DSOOctree::PointType&, double, float) const;
    void process(const std::unique_ptr<DeepSkyObject>&) const; //NOSONAR

private:
    DSOHandler* m_dsoHandler;
    DSOOctree::PointType m_obsPosition;
    double m_boundingRadius;
    double m_radiusSquared;
};

} // end namespace celestia::engine
