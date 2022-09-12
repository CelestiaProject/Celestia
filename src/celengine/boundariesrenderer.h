// boundariesrenderer.h
//
// Copyright (C) 2018-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <celrender/linerenderer.h>

class Color;
class ConstellationBoundaries;
class Renderer;
struct Matrices;

class BoundariesRenderer
{
public:
    BoundariesRenderer(const Renderer &renderer, const ConstellationBoundaries*);
    ~BoundariesRenderer() = default;
    BoundariesRenderer() = delete;
    BoundariesRenderer(const BoundariesRenderer&) = delete;
    BoundariesRenderer(BoundariesRenderer&&) = delete;
    BoundariesRenderer& operator=(const BoundariesRenderer&) = delete;
    BoundariesRenderer& operator=(BoundariesRenderer&&) = delete;

    void render(const Color &color, const Matrices &mvp);
    bool sameBoundaries(const ConstellationBoundaries*) const;

private:
    bool prepare();

    celestia::render::LineRenderer  m_lineRenderer;
    const ConstellationBoundaries  *m_boundaries      { nullptr };
    int                             m_lineCount       { 0 };
    bool                            m_initialized     { false };
};
