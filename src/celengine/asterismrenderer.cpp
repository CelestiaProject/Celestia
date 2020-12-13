// asterismrenderer.cpp
//
// Copyright (C) 2018-2019, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include "asterismrenderer.h"
#include "render.h"
#include "vecgl.h"

using namespace std;

AsterismRenderer::AsterismRenderer(const AsterismList *asterisms) :
    m_asterisms(asterisms)
{
    m_shadprop.texUsage = ShaderProperties::VertexColors;
    m_shadprop.lightModel = ShaderProperties::UnlitModel;
}

bool AsterismRenderer::sameAsterisms(const AsterismList *asterisms) const
{
    return m_asterisms == asterisms;
}

/*! Draw visible asterisms.
 */
void AsterismRenderer::render(const Renderer &renderer, const Color &defaultColor, const Matrices &mvp)
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

        // Attributes for lines drawn as triangles
        m_vo.allocate(data.size() * sizeof(LineEnds), data.data());
        m_vo.setVertices(3, GL_FLOAT, false, sizeof(LineEnds), offsetof(LineEnds, point1));
        m_vo.setVertexAttribArray(CelestiaGLProgram::NextVCoordAttributeIndex, 3, GL_FLOAT, false, sizeof(LineEnds), offsetof(LineEnds, point2));
        m_vo.setVertexAttribArray(CelestiaGLProgram::ScaleFactorAttributeIndex, 1, GL_FLOAT, false, sizeof(LineEnds), offsetof(LineEnds, scale));

        // Attributes for lines drawn as lines
        m_vo.setVertices(3, GL_FLOAT, false, sizeof(LineEnds) * 3, offsetof(LineEnds, point1), AttributesType::Alternative1);
    }

    prog->use();
    prog->setMVPMatrices(*mvp.projection, *mvp.modelview);
    glVertexAttrib(CelestiaGLProgram::ColorAttributeIndex, defaultColor);
    if (lineAsTriangles)
    {
        prog->lineWidthX = renderer.getLineWidthX();
        prog->lineWidthY = renderer.getLineWidthY();
        m_vo.draw(GL_TRIANGLES, m_totalLineCount * 6);
    }
    else
    {
        m_vo.draw(GL_LINES, m_totalLineCount * 2);
    }

    assert(m_asterisms->size() == m_lineCount.size());

    ptrdiff_t offset = 0;
    float opacity = defaultColor.alpha();
    for (size_t size = m_asterisms->size(), i = 0; i < size; i++)
    {
        auto *ast = (*m_asterisms)[i];
        if (!ast->getActive() || !ast->isColorOverridden())
        {
            offset += m_lineCount[i];
            continue;
        }

        Color color = {ast->getOverrideColor(), opacity};
        glVertexAttrib(CelestiaGLProgram::ColorAttributeIndex, color);
        if (lineAsTriangles)
            m_vo.draw(GL_TRIANGLES, m_lineCount[i] * 6, offset * 6);
        else
            m_vo.draw(GL_LINES, m_lineCount[i] * 2, offset * 2);
        offset += m_lineCount[i];
    }

    m_vo.unbind();
}

bool AsterismRenderer::prepare(std::vector<LineEnds> &data)
{
    // calculate required vertices number
    GLsizei vtx_num = 0;
    for (const auto ast : *m_asterisms)
    {
        GLsizei ast_vtx_num = 0;
        for (int k = 0; k < ast->getChainCount(); k++)
        {
            // as we use GL_TRIANGLES we should six times the number of
            // vertices as we don't need closed figures we have only one
            // copy of the 1st and last vertexes
            GLsizei s = ast->getChain(k).size();
            if (s > 1)
                ast_vtx_num += s - 1;
        }

        m_lineCount.push_back(ast_vtx_num);
        vtx_num += ast_vtx_num;
    }

    if (vtx_num == 0)
        return false;

    m_totalLineCount = vtx_num;

    // we reserve 6 times the space so we can allow to
    // draw a line segment with two triangles
    data.reserve(m_totalLineCount * 6);
    for (const auto ast : *m_asterisms)
    {
        for (int k = 0; k < ast->getChainCount(); k++)
        {
            const auto& chain = ast->getChain(k);

            // skip empty (without starts or only with one star) chains
            if (chain.size() <= 1)
                continue;

            for (unsigned i = 1; i < chain.size(); i++)
            {
                Eigen::Vector3f prev = chain[i - 1];
                Eigen::Vector3f cur = chain[i];
                data.emplace_back(prev, cur, -0.5);
                data.emplace_back(prev ,cur, 0.5);
                data.emplace_back(cur, prev, -0.5);
                data.emplace_back(cur, prev, -0.5);
                data.emplace_back(cur, prev, 0.5);
                data.emplace_back(prev, cur, -0.5);
            }
        }
    }
    return true;
}
