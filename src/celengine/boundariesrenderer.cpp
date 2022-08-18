// boundariesrenderer.cpp
//
// Copyright (C) 2018-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include <numeric>
#include <celengine/boundaries.h>
#include <celutil/color.h>
#include "boundariesrenderer.h"
#include "render.h"
#include "vecgl.h"

using celestia::engine::LineRenderer;

BoundariesRenderer::BoundariesRenderer(const Renderer &renderer, const ConstellationBoundaries *boundaries) :
    m_lineRenderer(LineRenderer(renderer, 1.0f, LineRenderer::PrimType::Lines, LineRenderer::StorageType::Static)),
    m_boundaries(boundaries)
{
}

bool BoundariesRenderer::sameBoundaries(const ConstellationBoundaries *boundaries) const
{
    return m_boundaries == boundaries;
}

void BoundariesRenderer::render(const Color &color, const Matrices &mvp)
{
    if (!m_initialized)
    {
        if (!prepare()) return;
        m_initialized = true;
    }

    m_lineRenderer.render(mvp, color, m_lineCount * 2);
    m_lineRenderer.finish();
}


bool BoundariesRenderer::prepare()
{
    auto chains = m_boundaries->getChains();
    auto lineCount = std::accumulate(chains.begin(), chains.end(), 0,
                                     [](int a, ConstellationBoundaries::Chain* b) { return a + b->size() - 1; });

    if (lineCount == 0)
        return false;

    m_lineCount = lineCount;

    for (const auto chain : chains)
    {
        for (unsigned i = 1; i < chain->size(); i++)
            m_lineRenderer.addSegment((*chain)[i - 1], (*chain)[i]);
    }

    return true;
}
