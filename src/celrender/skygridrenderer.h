// skygridrenderer.cpp
//
// Celestial longitude/latitude grid renderer.
//
// Copyright (C) 2024-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <Eigen/Core>
#include "linerenderer.h"

class Color;
class Renderer;

namespace celestia
{

namespace engine
{
struct SkyGrid;
} // end namespace celestia::engine

namespace render
{

class SkyGridRenderer
{
public:
    explicit SkyGridRenderer(Renderer&);
    ~SkyGridRenderer();

    void render(const engine::SkyGrid& grid, float zoom);

private:
    struct RenderInfo;

    int drawParallels(const RenderInfo&, const Eigen::Matrix3f&, const Color&, int);
    int drawMeridians(const RenderInfo&, const Eigen::Matrix3f&, const engine::SkyGrid&, int);

    LineRenderer m_gridRenderer;
    LineRenderer m_crossRenderer;
    Renderer& m_renderer;
};

} // end namespace celestia::render

} // end namespace celestia
