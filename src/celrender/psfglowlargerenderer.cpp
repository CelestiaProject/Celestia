// psfglowlargerenderer.cpp
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "psfglowlargerenderer.h"

#include <array>
#include <cmath>
#include <cstddef>

#include <celengine/glsupport.h>
#include <celengine/render.h>
#include <celengine/shadermanager.h>
#include <celrender/gl/buffer.h>
#include <celrender/gl/vertexobject.h>
#include <celcompat/numbers.h>
#include <celutil/array_view.h>
#include <celutil/color.h>

namespace gl  = celestia::gl;
namespace util = celestia::util;

namespace celestia::render
{

namespace
{
// 6-vertex quad: two triangles, corner ∈ ±0.5, uv ∈ {0,1}.
struct Corner
{
    signed char   x, y;
    unsigned char u, v;
};

constexpr std::array<Corner, 6> kQuadCorners = {{
    { -64,  64, 0, 255 },
    { -64, -64, 0,   0 },
    {  64, -64, 255, 0 },
    { -64,  64, 0, 255 },
    {  64, -64, 255, 0 },
    {  64,  64, 255, 255 },
}};
} // namespace

PsfGlowLargeRenderer::PsfGlowLargeRenderer(Renderer &renderer, capacity_t capacity) :
    m_renderer(renderer),
    m_capacity(capacity),
    m_vertices(static_cast<std::size_t>(capacity) * 6)
{
}

PsfGlowLargeRenderer::~PsfGlowLargeRenderer() = default;

void
PsfGlowLargeRenderer::start()
{
    m_prog = m_renderer.getShaderManager().getShader(StaticShader::PsfStarGlowLarge);
    m_nStars = 0;
}

void
PsfGlowLargeRenderer::render()
{
    if (m_nStars == 0 || m_prog == nullptr)
        return;

    makeCurrent();

    m_bo->invalidateData().setData(
        util::array_view(m_vertices.data(), static_cast<std::size_t>(m_nStars) * 6),
        gl::Buffer::BufferUsage::StreamDraw);

    m_vo->draw(static_cast<int>(m_nStars) * 6);
    m_nStars = 0;
}

void
PsfGlowLargeRenderer::finish()
{
    render();
    m_renderer.starPipelineOwner().clearIfActive(this);
}

void
PsfGlowLargeRenderer::makeCurrent()
{
    auto &owner = m_renderer.starPipelineOwner();
    if (owner.isActive(this) || m_prog == nullptr)
        return;

    owner.setActive(this);  // flushes whoever held the pipeline before

    // The pre-batched renderer always drew with depthTest=false; re-apply
    // that so we don't start z-testing when invoked from the per-interval
    // (solar-system) flush, where the surrounding state has depthTest=true.
    Renderer::PipelineState ps;
    ps.blending  = true;
    ps.blendFunc = {GL_ONE, GL_ONE};
    m_renderer.setPipelineState(ps);

    setupVertexArrayObject();

    m_prog->use();
    m_prog->setMVPMatrices(m_renderer.getCurrentProjectionMatrix(),
                           m_renderer.getCurrentModelViewMatrix());

    float a = (m_pointRadius > 0.0f) ? (m_optimization / m_pointRadius) : 0.0f;
    float denom = (celestia::numbers::pi_v<float> / std::max(m_pointRadius, 1e-6f)) - a;
    float b = (denom != 0.0f) ? (1.0f / denom) : 0.0f;
    m_prog->floatParam("psfA")          = a;
    m_prog->floatParam("psfB")          = b;
    m_prog->floatParam("psfPointScale") = m_pointScale;

    std::array<int, 4> vp{};
    m_renderer.getViewport(vp);
    float invW = (vp[2] > 0) ? (1.0f / static_cast<float>(vp[2])) : 0.0f;
    float invH = (vp[3] > 0) ? (1.0f / static_cast<float>(vp[3])) : 0.0f;
    m_prog->vec2Param("psfViewportRcp") = Eigen::Vector2f(invW, invH);
}

void
PsfGlowLargeRenderer::setupVertexArrayObject()
{
    if (m_initialized)
        return;

    m_initialized = true;

    m_bo = std::make_unique<gl::Buffer>(gl::Buffer::TargetHint::Array);
    m_vo = std::make_unique<gl::VertexObject>(gl::VertexObject::Primitive::Triangles);

    m_vo->addVertexBuffer(
        *m_bo,
        CelestiaGLProgram::VertexCoordAttributeIndex,
        2,
        gl::VertexObject::DataType::Byte,
        true,
        sizeof(StarVertex),
        offsetof(StarVertex, corner));

    m_vo->addVertexBuffer(
        *m_bo,
        CelestiaGLProgram::NormalAttributeIndex,
        3,
        gl::VertexObject::DataType::Float,
        false,
        sizeof(StarVertex),
        offsetof(StarVertex, center));

    m_vo->addVertexBuffer(
        *m_bo,
        CelestiaGLProgram::TextureCoord0AttributeIndex,
        2,
        gl::VertexObject::DataType::UnsignedByte,
        true,
        sizeof(StarVertex),
        offsetof(StarVertex, uv));

    m_vo->addVertexBuffer(
        *m_bo,
        CelestiaGLProgram::ColorAttributeIndex,
        4,
        gl::VertexObject::DataType::UnsignedByte,
        true,
        sizeof(StarVertex),
        offsetof(StarVertex, color));

    m_vo->addVertexBuffer(
        *m_bo,
        CelestiaGLProgram::IntensityAttributeIndex,
        1,
        gl::VertexObject::DataType::Float,
        false,
        sizeof(StarVertex),
        offsetof(StarVertex, peakRadiance));
}

void
PsfGlowLargeRenderer::addStar(const Eigen::Vector3f &center,
                              const Color           &linearColor,
                              float                  peakRadiance)
{
    if (m_nStars < m_capacity)
    {
        std::array<unsigned char, 4> packedColor{};
        linearColor.get(packedColor.data());

        StarVertex *out = &m_vertices[static_cast<std::size_t>(m_nStars) * 6];
        for (std::size_t i = 0; i < 6; ++i)
        {
            out[i].center       = center;
            out[i].peakRadiance = peakRadiance;
            out[i].color        = packedColor;
            out[i].corner       = { kQuadCorners[i].x, kQuadCorners[i].y };
            out[i].uv           = { kQuadCorners[i].u, kQuadCorners[i].v };
        }
        m_nStars++;
    }

    if (m_nStars == m_capacity)
    {
        render();
        m_nStars = 0;
    }
}

} // namespace celestia::render
