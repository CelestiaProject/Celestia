// psfglowlargerenderer.cpp
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "psfglowlargerenderer.h"

#include <algorithm>

#include <celastro/astro.h>
#include <celcompat/numbers.h>
#include <celengine/glsupport.h>
#include <celengine/shadermanager.h>

namespace celestia::render
{

PsfGlowLargeRenderer::PsfGlowLargeRenderer(Renderer &renderer, capacity_t capacity) :
    LargeStarRenderer(renderer, StaticShader::PsfStarGlowLarge, capacity)
{
}

PsfGlowLargeRenderer::~PsfGlowLargeRenderer() = default;

void
PsfGlowLargeRenderer::onMakeCurrent(const Eigen::Vector2f &viewportRcp)
{
    float a     = (m_pointRadius > 0.0f) ? (m_optimization / m_pointRadius) : 0.0f;
    float denom = (celestia::numbers::pi_v<float> / std::max(m_pointRadius, 1e-6f)) - a;
    float b     = (denom != 0.0f) ? (1.0f / denom) : 0.0f;

    program()->floatParam("psfA")           = a;
    program()->floatParam("psfB")           = b;
    program()->floatParam("psfMinVisRad")   = celestia::gl::sRGBRendering
                                                  ? celestia::astro::LOWEST_IRRADIATION_SRGB
                                                  : celestia::astro::LOWEST_IRRADIATION;
    program()->floatParam("psfPointScale")  = m_pointScale;
    program()->vec2Param ("psfViewportRcp") = viewportRcp;
}

} // namespace celestia::render
