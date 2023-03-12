// starfield.cpp
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celrender/gl/buffer.h>
#include <celrender/gl/vertexobject.h>
#include <celutil/color.h>
#include "glsupport.h"
#include "objectrenderer.h"
#include "shadermanager.h"
#include "render.h"
#include "texture.h"
#include "pointstarvertexbuffer.h"

namespace gl = celestia::gl;
namespace util = celestia::util;

PointStarVertexBuffer* PointStarVertexBuffer::current = nullptr;

PointStarVertexBuffer::PointStarVertexBuffer(const Renderer &renderer,
                                             capacity_t capacity) :
    m_renderer(renderer),
    m_capacity(capacity),
    m_vertices(std::make_unique<StarVertex[]>(capacity))
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

        m_bo->bind().invalidateData().setData(
            util::array_view(m_vertices.get(), m_nStars),
            gl::Buffer::BufferUsage::StreamDraw);

        if (m_pointSizeFromVertex)
            m_vo1->draw(gl::VertexObject::Primitive::Points, m_nStars);
        else
            m_vo2->draw(gl::VertexObject::Primitive::Points, m_nStars);
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
    }
    else
    {
        m_prog->pointScale = m_pointScale;
        glVertexAttrib1f(CelestiaGLProgram::PointSizeAttributeIndex, 1.0f);
    }
    current = this;
}

void PointStarVertexBuffer::setupVertexArrayObject()
{
    if (!m_initialized)
    {
        m_initialized = true;

        m_bo = std::make_unique<gl::Buffer>();
        m_vo1 = std::make_unique<gl::VertexObject>();
        m_vo2 = std::make_unique<gl::VertexObject>();

        m_bo->bind();

        m_vo1->addVertexBuffer(
            *m_bo,
            CelestiaGLProgram::VertexCoordAttributeIndex,
            3,
            gl::VertexObject::DataType::Float,
            false,
            sizeof(StarVertex),
            offsetof(StarVertex, position));

        m_vo1->addVertexBuffer(
            *m_bo,
            CelestiaGLProgram::ColorAttributeIndex,
            4,
            gl::VertexObject::DataType::UnsignedByte,
            true,
            sizeof(StarVertex),
            offsetof(StarVertex, color));

        m_vo1->addVertexBuffer(
            *m_bo,
            CelestiaGLProgram::PointSizeAttributeIndex,
            1,
            gl::VertexObject::DataType::Float,
            false,
            sizeof(StarVertex),
            offsetof(StarVertex, size));

        m_vo2->addVertexBuffer(
            *m_bo,
            CelestiaGLProgram::VertexCoordAttributeIndex,
            3,
            gl::VertexObject::DataType::Float,
            false,
            sizeof(StarVertex),
            offsetof(StarVertex, position));

        m_vo2->addVertexBuffer(
            *m_bo,
            CelestiaGLProgram::ColorAttributeIndex,
            4,
            gl::VertexObject::DataType::UnsignedByte,
            true,
            sizeof(StarVertex),
            offsetof(StarVertex, color));
    }
}

void PointStarVertexBuffer::finish()
{
    render();
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
