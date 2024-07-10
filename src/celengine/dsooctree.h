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

using DynamicDSOOctree = DynamicOctree<std::unique_ptr<DeepSkyObject>, double>;
using DSOOctree = StaticOctree<std::unique_ptr<DeepSkyObject>, double>;
using DSOHandler = OctreeProcessor<std::unique_ptr<DeepSkyObject>, double>;

template<>
const inline std::uint32_t DynamicDSOOctree::SPLIT_THRESHOLD = 10;

template<>
bool DynamicDSOOctree::exceedsBrightnessThreshold(const std::unique_ptr<DeepSkyObject>&, float); //NOSONAR

template<>
bool DynamicDSOOctree::isStraddling(const Eigen::Vector3d&, const std::unique_ptr<DeepSkyObject>&); //NOSONAR

template<>
float DynamicDSOOctree::applyDecay(float);

template<>
DynamicDSOOctree* DynamicDSOOctree::getChild(const std::unique_ptr<DeepSkyObject>&, const PointType&) const; //NOSONAR

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
