// largestarrenderer.h
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

#include <Eigen/Core>

#include <celengine/starpipelineowner.h>
#include <celutil/classops.h>
#include "gl/buffer.h"

class Color;
class Renderer;
class CelestiaGLProgram;
enum class StaticShader;

namespace celestia::gl
{
class VertexObject;
} // namespace celestia::gl

namespace celestia::render
{

// Shared infrastructure for batched billboard renderers that draw point
// stars whose gl_PointSize would exceed GL_ALIASED_POINT_SIZE_RANGE.
// Each star expands to a 4-vertex indexed quad, drawing the whole
// frame's oversized stars in a single call.  Subclasses supply the
// shader id and per-frame GL/shader configuration via onMakeCurrent().
class LargeStarRenderer : public StarPipelineFlushable, private util::NoMove
{
public:
    using capacity_t = unsigned int;

    ~LargeStarRenderer() override;

    LargeStarRenderer() = delete;

    void start();
    void render();
    void finish() override;

    void addStar(const Eigen::Vector3f &center, const Color &color, float scalar,
                 float limbRadius = 0.0f, float alpha = 1.0f);

protected:
    explicit LargeStarRenderer(Renderer &renderer, StaticShader shaderId, capacity_t capacity);

    Renderer&          renderer() noexcept       { return m_renderer; }
    CelestiaGLProgram* program()  const noexcept { return m_prog; }

    // Called from makeCurrent() after the program is bound and MVP set.
    // Derived classes set pipeline state, bind textures, and assign
    // shader-specific uniforms.
    virtual void onMakeCurrent(const Eigen::Vector2f &viewportRcp) = 0;

private:
    struct StarInstance
    {
        Eigen::Vector3f              center;
        float                        scalar;  // peak radiance or pixel size
        float                        limbRadius; // resolved-body limb (px), 0 if none
        float                        alpha;   // PSF glow fade, full float precision
        std::array<unsigned char, 4> color;   // legacy shader also reads .a
    };

    void makeCurrent();
    void setupVertexArrayObject();

    Renderer                         &m_renderer;
    StaticShader                      m_shaderId;
    capacity_t                        m_capacity;
    capacity_t                        m_nStars      { 0 };
    std::unique_ptr<StarInstance[]>   m_instances; //NOSONAR

    CelestiaGLProgram                *m_prog        { nullptr };
    gl::Buffer::SharedPtr             m_bo;
    std::unique_ptr<gl::VertexObject> m_vo;
    bool                              m_initialized { false };
};

} // namespace celestia::render
