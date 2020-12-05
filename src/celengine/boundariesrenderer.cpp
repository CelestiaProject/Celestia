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
#include "vecgl.h"

using namespace Eigen;
using namespace std;

BoundariesRenderer::BoundariesRenderer(const ConstellationBoundaries *boundaries) :
    m_boundaries(boundaries)
{
    m_shadprop.texUsage = ShaderProperties::VertexColors | ShaderProperties::LineAsTriangles;
    m_shadprop.lightModel = ShaderProperties::UnlitModel;
}

bool BoundariesRenderer::sameBoundaries(const ConstellationBoundaries *boundaries) const
{
    return m_boundaries == boundaries;
}

void BoundariesRenderer::render(const Renderer &renderer, const Color &color, const Matrices &mvp)
{
    auto *prog = renderer.getShaderManager().getShader(m_shadprop);
    if (prog == nullptr)
        return;

    m_vo.bind();
    if (!m_vo.initialized())
    {
        std::vector<LineEnds> data;
        if (!prepare(data))
        {
            m_vo.unbind();
            return;
        }
        m_vo.allocate(m_vtxTotal * sizeof(LineEnds), data.data());
        m_vo.setVertices(3, GL_FLOAT, false, sizeof(LineEnds), offsetof(LineEnds, point1));
        m_vo.setVertexAttribArray(CelestiaGLProgram::NextVCoordAttributeIndex, 3, GL_FLOAT, false, sizeof(LineEnds), offsetof(LineEnds, point2));
        m_vo.setVertexAttribArray(CelestiaGLProgram::ScaleFactorAttributeIndex, 1, GL_FLOAT, false, sizeof(LineEnds), offsetof(LineEnds, scale));
    }

    prog->use();
    prog->setMVPMatrices(*mvp.projection, *mvp.modelview);
    prog->lineWidthX = renderer.getLineWidthX();
    prog->lineWidthY = renderer.getLineWidthY();
    glVertexAttrib(CelestiaGLProgram::ColorAttributeIndex, color);
    m_vo.draw(GL_TRIANGLES, m_vtxTotal);

    m_vo.unbind();
}


bool BoundariesRenderer::prepare(std::vector<LineEnds> &data)
{
    auto chains = m_boundaries->getChains();
    auto vtx_num = accumulate(chains.begin(), chains.end(), 0,
                              [](int a, ConstellationBoundaries::Chain* b) { return a + b->size(); });

    if (vtx_num == 0)
        return false;

    // as we use GL_TRIANGLES we should six times the number of vertices
    vtx_num *= 6;
    m_vtxTotal = vtx_num;

    data.reserve(m_vtxTotal);
    for (const auto chain : chains)
    {
        for (unsigned i = 1; i < chain->size(); i++)
        {
            Eigen::Vector3f prev = (*chain)[i - 1];
            Eigen::Vector3f cur = (*chain)[i];
            data.emplace_back(prev, cur, -0.5);
            data.emplace_back(prev ,cur, 0.5);
            data.emplace_back(cur, prev, -0.5);
            data.emplace_back(cur, prev, -0.5);
            data.emplace_back(cur, prev, 0.5);
            data.emplace_back(prev, cur, -0.5);
        }
    }

    return true;
}
