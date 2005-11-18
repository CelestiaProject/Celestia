//
// C++ Interface: staroctree
//
// Description:
//
//
// Author: Toti <root@totibox>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef _STAROCTREE_H_
#define _STAROCTREE_H_

#include <celengine/star.h>
#include <celengine/octree.h>


typedef DynamicOctree  <Star, float> DynamicStarOctree;
typedef StaticOctree   <Star, float> StarOctree;
typedef OctreeProcessor<Star, float> StarHandler;

#endif  // _STAROCTREE_H_
