// psfstarvertexbuffer.h
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
class CelestiaGLProgram;

namespace celestia::gl
{
class Buffer;
class VertexObject;
}

// Vertex buffer used by StarStyle::PointSpreadFunction.
// Vertices carry per-star peak radiance (HDR float) and a linear, green-
// normalised colour.  The shader is the same in both modes; a uniform
// selects "point" (fixed pixel disc) or "glow" (PSF approximation).
class PsfStarVertexBuffer
{
public:
    using capacity_t = unsigned int;

    enum class Mode
    {
        Point = 0,
        Glow  = 1,
    };

    PsfStarVertexBuffer(const Renderer &renderer, capacity_t capacity);
    ~PsfStarVertexBuffer() = default;
    PsfStarVertexBuffer() = delete;
    PsfStarVertexBuffer(const PsfStarVertexBuffer&) = delete;
    PsfStarVertexBuffer(PsfStarVertexBuffer&&) = delete;
    PsfStarVertexBuffer& operator=(const PsfStarVertexBuffer&) = delete;
    PsfStarVertexBuffer& operator=(PsfStarVertexBuffer&&) = delete;

    void start(Mode mode);
    void render();
    void finish();

    void addStar(const Eigen::Vector3f &pos, const Color &color, float peakRadiance);

    void setPointRadius(float r)      { m_pointRadius = r; }
    void setOptimization(float opt)   { m_optimization = opt; }
    void setPointScale(float scale)   { m_pointScale = scale; }

    static void enable();
    static void disable();

private:
    struct StarVertex
    {
        Eigen::Vector3f position;
        float           peakRadiance;
        unsigned char   color[4];
    };

    const Renderer                 &m_renderer;
    capacity_t                      m_capacity;
    capacity_t                      m_nStars                { 0 };
    std::unique_ptr<StarVertex[]>   m_vertices;
    Mode                            m_mode                  { Mode::Point };
    float                           m_pointRadius           { 2.0f };
    float                           m_optimization          { 0.1f };
    float                           m_pointScale            { 1.0f };
    CelestiaGLProgram              *m_prog                  { nullptr };

    std::unique_ptr<celestia::gl::Buffer>        m_bo;
    std::unique_ptr<celestia::gl::VertexObject>  m_vo;
    bool m_initialized{ false };

    static PsfStarVertexBuffer *current;

    void makeCurrent();
    void setupVertexArrayObject();
};
