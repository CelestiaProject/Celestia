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

#include <celengine/star.h>
#include <celengine/octree.h>

struct StarOctreeTraits
{
    // In testing, changing SPLIT_THRESHOLD from 100 to 50 nearly
    // doubled the number of nodes in the tree, but provided only between a
    // 0 to 5 percent frame rate improvement.
    static constexpr std::uint32_t SplitThreshold = 75;

    // magnitude increase corresponds to decrease in luminosity by factor of 4
    static constexpr float decayMagnitude(float mag) { return mag + 1.50515f; }
    static inline Eigen::Vector3f getPosition(const Star& obj) { return obj.getPosition(); }
    static inline float getAbsMag(const Star& obj) { return obj.getAbsoluteMagnitude(); }
    static inline float getRadius(const Star& obj) { return obj.getOrbitalRadius(); }
};

using DynamicStarOctree = DynamicOctree<Star, StarOctreeTraits>;
using StarOctree = DynamicStarOctree::static_octree_type;
using StarHandler = OctreeProcessor<Star, float>;
