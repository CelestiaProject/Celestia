// legacylargestarrenderer.cpp
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "legacylargestarrenderer.h"

#include <celengine/glsupport.h>
#include <celengine/render.h>
#include <celengine/shadermanager.h>
#include <celengine/texture.h>

namespace celestia::render
{

LegacyLargeStarRenderer::LegacyLargeStarRenderer(Renderer &renderer, capacity_t capacity) :
    LargeStarRenderer(renderer, StaticShader::LargeStar, capacity)
{
}

LegacyLargeStarRenderer::~LegacyLargeStarRenderer() = default;

void
LegacyLargeStarRenderer::onMakeCurrent(const Eigen::Vector2f &viewportRcp)
{
    if (m_texture != nullptr)
        m_texture->bind();

    program()->samplerParam("starTex") = 0;
    program()->vec2Param("viewportRcp") = viewportRcp;
}

} // namespace celestia::render
