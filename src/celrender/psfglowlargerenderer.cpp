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

#include <celengine/glsupport.h>
#include <celengine/render.h>
#include <celengine/shadermanager.h>
#include <celrender/gl/buffer.h>
#include <celrender/gl/vertexobject.h>
#include <celcompat/numbers.h>
#include <celutil/color.h>

namespace celestia::render
{

PsfGlowLargeRenderer::PsfGlowLargeRenderer(Renderer &renderer) :
    m_renderer(renderer)
{
}

PsfGlowLargeRenderer::~PsfGlowLargeRenderer() = default;

void
PsfGlowLargeRenderer::render(
    const Eigen::Vector3f &position,
    const Color           &linearColor,
    float                  peakRadiance,
    float                  pointRadius,
    float                  optimization,
    float                  sizePhys,
    const Matrices        &mvp)
{
    auto *prog = m_renderer.getShaderManager().getShader(StaticShader::PsfStarGlowLarge);
    if (prog == nullptr)
        return;

    // Same additive linear-radiance pipeline as the point-sprite glow pass.
    Renderer::PipelineState ps;
    ps.blending  = true;
    ps.blendFunc = {GL_ONE, GL_ONE};
    m_renderer.setPipelineState(ps);

    prog->use();
    prog->setMVPMatrices(*mvp.projection, *mvp.modelview);
    prog->vec3Param("center")        = position;

    auto col = linearColor.toVector4();
    prog->vec3Param("color")         = Eigen::Vector3f(col.x(), col.y(), col.z());

    prog->floatParam("peakRadiance") = peakRadiance;

    float a = (pointRadius > 0.0f) ? (optimization / pointRadius) : 0.0f;
    float denom = (celestia::numbers::pi_v<float> / std::max(pointRadius, 1e-6f)) - a;
    float b = (denom != 0.0f) ? (1.0f / denom) : 0.0f;
    prog->floatParam("psfA")         = a;
    prog->floatParam("psfB")         = b;

    // Clip-space full extent of the quad (matches LargeStarRenderer convention).
    // Use the current viewport, not the window — multi-view installs sub-rect
    // viewports and clip-space [-1,1] maps to the active viewport.
    std::array<int, 4> vp{};
    m_renderer.getViewport(vp);
    prog->floatParam("pointWidth")   = sizePhys / static_cast<float>(vp[2]) * 2.0f;
    prog->floatParam("pointHeight")  = sizePhys / static_cast<float>(vp[3]) * 2.0f;

    initialize();
    m_vo->draw();
}

void
PsfGlowLargeRenderer::initialize()
{
    if (m_initialized)
        return;

    m_initialized = true;

    // 6 vertices: position offset in [-0.5, 0.5]^2, texCoord in [0, 1]^2.
    const std::array verts = {
        -0.5f,  0.5f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.0f, 0.0f,
         0.5f, -0.5f, 1.0f, 0.0f,
        -0.5f,  0.5f, 0.0f, 1.0f,
         0.5f, -0.5f, 1.0f, 0.0f,
         0.5f,  0.5f, 1.0f, 1.0f,
    };

    m_bo = std::make_unique<gl::Buffer>(gl::Buffer::TargetHint::Array, verts);
    m_vo = std::make_unique<gl::VertexObject>(gl::VertexObject::Primitive::Triangles);

    m_vo->setCount(6);
    m_vo->addVertexBuffer(
        *m_bo,
        CelestiaGLProgram::VertexCoordAttributeIndex,
        2,
        gl::VertexObject::DataType::Float,
        false,
        4 * sizeof(float),
        0);
    m_vo->addVertexBuffer(
        *m_bo,
        CelestiaGLProgram::TextureCoord0AttributeIndex,
        2,
        gl::VertexObject::DataType::Float,
        false,
        4 * sizeof(float),
        2 * sizeof(float));
}

} // namespace celestia::render
