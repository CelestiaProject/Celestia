// asterismrenderer.cpp
//
// Copyright (C) 2018-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "asterismrenderer.h"

#include <cassert>
#include <cstddef>

#include <celengine/glsupport.h>
#include <celutil/color.h>

namespace celestia::render
{

AsterismRenderer::AsterismRenderer(const Renderer &renderer, const AsterismList *asterisms) :
    m_lineRenderer(LineRenderer(renderer, 1.0f, LineRenderer::PrimType::Lines, LineRenderer::StorageType::Static)),
    m_asterisms(asterisms)
{
}

bool AsterismRenderer::sameAsterisms(const AsterismList *asterisms) const
{
    return m_asterisms == asterisms;
}

/*! Draw visible asterisms.
 */
void AsterismRenderer::render(const Color &defaultColor, const Matrices &mvp)
{
    if (!m_initialized)
    {
        if (!prepare()) return;
        m_initialized = true;
    }

    m_lineRenderer.render(mvp, defaultColor, m_totalLineCount * 2);

    assert(m_asterisms->size() == m_lineCount.size());

    int offset = 0;
    float opacity = defaultColor.alpha();
    for (std::size_t size = m_asterisms->size(), i = 0; i < size; i++)
    {
        const auto& ast = (*m_asterisms)[i];
        if (!ast.getActive() || !ast.isColorOverridden())
        {
            offset += m_lineCount[i];
            continue;
        }

        Color color = {ast.getOverrideColor(), opacity};
        m_lineRenderer.render(mvp, color, m_lineCount[i] * 2, offset * 2);
        offset += m_lineCount[i];
    }
    m_lineRenderer.finish();
}

bool AsterismRenderer::prepare()
{
    // calculate required vertices number
    GLsizei vtx_num = 0;
    for (const auto& ast : *m_asterisms)
    {
        GLsizei ast_vtx_num = 0;
        for (int k = 0; k < ast.getChainCount(); k++)
        {
            // as we use GL_LINES we should double the number of vertices.
            // as we don't need closed figures we have only one copy of
            // the 1st and last vertexes
            auto s = static_cast<GLsizei>(ast.getChain(k).size());
            if (s > 1)
                ast_vtx_num += s - 1;
        }

        m_lineCount.push_back(ast_vtx_num);
        vtx_num += ast_vtx_num;
    }

    if (vtx_num == 0)
        return false;

    m_totalLineCount = vtx_num;

    for (const auto& ast : *m_asterisms)
    {
        for (int k = 0; k < ast.getChainCount(); k++)
        {
            const auto& chain = ast.getChain(k);
            for (unsigned i = 1; i < chain.size(); i++)
                m_lineRenderer.addSegment(chain[i - 1], chain[i]);
        }
    }

    return true;
}

} // end namespace celestia::render
