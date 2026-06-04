// psfstarvertexbuffer.h
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

#include "starpipelineowner.h"

class Color;
class Renderer;
class CelestiaGLProgram;

namespace celestia::gl
{
class Buffer;
class VertexObject;
}

namespace celestia::render
{

// Vertex buffer used by StarStyle::PointSpreadFunction.
// Vertices carry per-star peak radiance (HDR float) and a linear, green-
// normalised colour.  The shader is the same in both modes; a uniform
// selects "point" (fixed pixel disc) or "glow" (PSF approximation).
class PsfStarVertexBuffer : public StarPipelineFlushable
{
public:
    using capacity_t = unsigned int;

    enum class Mode
    {
        Point = 0,
        Glow  = 1,
    };

    PsfStarVertexBuffer(const Renderer &renderer, capacity_t capacity);
    ~PsfStarVertexBuffer() override = default;
    PsfStarVertexBuffer() = delete;
    PsfStarVertexBuffer(const PsfStarVertexBuffer&) = delete;
    PsfStarVertexBuffer(PsfStarVertexBuffer&&) = delete;
    PsfStarVertexBuffer& operator=(const PsfStarVertexBuffer&) = delete;
    PsfStarVertexBuffer& operator=(PsfStarVertexBuffer&&) = delete;

    void start(Mode mode);
    void render();
    void finish() override;

    void addStar(const Eigen::Vector3f &pos, const Color &color, float peakRadiance);

    void setPointRadius(float r)      { m_pointRadius = r; }
    void setOptimization(float opt)   { m_optimization = opt; }
    void setPointScale(float scale)   { m_pointScale = scale; }

    static void enable();
    static void disable();

private:
    struct StarVertex
    {
        Eigen::Vector3f            position;
        float                      peakRadiance;
        std::array<unsigned char, 4> color;
    };

    const Renderer                 &m_renderer;
    capacity_t                      m_capacity;
    capacity_t                      m_nStars                { 0 };
    std::vector<StarVertex>         m_vertices;
    Mode                            m_mode                  { Mode::Point };
    float                           m_pointRadius           { 1.5f };
    float                           m_optimization          { 0.1f };
    float                           m_pointScale            { 1.0f };
    CelestiaGLProgram              *m_prog                  { nullptr };

    std::unique_ptr<celestia::gl::Buffer>        m_bo;
    std::unique_ptr<celestia::gl::VertexObject>  m_vo;
    bool m_initialized{ false };

    void makeCurrent();
    void setupVertexArrayObject();
};

// Normalises a linear-RGB star colour following CSR.py's
// `green_normalization()` (V-band reference + saturation limit).  The
// returned colour has max channel = 1 (fits in a UByte attribute) and
// no channel below `saturationLimit`; `greenScale` receives the
// `1 / green` factor that the caller bakes into the per-star peak
// radiance so the shader's `color * peak` product reproduces
// `(color_arr / green) * peak`.
Color psfGreenNormalization(const Color &c, float saturationLimit, float &greenScale);

} // namespace celestia::render
