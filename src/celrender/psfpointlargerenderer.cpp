// psfpointlargerenderer.cpp
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "psfpointlargerenderer.h"

#include <celengine/shadermanager.h>

namespace celestia::render
{

PsfPointLargeRenderer::PsfPointLargeRenderer(const Renderer &renderer, capacity_t capacity) :
    LargeStarRenderer(renderer, StaticShader::PsfStarPoint, capacity, StaticShaderOptions::Billboard)
{
}

void
PsfPointLargeRenderer::onMakeCurrent(const Eigen::Vector2f &viewportRcp)
{
    program()->floatParam("pointRadius") = m_pointRadius;
    program()->floatParam("pointScale") = m_pointScale;
    program()->vec2Param("viewportRcp") = viewportRcp;
}

} // namespace celestia::render
