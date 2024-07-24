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

#include "deepskyobj.h"
#include "octree.h"

namespace celestia::engine
{

using DSOOctree = StaticOctree<std::unique_ptr<DeepSkyObject>, double>;
using DSOHandler = OctreeProcessor<std::unique_ptr<DeepSkyObject>, double>;

template<>
void DSOOctree::processVisibleObjects(DSOHandler&,
                                      const PointType&,
                                      const PlaneType*,
                                      float,
                                      double) const;

template<>
void DSOOctree::processCloseObjects(DSOHandler&,
                                    const PointType&,
                                    double,
                                    double) const;

} // end namespace celestia::engine
