// psfglowlargerenderer.h
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <celrender/largestarrenderer.h>

namespace celestia::render
{

// Batched billboard renderer for PSF glow stars whose gl_PointSize
// would exceed the driver's GL_ALIASED_POINT_SIZE_RANGE.
class PsfGlowLargeRenderer : public LargeStarRenderer
{
public:
    explicit PsfGlowLargeRenderer(Renderer &renderer, capacity_t capacity = 2048);
    ~PsfGlowLargeRenderer() override;

    void setPointRadius(float r)    { m_pointRadius  = r; }
    void setOptimization(float opt) { m_optimization = opt; }
    void setPointScale(float scale) { m_pointScale   = scale; }

protected:
    void onMakeCurrent(const Eigen::Vector2f &viewportRcp) override;

private:
    float m_pointRadius  { 1.5f };
    float m_optimization { 0.1f };
    float m_pointScale   { 1.0f };
};

} // namespace celestia::render
