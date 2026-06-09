// psfstarvertexbuffer.cpp
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "psfstarvertexbuffer.h"

#include <cmath>

#include <celrender/gl/buffer.h>
#include <celrender/gl/vertexobject.h>
#include <celastro/astro.h>
#include <celcompat/numbers.h>
#include <celutil/color.h>

#include "glsupport.h"
#include "render.h"
#include "shadermanager.h"
#include "starpipelineowner.h"

namespace gl  = celestia::gl;
namespace util = celestia::util;

namespace celestia::render
{

Color
psfGreenNormalization(const Color &c, float saturationLimit, float &greenScale)
{
    float r = c.red();
    float g = c.green();
    float b = c.blue();

    float mx = std::max({ r, g, b });
    if (mx <= 0.0f)
    {
        greenScale = 1.0f;
        return c;
    }

    r /= mx;
    g /= mx;
    b /= mx;

    float mn = std::min({ r, g, b });
    if (float delta = saturationLimit - mn; delta > 0.0f)
    {
        // Desaturate toward white so no channel falls below the limit.
        float dr = 1.0f - r;
        float dg = 1.0f - g;
        float db = 1.0f - b;
        r = std::min(1.0f, r + delta * dr * dr);
        g = std::min(1.0f, g + delta * dg * dg);
        b = std::min(1.0f, b + delta * db * db);
    }

    greenScale = (g > 0.0f) ? (1.0f / g) : 1.0f;
    return Color(r, g, b);
}

PsfStarVertexBuffer::PsfStarVertexBuffer(const Renderer &renderer,
                                         capacity_t capacity) :
    m_renderer(renderer),
    m_capacity(capacity),
    m_vertices(capacity)
{
}

void
PsfStarVertexBuffer::start(Mode mode)
{
    m_mode = mode;
    StaticShader id = (mode == Mode::Point) ? StaticShader::PsfStarPoint
                                            : StaticShader::PsfStarGlow;
    m_prog = m_renderer.getShaderManager().getShader(id);
}

void
PsfStarVertexBuffer::render()
{
    if (m_nStars == 0 || m_prog == nullptr)
        return;

    makeCurrent();

    m_bo->invalidateData().setData(
        util::array_view(m_vertices.data(), m_nStars),
        gl::Buffer::BufferUsage::StreamDraw);

    m_vo->draw(m_nStars);
    m_nStars = 0;
}

void
PsfStarVertexBuffer::makeCurrent()
{
    auto &owner = m_renderer.starPipelineOwner();
    if (owner.isActive(this) || m_prog == nullptr)
        return;

    owner.setActive(this);  // flushes whoever held the pipeline before

    setupVertexArrayObject();

    m_prog->use();
    m_prog->setMVPMatrices(m_renderer.getCurrentProjectionMatrix(),
                           m_renderer.getCurrentModelViewMatrix());
    m_prog->floatParam("pointRadius") = m_pointRadius;
    m_prog->floatParam("pointScale")  = m_pointScale;
    if (m_mode == Mode::Glow)
    {
        float a = (m_pointRadius > 0.0f) ? (m_optimization / m_pointRadius) : 0.0f;
        float denom = (celestia::numbers::pi_v<float> / std::max(m_pointRadius, 1e-6f)) - a;
        float b = (denom != 0.0f) ? (1.0f / denom) : 0.0f;
        m_prog->floatParam("psfA") = a;
        m_prog->floatParam("psfB") = b;
        m_prog->floatParam("psfMinVisRad") = celestia::gl::sRGBRendering
                                                 ? celestia::astro::LOWEST_IRRADIATION_SRGB
                                                 : celestia::astro::LOWEST_IRRADIATION;
    }
}

void
PsfStarVertexBuffer::setupVertexArrayObject()
{
    if (m_initialized)
        return;

    m_initialized = true;

    m_bo = std::make_unique<gl::Buffer>(gl::Buffer::TargetHint::Array);
    m_vo = std::make_unique<gl::VertexObject>(gl::VertexObject::Primitive::Points);

    m_vo->addVertexBuffer(
        *m_bo,
        CelestiaGLProgram::VertexCoordAttributeIndex,
        3,
        gl::VertexObject::DataType::Float,
        false,
        sizeof(StarVertex),
        offsetof(StarVertex, position));

    // peakRadiance bound to IntensityAttributeIndex (vec1 float)
    m_vo->addVertexBuffer(
        *m_bo,
        CelestiaGLProgram::IntensityAttributeIndex,
        1,
        gl::VertexObject::DataType::Float,
        false,
        sizeof(StarVertex),
        offsetof(StarVertex, peakRadiance));

    m_vo->addVertexBuffer(
        *m_bo,
        CelestiaGLProgram::ColorAttributeIndex,
        4,
        gl::VertexObject::DataType::UnsignedByte,
        true,
        sizeof(StarVertex),
        offsetof(StarVertex, color));
}

void
PsfStarVertexBuffer::finish()
{
    render();
    m_renderer.starPipelineOwner().clearIfActive(this);
}

void
PsfStarVertexBuffer::enable()
{
#ifndef GL_ES
    glEnable(GL_PROGRAM_POINT_SIZE);
#endif
}

void
PsfStarVertexBuffer::disable()
{
#ifndef GL_ES
    glDisable(GL_PROGRAM_POINT_SIZE);
#endif
}

void
PsfStarVertexBuffer::addStar(const Eigen::Vector3f &pos,
                             const Color &color,
                             float peakRadiance)
{
    if (m_nStars < m_capacity)
    {
        m_vertices[m_nStars].position = pos;
        m_vertices[m_nStars].peakRadiance = peakRadiance;
        color.get(m_vertices[m_nStars].color.data());
        m_nStars++;
    }

    if (m_nStars == m_capacity)
    {
        render();
        m_nStars = 0;
    }
}
} // namespace celestia::render
