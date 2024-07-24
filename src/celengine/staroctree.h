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

namespace celestia::engine
{

using StarOctree = StaticOctree<Star, float>;
using StarHandler = OctreeProcessor<Star, float>;

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

} // end namespace celestia::engine
