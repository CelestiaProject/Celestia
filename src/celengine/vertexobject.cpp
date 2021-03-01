// vertexobject.cpp
//
// Copyright (C) 2019, the Celestia Development Team
//
// VBO/VAO wrappper class. Currently GL2/GL2+VAO only.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "shadermanager.h"
#include "vertexobject.h"

using namespace celestia;

namespace celgl
{
VertexObject::VertexObject(GLenum bufferType) :
    m_bufferType(bufferType)
{
}

VertexObject::VertexObject(GLenum bufferType, GLsizeiptr bufferSize, GLenum streamType) :
    m_bufferSize(bufferSize),
    m_bufferType(bufferType),
    m_streamType(streamType)
{
}

VertexObject::~VertexObject()
{
    if (m_vaoId != 0 && isVAOSupported())
        glDeleteVertexArrays(1, &m_vaoId);

    if (m_vboId != 0)
        glDeleteBuffers(1, &m_vboId);
}

void VertexObject::bind(AttributesType attributes) noexcept
{
    m_currentAttributes = attributes;
    if ((m_state & State::Initialize) != 0)
    {
        if (isVAOSupported())
        {
            glGenVertexArrays(1, &m_vaoId);
            glBindVertexArray(m_vaoId);
        }
        glGenBuffers(1, &m_vboId);
        glBindBuffer(m_bufferType, m_vboId);
    }
    else
    {
        if (isVAOSupported())
        {
            glBindVertexArray(m_vaoId);
            if ((m_state & State::Update) != 0)
                glBindBuffer(m_bufferType, m_vboId);
        }
        else
        {
            glBindBuffer(m_bufferType, m_vboId);
            enableAttribArrays();
        }
    }
}

void VertexObject::bindWritable(AttributesType attributes) noexcept
{
    m_state |= State::Update;
    bind(attributes);
}

void VertexObject::unbind() noexcept
{
    if (isVAOSupported())
    {
        if ((m_state & (State::Initialize | State::Update)) != 0)
            glBindBuffer(m_bufferType, 0);
        glBindVertexArray(0);
    }
    else
    {
        disableAttribArrays();
        glBindBuffer(m_bufferType, 0);
    }
    m_state = State::NormalState;
    m_currentAttributes = AttributesType::Invalid;
}

bool VertexObject::allocate(const void* data) noexcept
{
    glBufferData(m_bufferType, m_bufferSize, data, m_streamType);
    return glGetError() != GL_NO_ERROR;
}

bool VertexObject::allocate(GLsizeiptr bufferSize, const void* data) noexcept
{
    m_bufferSize = bufferSize;
    return allocate(data);
}

bool VertexObject::allocate(GLenum bufferType, GLsizeiptr bufferSize, const void* data, GLenum streamType) noexcept
{
    m_bufferType = bufferType;
    m_bufferSize = bufferSize;
    m_streamType = streamType;
    return allocate(data);
}

bool VertexObject::setBufferData(const void* data, GLintptr offset, GLsizeiptr size) noexcept
{
    glBufferSubData(m_bufferType, offset, size == 0 ? m_bufferSize : size, data);
    return glGetError() == GL_NO_ERROR;
}

void VertexObject::draw(GLenum primitive, GLsizei count, GLint first) noexcept
{
    if ((m_state & State::Initialize) != 0)
        enableAttribArrays();

    glDrawArrays(primitive, first, count);
}

void VertexObject::enableAttribArrays() noexcept
{
    glBindBuffer(m_bufferType, m_vboId);
    for (const auto& t : m_attribParams[(unsigned int)m_currentAttributes])
    {
        auto  n = t.first;
        auto& p = t.second;
        glEnableVertexAttribArray(n);
        glVertexAttribPointer(n, p.count, p.type, p.normalized, p.stride, (GLvoid*) p.offset);
    }
}

void VertexObject::disableAttribArrays() noexcept
{
    if (m_currentAttributes == AttributesType::Invalid)
        return;

    for (const auto& t : m_attribParams[(unsigned int)m_currentAttributes])
        glDisableVertexAttribArray(t.first);

    glBindBuffer(m_bufferType, 0);
}

void VertexObject::setVertices(GLint count, GLenum type, bool normalized, GLsizei stride, GLsizeiptr offset, AttributesType attributes) noexcept
{
    setVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex, count, type, normalized, stride, offset, attributes);
}

void VertexObject::setNormals(GLint count, GLenum type, bool normalized, GLsizei stride, GLsizeiptr offset, AttributesType attributes) noexcept
{
    setVertexAttribArray(CelestiaGLProgram::NormalAttributeIndex, count, type, normalized, stride, offset, attributes);
}

void VertexObject::setColors(GLint count, GLenum type, bool normalized, GLsizei stride, GLsizeiptr offset, AttributesType attributes) noexcept
{
    setVertexAttribArray(CelestiaGLProgram::ColorAttributeIndex, count, type, normalized, stride, offset, attributes);
}

void VertexObject::setTextureCoords(GLint count, GLenum type, bool normalized, GLsizei stride, GLsizeiptr offset, AttributesType attributes) noexcept
{
    setVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex, count, type, normalized, stride, offset, attributes);
}

void VertexObject::setTangents(GLint count, GLenum type, bool normalized, GLsizei stride, GLsizeiptr offset, AttributesType attributes) noexcept
{
    setVertexAttribArray(CelestiaGLProgram::TangentAttributeIndex, count, type, normalized, stride, offset, attributes);
}

void VertexObject::setPointSizes(GLint count, GLenum type, bool normalized, GLsizei stride, GLsizeiptr offset, AttributesType attributes) noexcept
{
    setVertexAttribArray(CelestiaGLProgram::PointSizeAttributeIndex, count, type, normalized, offset, stride, attributes);
}

void VertexObject::setVertexAttribArray(GLint location, GLint count, GLenum type, bool normalized, GLsizei stride, GLsizeiptr offset, AttributesType attributes)
{
    if (location < 0)
        return;

    PtrParams p = { offset, stride, count, type, normalized };
    m_attribParams[(unsigned int)attributes][location] = p;
}
} // namespace
