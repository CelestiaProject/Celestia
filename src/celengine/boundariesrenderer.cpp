// boundariesrenderer.cpp
//
// Copyright (C) 2018-2019, the Celestia Development Team
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

using namespace Eigen;
using namespace std;

BoundariesRenderer::BoundariesRenderer(const ConstellationBoundaries *boundaries) :
    m_boundaries(boundaries)
{
}

bool BoundariesRenderer::sameBoundaries(const ConstellationBoundaries *boundaries) const
{
    return m_boundaries == boundaries;
}

void BoundariesRenderer::render(const Renderer &renderer, const Color &color)
{
    auto *prog = renderer.getShaderManager().getShader(m_shadprop);
    if (prog == nullptr)
        return;

    m_vo.bind();
    if (!m_vo.initialized())
    {
        auto *vtx_buf = prepare();
        if (vtx_buf == nullptr)
        {
            m_vo.unbind();
            return;
        }
        m_vo.allocate(m_vtxTotal * 3 * sizeof(GLshort), vtx_buf);
        m_vo.setVertices(3, GL_SHORT, false, 0, 0);
        delete[] vtx_buf;
    }

    prog->use();
    prog->color = color.toVector4();
    m_vo.draw(GL_LINES, m_vtxTotal);

    glUseProgram(0);
    m_vo.unbind();
}


GLshort* BoundariesRenderer::prepare()
{
    auto chains = m_boundaries->getChains();
    auto vtx_num = accumulate(chains.begin(), chains.end(), 0,
                              [](int a, ConstellationBoundaries::Chain* b) { return a + b->size(); });

    if (vtx_num == 0)
        return nullptr;

    // as we use GL_LINES we should double the number of vertices
    vtx_num *= 2;
    m_vtxTotal = vtx_num;

    auto *vtx_buf = new GLshort[vtx_num * 3];
    GLshort* ptr = vtx_buf;
    for (const auto chain : chains)
    {
        for (unsigned j = 0; j < 3; j++, ptr++)
            *ptr = (GLshort) (*chain)[0][j];
        for (unsigned i = 1; i < chain->size(); i++)
        {
            for (unsigned j = 0; j < 3; j++)
                ptr[j] = ptr[j + 3] = (GLshort) (*chain)[i][j];
            ptr += 6;
        }
        for (unsigned j = 0; j < 3; j++, ptr++)
            *ptr = (GLshort) (*chain)[0][j];
    }

    return vtx_buf;
}
