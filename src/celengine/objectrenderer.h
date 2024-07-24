// objectrenderer.h
//
// Copyright (C) 2001-2019, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>

#include "octree.h"

class Observer;
class Renderer;

template<class OBJ, class PREC>
class ObjectRenderer : public celestia::engine::OctreeProcessor<OBJ, PREC>
{
public:
    const Observer* observer    { nullptr };
    Renderer*  renderer         { nullptr };

    float pixelSize             { 0.0f };
    float faintestMag           { 0.0f };
    float distanceLimit         { 0.0f };

    // Objects brighter than labelThresholdMag will be labeled
    float labelThresholdMag     { 0.0f };

    std::uint64_t renderFlags   { 0 };
    int labelMode               { 0 };

protected:
    explicit ObjectRenderer(PREC _distanceLimit) :
        distanceLimit(static_cast<float>(_distanceLimit))
    {
    }
};
