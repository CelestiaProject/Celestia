// eclipticlinerenderer.cpp
//
// Copyright (C) 2001-present, Celestia Development Team
// Original version Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celmath/mathlib.h> // sincos
#include <celengine/render.h>

#include "eclipticlinerenderer.h"

constexpr float kEclipticScale = 1000.0f;
constexpr int kEclipticCount = 200;

constexpr float PI = celestia::numbers::pi_v<float>;

namespace celestia::render
{
EclipticLineRenderer::EclipticLineRenderer(Renderer &renderer) :
    m_renderer(renderer),
    m_lineRenderer(renderer, 1.0f, LineRenderer::PrimType::LineLoop, LineRenderer::StorageType::Static)
{
}

void
EclipticLineRenderer::init()
{
    m_initialized = true;
    for (int i = 0; i < kEclipticCount; i++)
    {
        float s, c;
        float angle = static_cast<float>(2 * i) / static_cast<float>(kEclipticCount) * PI;
        celmath::sincos(angle, s, c);
        m_lineRenderer.addVertex(c * kEclipticScale, 0.0f, s * kEclipticScale);
    }
}

void
EclipticLineRenderer::render()
{
    if (!m_initialized)
        init();

    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    ps.smoothLines = true;
    m_renderer.setPipelineState(ps);

    m_lineRenderer.render({&m_renderer.getProjectionMatrix(), &m_renderer.getModelViewMatrix()}, Renderer::EclipticColor, kEclipticCount);
    m_lineRenderer.finish();
}
}
