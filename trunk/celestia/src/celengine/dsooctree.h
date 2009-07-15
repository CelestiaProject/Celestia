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

#ifndef _CELENGINE_DSOOCTREE_H_
#define _CELENGINE_DSOOCTREE_H_

#include <celengine/deepskyobj.h>
#include <celengine/octree.h>


typedef DynamicOctree  <DeepSkyObject*, double> DynamicDSOOctree;
typedef StaticOctree   <DeepSkyObject*, double> DSOOctree;
typedef OctreeProcessor<DeepSkyObject*, double> DSOHandler;

#endif  // _CELENGINE_DSOOCTREE_H_
