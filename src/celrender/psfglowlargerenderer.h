// psfglowlargerenderer.h
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <array>
#include <memory>
#include <vector>

#include <Eigen/Core>

#include <celengine/starpipelineowner.h>

class Color;
class Renderer;
class CelestiaGLProgram;

namespace celestia::gl
{
class Buffer;
class VertexObject;
} // namespace celestia::gl

namespace celestia::render
{

// Batched billboard renderer for PSF glow stars whose gl_PointSize
// would exceed the driver's GL_ALIASED_POINT_SIZE_RANGE.  Each star
// is expanded to a 6-vertex quad and submitted with the rest of the
// frame's large glows in a single draw.
class PsfGlowLargeRenderer : public StarPipelineFlushable
{
public:
    using capacity_t = unsigned int;

    explicit PsfGlowLargeRenderer(Renderer &renderer, capacity_t capacity = 2048);
    ~PsfGlowLargeRenderer() override;

    PsfGlowLargeRenderer() = delete;
    PsfGlowLargeRenderer(const PsfGlowLargeRenderer&) = delete;
    PsfGlowLargeRenderer(PsfGlowLargeRenderer&&) = delete;
    PsfGlowLargeRenderer& operator=(const PsfGlowLargeRenderer&) = delete;
    PsfGlowLargeRenderer& operator=(PsfGlowLargeRenderer&&) = delete;

    void start();
    void render();
    void finish() override;

    void addStar(const Eigen::Vector3f &center,
                 const Color           &linearColor,
                 float                  peakRadiance);

    void setPointRadius(float r)    { m_pointRadius = r; }
    void setOptimization(float opt) { m_optimization = opt; }
    void setPointScale(float scale) { m_pointScale = scale; }

private:
    struct StarVertex
    {
        Eigen::Vector3f              center;
        float                        peakRadiance;
        std::array<unsigned char, 4> color;
        std::array<signed char, 2>   corner;  // ±64  -> ±0.5  (signed byte normalized)
        std::array<unsigned char, 2> uv;      // 0/255 -> 0/1  (unsigned byte normalized)
    };

    void makeCurrent();
    void setupVertexArrayObject();

    Renderer                       &m_renderer;
    capacity_t                      m_capacity;
    capacity_t                      m_nStars        { 0 };
    std::vector<StarVertex>         m_vertices;

    float                           m_pointRadius   { 1.5f };
    float                           m_optimization  { 0.1f };
    float                           m_pointScale    { 1.0f };

    CelestiaGLProgram              *m_prog          { nullptr };
    std::unique_ptr<gl::Buffer>     m_bo;
    std::unique_ptr<gl::VertexObject> m_vo;
    bool                            m_initialized   { false };
};

} // namespace celestia::render
