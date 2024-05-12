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

#include <Eigen/Core>

#include <celengine/deepskyobj.h>
#include <celengine/octree.h>

namespace celestia::engine
{

struct DSOOctreeTraits
{
    using object_type = std::unique_ptr<DeepSkyObject>;

    static constexpr std::uint32_t SplitThreshold = 10;

    static constexpr float decayMagnitude(float mag) { return mag + 0.5f; }
    static inline Eigen::Vector3d getPosition(const std::unique_ptr<DeepSkyObject>& obj) { return obj->getPosition(); }
    static inline float getAbsMag(const std::unique_ptr<DeepSkyObject>& obj) { return obj->getAbsoluteMagnitude(); }
    static inline float getRadius(const std::unique_ptr<DeepSkyObject>& obj) { return obj->getBoundingSphereRadius(); }
};

using DSOOctreeBuilder = OctreeBuilder<DSOOctreeTraits>;
using DSOOctree = Octree<std::unique_ptr<DeepSkyObject>, double>;

} // end namespace celestia::engine
