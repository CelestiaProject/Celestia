// starfield.cpp
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "glsupport.h"
#include <celutil/color.h>
#include "objectrenderer.h"
#include "shadermanager.h"
#include "render.h"
#include "texture.h"
#include "pointstarvertexbuffer.h"

PointStarVertexBuffer* PointStarVertexBuffer::current = nullptr;

PointStarVertexBuffer::PointStarVertexBuffer(const Renderer &renderer,
                                             capacity_t capacity) :
    m_renderer(renderer),
    m_capacity(capacity),
    m_vertices(std::make_unique<StarVertex[]>(capacity)),
    m_vao(sizeof(StarVertex) * capacity, GL_STREAM_DRAW)
{
}

void PointStarVertexBuffer::startSprites()
{
    m_prog = m_renderer.getShaderManager().getShader("star");
    m_pointSizeFromVertex = true;
}

void PointStarVertexBuffer::startBasicPoints()
{
    ShaderProperties shadprop;
    shadprop.texUsage = ShaderProperties::VertexColors | ShaderProperties::StaticPointSize;
    shadprop.lightModel = ShaderProperties::UnlitModel;
    m_prog = m_renderer.getShaderManager().getShader(shadprop);
    m_pointSizeFromVertex = false;
}

void PointStarVertexBuffer::render()
{
    if (m_nStars != 0)
    {
        makeCurrent();
        if (m_texture != nullptr)
            m_texture->bind();
        m_vao.allocate(nullptr); // orphan the buffer
        m_vao.setBufferData(m_vertices.get(), 0, m_nStars * sizeof(StarVertex));
        m_vao.draw(GL_POINTS, m_nStars);
        m_nStars = 0;
    }
}

void PointStarVertexBuffer::makeCurrent()
{
    if (current == this || m_prog == nullptr)
        return;

    if (current != nullptr)
        current->finish();

    setupVertexArrayObject();

    m_prog->use();
    m_prog->setMVPMatrices(m_renderer.getCurrentProjectionMatrix(), m_renderer.getCurrentModelViewMatrix());
    if (m_pointSizeFromVertex)
    {
        m_prog->samplerParam("starTex") = 0;
        m_vao.enableVertexAttribArray(CelestiaGLProgram::PointSizeAttributeIndex);
    }
    else
    {
        m_prog->pointScale = m_pointScale;
        m_vao.setVertexAttribConstant(CelestiaGLProgram::PointSizeAttributeIndex, 1.0f);
    }
    current = this;
}

void PointStarVertexBuffer::setupVertexArrayObject()
{
    if (m_vao.initialized())
    {
        m_vao.bindWritable();
    }
    else
    {
        m_vao.bind();

        m_vao.setVertexAttribArray(
            CelestiaGLProgram::VertexCoordAttributeIndex,
            3,
            GL_FLOAT,
            false,
            sizeof(StarVertex),
            offsetof(StarVertex, position));

        m_vao.setVertexAttribArray(
            CelestiaGLProgram::ColorAttributeIndex,
            4,
            GL_UNSIGNED_BYTE,
            true,
            sizeof(StarVertex),
            offsetof(StarVertex, color));

        m_vao.setVertexAttribArray(
            CelestiaGLProgram::PointSizeAttributeIndex,
            1,
            GL_FLOAT,
            false,
            sizeof(StarVertex),
            offsetof(StarVertex, size));
    }
}

void PointStarVertexBuffer::finish()
{
    render();
    m_vao.unbind();
    current = nullptr;
}

void PointStarVertexBuffer::enable()
{
#ifndef GL_ES
    glEnable(GL_POINT_SPRITE);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif
}

void PointStarVertexBuffer::disable()
{
#ifndef GL_ES
    glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glDisable(GL_POINT_SPRITE);
#endif
}

void PointStarVertexBuffer::setTexture(Texture *texture)
{
    m_texture = texture;
}

void PointStarVertexBuffer::setPointScale(float pointSize)
{
    m_pointScale = pointSize;
}
