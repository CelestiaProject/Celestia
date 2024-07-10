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

#include <celengine/star.h>
#include <celengine/octree.h>

using DynamicStarOctree = DynamicOctree<Star, float>;
using StarOctree = StaticOctree<Star, float>;
using StarHandler = OctreeProcessor<Star, float>;

// In testing, changing SPLIT_THRESHOLD from 100 to 50 nearly
// doubled the number of nodes in the tree, but provided only between a
// 0 to 5 percent frame rate improvement.
template<>
const inline std::uint32_t DynamicStarOctree::SPLIT_THRESHOLD = 75;

template<>
bool DynamicStarOctree::exceedsBrightnessThreshold(const Star&, float);

template<>
bool DynamicStarOctree::isStraddling(const Eigen::Vector3f&, const Star&);

template<>
float DynamicStarOctree::applyDecay(float excludingFactor);

template<>
DynamicStarOctree* DynamicStarOctree::getChild(const Star&, const Eigen::Vector3f&) const;

template<>
void StarOctree::processVisibleObjects(StarHandler&,
                                       const PointType&,
                                       const PlaneType*,
                                       float,
                                       float) const;

template<>
void StarOctree::processCloseObjects(StarHandler&,
                                     const PointType&,
                                     float,
                                     float) const;
