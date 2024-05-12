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

using DynamicStarOctree = DynamicOctree<Star, float>;
using StarOctree = StaticOctree<Star, float>;
using StarHandler = OctreeProcessor<Star, float>;

constexpr inline float STAR_OCTREE_ROOT_SIZE = 1000000000.0f;
