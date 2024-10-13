// starrenderer.h
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <vector>

#include <Eigen/Core>

#include "objectrenderer.h"
#include "renderlistentry.h"

class ColorTemperatureTable;
class StarVertexBuffer;
class Star;
class StarDatabase;

// TODO: move these variables to StarRenderer class
// without adding a variable. Requires C++17
constexpr inline float StarDistanceLimit     = 1.0e6f;
// Star disc size in pixels
constexpr inline float BaseStarDiscSize      = 5.0f;
constexpr inline float GlareOpacity          = 0.65f;

class StarRenderer : public ObjectRenderer<Star, float>
{
public:
    StarRenderer();
    void process(const Star &star, float distance, float appMag) override;

    Eigen::Vector3d obsPos;
    Eigen::Vector3f viewNormal;
    std::vector<RenderListEntry>* renderList    { nullptr };
    StarVertexBuffer* starVertexBuffer     { nullptr };
    const StarDatabase* starDB                  { nullptr };
    const ColorTemperatureTable* colorTemp      { nullptr };
    float SolarSystemMaxDistance                { 1.0f };
    float cosFOV                                { 1.0f };
};
