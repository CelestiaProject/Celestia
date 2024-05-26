// pointstarrenderer.h
//
// Copyright (C) 2001-2019, the Celestia Development Team
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
#include "univcoord.h"

class ColorTemperatureTable;
class Observer;
class PointStarVertexBuffer;
struct RenderListEntry;
class Renderer;
class Star;
class StarDatabase;
class Texture;

// TODO: move these variables to PointStarRenderer class
// without adding a variable. Requires C++17
constexpr inline float StarDistanceLimit     = 1.0e6f;
// Star disc size in pixels
constexpr inline float BaseStarDiscSize      = 5.0f;
constexpr inline float MaxScaledDiscStarSize = 8.0f;
constexpr inline float GlareOpacity          = 0.65f;

class PointStarRenderer : public ObjectRenderer<float>
{
public:
    PointStarRenderer(const Observer*,
                      Renderer*,
                      const StarDatabase*,
                      float _faintestMag,
                      float _labelThresholdMag);

    void setupVertexBuffers(Texture* starTexture, Texture* glareTexture, float pointScale, bool usePoints) const;
    void finish() const;

    void process(const Star &star) const;

private:
    UniversalCoord observerCoord;
    double observerTime;
    Eigen::Vector3f viewNormal;
    Eigen::Vector3f viewMatZ;
    Renderer* renderer;
    std::vector<RenderListEntry>* renderList;
    PointStarVertexBuffer* starVertexBuffer;
    PointStarVertexBuffer* glareVertexBuffer;
    const StarDatabase* starDB;
    const ColorTemperatureTable* colorTemp;
    unsigned int labelMode;
    float solarSystemMaxDistance;
    float cosFOV;
    float pixelSize;
    float size;
    float labelThresholdMag;
};
