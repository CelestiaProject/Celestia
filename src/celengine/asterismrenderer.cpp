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
void AsterismRenderer::render(const Renderer &renderer, const Color &defaultColor)
{
    auto *prog = renderer.getShaderManager().getShader(m_shadprop);
    if (prog == nullptr)
        return;

    m_vo.bind();
    if (!m_vo.initialized())
    {
        auto *vtxBuf = prepare();
        if (vtxBuf == nullptr)
        {
            m_vo.unbind();
            return;
        }

        m_vo.allocate(m_vtxTotal * 3 * sizeof(GLfloat), vtxBuf);
        m_vo.setVertices(3, GL_FLOAT, false, 0, 0);
        delete[] vtxBuf;
    }

    prog->use();
    glVertexAttrib(CelestiaGLProgram::ColorAttributeIndex, defaultColor);
    m_vo.draw(GL_LINES, m_vtxTotal);

    assert(m_asterisms->size() == m_vtxCount.size());

    ptrdiff_t offset = 0;
    float opacity = defaultColor.alpha();
    for (size_t size = m_asterisms->size(), i = 0; i < size; i++)
    {
        auto *ast = (*m_asterisms)[i];
        if (!ast->getActive() || !ast->isColorOverridden())
        {
            offset += m_vtxCount[i];
            continue;
        }

	Color color = {ast->getOverrideColor(), opacity};
        glVertexAttrib(CelestiaGLProgram::ColorAttributeIndex, color);
        m_vo.draw(GL_LINES, m_vtxCount[i], offset);
        offset += m_vtxCount[i];
    }

    glUseProgram(0);
    m_vo.unbind();
}

GLfloat* AsterismRenderer::prepare()
{
    // calculate required vertices number
    GLsizei vtx_num = 0;
    for (const auto ast : *m_asterisms)
    {
        GLsizei ast_vtx_num = 0;
        for (int k = 0; k < ast->getChainCount(); k++)
        {
            // as we use GL_LINES we should double the number of vertices
            // as we don't need closed figures we have only one copy of
            // the 1st and last vertexes
            GLsizei s = ast->getChain(k).size();
            if (s > 1)
                ast_vtx_num += 2 * s - 2;
        }

        m_vtxCount.push_back(ast_vtx_num);
        vtx_num += ast_vtx_num;
    }

    if (vtx_num == 0)
        return nullptr;
    m_vtxTotal = vtx_num;

    GLfloat* vtx_buf = new GLfloat[vtx_num * 3];
    GLfloat* ptr = vtx_buf;

    for (const auto ast : *m_asterisms)
    {
        for (int k = 0; k < ast->getChainCount(); k++)
        {
            const auto& chain = ast->getChain(k);

            // skip empty (without starts or only with one star) chains
            if (chain.size() <= 1)
                continue;

            memcpy(ptr, chain[0].data(), 3 * sizeof(float));
            ptr += 3;
            for (unsigned i = 1; i < chain.size() - 1; i++)
            {
                memcpy(ptr,     chain[i].data(), 3 * sizeof(float));
                memcpy(ptr + 3, chain[i].data(), 3 * sizeof(float));
                ptr += 6;
            }
            memcpy(ptr, chain[chain.size() - 1].data(), 3 * sizeof(float));
            ptr += 3;
        }
    }
    return vtx_buf;
}
