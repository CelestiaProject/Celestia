//
// C++ Interface: dsooctree
//
// Description:
//
//
// Author: Toti <root@totibox>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef _DSOOCTREE_H_
#define _DSOOCTREE_H_

#include <celengine/deepskyobj.h>
#include <celengine/octree.h>


typedef DynamicOctree  <DeepSkyObject*, double> DynamicDSOOctree;
typedef StaticOctree   <DeepSkyObject*, double> DSOOctree;
typedef OctreeProcessor<DeepSkyObject*, double> DSOHandler;

#endif  // _DSOOCTREE_H_
