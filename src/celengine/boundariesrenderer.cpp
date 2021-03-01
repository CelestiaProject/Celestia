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
    m_shadprop.texUsage = ShaderProperties::VertexColors;
    m_shadprop.lightModel = ShaderProperties::UnlitModel;
}

bool BoundariesRenderer::sameBoundaries(const ConstellationBoundaries *boundaries) const
{
    return m_boundaries == boundaries;
}

void BoundariesRenderer::render(const Renderer &renderer, const Color &color, const Matrices &mvp)
{
    using AttributesType = celgl::VertexObject::AttributesType;

    ShaderProperties props = m_shadprop;
    bool lineAsTriangles = renderer.shouldDrawLineAsTriangles();
    if (lineAsTriangles)
        props.texUsage |= ShaderProperties::LineAsTriangles;

    auto *prog = renderer.getShaderManager().getShader(props);
    if (prog == nullptr)
        return;

    m_vo.bind(lineAsTriangles ? AttributesType::Default : AttributesType::Alternative1);
    if (!m_vo.initialized())
    {
        std::vector<LineEnds> data;
        if (!prepare(data))
        {
            m_vo.unbind();
            return;
        }
        m_vo.allocate(data.size() * sizeof(LineEnds), data.data());
        // Attributes for lines drawn as triangles
        m_vo.setVertices(3, GL_FLOAT, false, sizeof(LineEnds), offsetof(LineEnds, point1));
        m_vo.setVertexAttribArray(CelestiaGLProgram::NextVCoordAttributeIndex, 3, GL_FLOAT, false, sizeof(LineEnds), offsetof(LineEnds, point2));
        m_vo.setVertexAttribArray(CelestiaGLProgram::ScaleFactorAttributeIndex, 1, GL_FLOAT, false, sizeof(LineEnds), offsetof(LineEnds, scale));

        // Attributes for lines drawn as lines
        m_vo.setVertices(3, GL_FLOAT, false, sizeof(LineEnds) * 3, offsetof(LineEnds, point1), AttributesType::Alternative1);
    }

    prog->use();
    prog->setMVPMatrices(*mvp.projection, *mvp.modelview);
    glVertexAttrib(CelestiaGLProgram::ColorAttributeIndex, color);
    if (lineAsTriangles)
    {
        prog->lineWidthX = renderer.getLineWidthX();
        prog->lineWidthY = renderer.getLineWidthY();
        m_vo.draw(GL_TRIANGLES, m_lineCount * 6, 0);
    }
    else
    {
        m_vo.draw(GL_LINES, m_lineCount * 2, 0);
    }

    m_vo.unbind();
}


bool BoundariesRenderer::prepare(std::vector<LineEnds> &data)
{
    auto chains = m_boundaries->getChains();
    auto lineCount = accumulate(chains.begin(), chains.end(), 0,
                                [](int a, ConstellationBoundaries::Chain* b) { return a + b->size() - 1; });

    if (lineCount == 0)
        return false;

    m_lineCount = lineCount;

    // we reserve 6 times the space so we can allow to
    // draw a line segment with two triangles
    data.reserve(m_lineCount * 6);
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
