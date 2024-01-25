// projectionmode.cpp
//
// Copyright (C) 2023-present, Celestia Development Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "projectionmode.h"

namespace celestia::engine
{

ProjectionMode::ProjectionMode(float width, float height, int distanceToScreen, int screenDpi) :
    width(width),
    height(height),
    distanceToScreen(distanceToScreen),
    screenDpi(screenDpi)
{
}

std::tuple<float, float> ProjectionMode::getDefaultDepthRange() const
{
    return std::make_tuple(0.5f, 1.0e9f);
}

void ProjectionMode::setScreenDpi(int dpi)
{
    screenDpi = dpi;
}

void ProjectionMode::setDistanceToScreen(int dts)
{
    distanceToScreen = dts;
}

void ProjectionMode::setSize(float w, float h)
{
    width = w;
    height = h;
}

}
