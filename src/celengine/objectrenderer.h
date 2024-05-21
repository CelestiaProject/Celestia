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

#include <Eigen/Core>
#include "octree.h"

class Observer;
class Renderer;

template<typename PREC>
class ObjectRenderer
{
public:
    bool checkNode(const Eigen::Matrix<PREC, 3, 1>&, PREC, float) const;

    const Observer* observer    { nullptr };
    Renderer* renderer          { nullptr };

    PREC distanceLimit;

    float pixelSize             { 0.0f };
    float faintestMag           { 0.0f };

    // Objects brighter than labelThresholdMag will be labeled
    float labelThresholdMag     { 0.0f };

    std::uint64_t renderFlags   { 0 };
    int labelMode               { 0 };

protected:
    ObjectRenderer(const Observer* _observer, Renderer* _renderer, PREC _distanceLimit) :
        observer(_observer),
        renderer(_renderer),
        distanceLimit(_distanceLimit)
    {
    };

    ~ObjectRenderer() = default;
};

template<typename PREC>
bool
ObjectRenderer<PREC>::checkNode(const Eigen::Matrix<PREC, 3, 1>& center,
                                PREC size,
                                float brightestMag) const
{
    return true;
}
