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

#ifndef _CELENGINE_STAROCTREE_H_
#define _CELENGINE_STAROCTREE_H_

#include <celengine/star.h>
#include <celengine/octree.h>


typedef DynamicOctree  <Star, float> DynamicStarOctree;
typedef StaticOctree   <Star, float> StarOctree;
typedef OctreeProcessor<Star, float> StarHandler;

#endif  // _CELENGINE_STAROCTREE_H_
