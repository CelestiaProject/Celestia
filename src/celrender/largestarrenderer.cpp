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
    GLbyte  x, y;
    GLubyte u, v;
};

// 4-vertex quad in BL, BR, TL, TR order; drawn as triangle strip
constexpr std::array<Corner, 4> kQuadCorners = {{
    { -127, -127, 0,   255 },
    {  127, -127, 255, 255 },
    { -127,  127, 0,   0   },
    {  127,  127, 255, 0   },
}};

} // namespace

LargeStarRenderer::LargeStarRenderer(Renderer    &renderer,
                                     StaticShader shaderId,
                                     capacity_t   capacity) :
    m_renderer(renderer),
    m_shaderId(shaderId),
    m_capacity(capacity),
    m_vertices(static_cast<std::size_t>(capacity))
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
        util::array_view(m_vertices.data(), static_cast<std::size_t>(m_nStars)),
        gl::Buffer::BufferUsage::StreamDraw);

    m_vo->drawInstances(static_cast<GLsizei>(m_nStars));
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

    auto vertexBuffer = gl::Buffer::create(gl::Buffer::TargetHint::Array, kQuadCorners);

    m_bo = gl::Buffer::create(gl::Buffer::TargetHint::Array);
    m_vo = std::make_unique<gl::VertexObject>(gl::VertexObject::Primitive::TriangleStrip);

    m_vo->addVertexBuffer(
        vertexBuffer,
        CelestiaGLProgram::VertexCoordAttributeIndex,
        2,
        gl::VertexObject::DataType::Byte,
        true,
        sizeof(Corner),
        offsetof(Corner, x));

    m_vo->addVertexBuffer(
        vertexBuffer,
        CelestiaGLProgram::TextureCoord0AttributeIndex,
        2,
        gl::VertexObject::DataType::UnsignedByte,
        true,
        sizeof(Corner),
        offsetof(Corner, u));

    m_vo->setCount(4);

    m_vo->addInstanceBuffer(
        m_bo,
        CelestiaGLProgram::NormalAttributeIndex,
        3,
        gl::VertexObject::DataType::Float,
        false,
        sizeof(StarVertex),
        offsetof(StarVertex, center));

    // Color stays 4-component: this layout is shared with
    // LegacyLargeStarRenderer, whose shader reads in_Color.a.
    m_vo->addInstanceBuffer(
        m_bo,
        CelestiaGLProgram::ColorAttributeIndex,
        4,
        gl::VertexObject::DataType::UnsignedByte,
        true,
        sizeof(StarVertex),
        offsetof(StarVertex, color));

    m_vo->addInstanceBuffer(
        m_bo,
        CelestiaGLProgram::IntensityAttributeIndex,
        1,
        gl::VertexObject::DataType::Float,
        false,
        sizeof(StarVertex),
        offsetof(StarVertex, scalar));

    // continuous distance-derived glow fade; float for a smooth transition
    // (unused by the legacy shader)
    m_vo->addInstanceBuffer(
        m_bo,
        CelestiaGLProgram::AlphaAttributeIndex,
        1,
        gl::VertexObject::DataType::Float,
        false,
        sizeof(StarVertex),
        offsetof(StarVertex, alpha));

    m_vo->addInstanceBuffer(
        m_bo,
        CelestiaGLProgram::LimbRadiusAttributeIndex,
        1,
        gl::VertexObject::DataType::Float,
        false,
        sizeof(StarVertex),
        offsetof(StarVertex, limbRadius));
}

void
LargeStarRenderer::addStar(const Eigen::Vector3f &center,
                           const Color           &color,
                           float                  scalar,
                           float                  limbRadius,
                           float                  alpha)
{
    if (m_nStars < m_capacity)
    {
        std::array<unsigned char, 4> packedColor{};
        color.get(packedColor.data());

        StarVertex &out = m_vertices[static_cast<std::size_t>(m_nStars)];
        out.center     = center;
        out.scalar     = scalar;
        out.limbRadius = limbRadius;
        out.alpha      = alpha;
        out.color      = packedColor;
        ++m_nStars;
    }

    if (m_nStars == m_capacity)
        render();
}

} // namespace celestia::render
