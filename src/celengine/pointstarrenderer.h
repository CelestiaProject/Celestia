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

#include "univcoord.h"
#include "visibleobjectvisitor.h"

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
// Star disc size in pixels
constexpr inline float BaseStarDiscSize      = 5.0f;
constexpr inline float MaxScaledDiscStarSize = 8.0f;
constexpr inline float GlareOpacity          = 0.65f;

class PointStarRenderer : private VisibleObjectVisitor<float>
{
public:
    PointStarRenderer(const Observer*,
                      Renderer*,
                      const StarDatabase*,
                      float faintestMag,
                      float labelThresholdMag,
                      float distanceLimit);

    void setupVertexBuffers(Texture* starTexture, Texture* glareTexture, float pointScale, bool usePoints) const;
    void finish() const;

    using VisibleObjectVisitor::checkNode;
    void process(const Star &star) const;

private:
    UniversalCoord m_observerCoord;
    double m_observerTime;
    Eigen::Vector3f m_viewNormal;
    Eigen::Vector3f m_viewMatZ;
    Renderer* m_renderer;
    std::vector<RenderListEntry>* m_renderList;
    PointStarVertexBuffer* m_starVertexBuffer;
    PointStarVertexBuffer* m_glareVertexBuffer;
    const StarDatabase* m_starDB;
    const ColorTemperatureTable* m_colorTemp;
    unsigned int m_labelMode;
    float m_solarSystemMaxDistance;
    float m_cosFOV;
    float m_pixelSize;
    float m_size;
    float m_labelThresholdMag;
};
