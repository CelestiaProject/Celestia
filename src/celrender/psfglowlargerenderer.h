// psfglowlargerenderer.h
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>

#include <Eigen/Core>

class Color;
class Renderer;
struct Matrices;

namespace celestia::gl
{
class Buffer;
class VertexObject;
} // namespace celestia::gl

namespace celestia::render
{

// Billboard-based fallback for PSF glow stars whose computed gl_PointSize
// would exceed the driver's GL_ALIASED_POINT_SIZE_RANGE upper bound.
// Draws a per-star quad sized in clip space and runs the Spencer (1995)
// photopic-PSF math in the fragment shader, identical to the point-sprite
// path but without the size cap.
class PsfGlowLargeRenderer
{
public:
    explicit PsfGlowLargeRenderer(Renderer &renderer);
    ~PsfGlowLargeRenderer();

    // pointRadius/optimization define the Spencer kernel shape (must match
    // the point-sprite glow pass).  size is the full diameter of the
    // bounding box in physical pixels (= 2 * r_glow_logical * pointScale).
    void render(const Eigen::Vector3f &position,
                const Color           &linearColor,
                float                  peakRadiance,
                float                  pointRadius,
                float                  optimization,
                float                  pointScale,
                float                  sizePhys,
                const Matrices        &mvp);

private:
    void initialize();
    bool m_initialized{ false };

    Renderer &m_renderer;

    std::unique_ptr<gl::Buffer> m_bo;
    std::unique_ptr<gl::VertexObject> m_vo;
};

} // namespace celestia::render
