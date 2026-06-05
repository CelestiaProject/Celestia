// largestarrenderer.cpp
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "largestarrenderer.h"

#include <array>
#include <cstddef>

#include <celengine/glsupport.h>
#include <celengine/render.h>
#include <celengine/shadermanager.h>
#include <celrender/gl/buffer.h>
#include <celrender/gl/vertexobject.h>
#include <celutil/array_view.h>
#include <celutil/color.h>

namespace gl   = celestia::gl;
namespace util = celestia::util;

namespace celestia::render
{

namespace
{
struct Corner
{
    signed char   x, y;
    unsigned char u, v;
};

// 4-vertex quad in TL, BL, BR, TR order; drawn as two triangles via
// the static index buffer.
constexpr std::array<Corner, 4> kQuadCorners = {{
    { -127,  127, 0,   0   },
    { -127, -127, 0,   255 },
    {  127, -127, 255, 255 },
    {  127,  127, 255, 0   },
}};

constexpr std::size_t kVerticesPerStar = 4;
constexpr std::size_t kIndicesPerStar  = 6;
} // namespace

LargeStarRenderer::LargeStarRenderer(Renderer    &renderer,
                                     StaticShader shaderId,
                                     capacity_t   capacity) :
    m_renderer(renderer),
    m_shaderId(shaderId),
    m_capacity(capacity),
    m_vertices(static_cast<std::size_t>(capacity) * kVerticesPerStar)
{
}

LargeStarRenderer::~LargeStarRenderer() = default;

void
LargeStarRenderer::start()
{
    m_prog   = m_renderer.getShaderManager().getShader(m_shaderId);
    m_nStars = 0;
}

void
LargeStarRenderer::render()
{
    if (m_nStars == 0 || m_prog == nullptr)
        return;

    makeCurrent();

    m_bo->invalidateData().setData(
        util::array_view(m_vertices.data(), static_cast<std::size_t>(m_nStars) * kVerticesPerStar),
        gl::Buffer::BufferUsage::StreamDraw);

    m_vo->draw(static_cast<int>(m_nStars) * static_cast<int>(kIndicesPerStar));
    m_nStars = 0;
}

void
LargeStarRenderer::finish()
{
    render();
    m_renderer.starPipelineOwner().clearIfActive(this);
}

void
LargeStarRenderer::makeCurrent()
{
    auto &owner = m_renderer.starPipelineOwner();
    if (owner.isActive(this) || m_prog == nullptr)
        return;

    owner.setActive(this);

    setupVertexArrayObject();

    m_prog->use();
    m_prog->setMVPMatrices(m_renderer.getCurrentProjectionMatrix(),
                           m_renderer.getCurrentModelViewMatrix());

    std::array<int, 4> vp{};
    m_renderer.getViewport(vp);
    float invW = (vp[2] > 0) ? (1.0f / static_cast<float>(vp[2])) : 0.0f;
    float invH = (vp[3] > 0) ? (1.0f / static_cast<float>(vp[3])) : 0.0f;

    onMakeCurrent(Eigen::Vector2f(invW, invH));
}

void
LargeStarRenderer::setupVertexArrayObject()
{
    if (m_initialized)
        return;

    m_initialized = true;

    m_bo = std::make_unique<gl::Buffer>(gl::Buffer::TargetHint::Array);
    m_vo = std::make_unique<gl::VertexObject>(gl::VertexObject::Primitive::Triangles);

    // Two triangles per quad: (0,1,2) and (0,2,3).
    std::vector<GLushort> indices(static_cast<std::size_t>(m_capacity) * kIndicesPerStar);
    for (std::size_t i = 0; i < m_capacity; ++i)
    {
        auto base = static_cast<GLushort>(i * kVerticesPerStar);
        auto *out = &indices[i * kIndicesPerStar];
        out[0] = base;
        out[1] = static_cast<GLushort>(base + 1);
        out[2] = static_cast<GLushort>(base + 2);
        out[3] = base;
        out[4] = static_cast<GLushort>(base + 2);
        out[5] = static_cast<GLushort>(base + 3);
    }
    gl::Buffer indexBuffer(gl::Buffer::TargetHint::ElementArray, indices);
    m_vo->setIndexBuffer(std::move(indexBuffer), 0, gl::VertexObject::IndexType::UnsignedShort);

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
        offsetof(StarVertex, scalar));
}

void
LargeStarRenderer::addStar(const Eigen::Vector3f &center,
                           const Color           &color,
                           float                  scalar)
{
    if (m_nStars < m_capacity)
    {
        std::array<unsigned char, 4> packedColor{};
        color.get(packedColor.data());

        StarVertex *out = &m_vertices[static_cast<std::size_t>(m_nStars) * kVerticesPerStar];
        for (std::size_t i = 0; i < kVerticesPerStar; ++i)
        {
            out[i].center = center;
            out[i].scalar = scalar;
            out[i].color  = packedColor;
            out[i].corner = { kQuadCorners[i].x, kQuadCorners[i].y };
            out[i].uv     = { kQuadCorners[i].u, kQuadCorners[i].v };
        }
        m_nStars++;
    }

    if (m_nStars == m_capacity)
        render();
}

} // namespace celestia::render
