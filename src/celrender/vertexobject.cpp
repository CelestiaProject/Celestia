// vertexobject.cpp
//
// Copyright (C) 2019-present, the Celestia Development Team
//
// VBO/VAO wrappper class. Currently GL2/GL2+VAO only.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "vertexobject.h"

#include <cassert>

namespace
{
[[ nodiscard ]] inline bool isVAOSupported()
{
#ifndef GL_ES
    return celestia::gl::ARB_vertex_array_object;
#else
    return celestia::gl::OES_vertex_array_object;
#endif
}
}

namespace celestia::render
{
VertexObject::VertexObject() = default;

VertexObject::VertexObject(GLsizeiptr bufferSize, GLenum streamType) :
    m_bufferSize(bufferSize),
    m_streamType(streamType)
{
}

VertexObject::VertexObject(VertexObject &&other) noexcept :
    m_attribParams(std::move(other.m_attribParams)),
    m_vboId(other.m_vboId),
    m_vaoId(other.m_vaoId),
    m_bufferSize(other.m_bufferSize),
    m_streamType(other.m_streamType)
{
    other.m_vboId      = 0;
    other.m_vaoId      = 0;
    other.m_bufferSize = 0;
    other.m_streamType = 0;
}

VertexObject& VertexObject::operator=(VertexObject &&other) noexcept
{
    m_attribParams = std::move(other.m_attribParams);
    m_vboId        = other.m_vboId;
    m_vaoId        = other.m_vaoId;
    m_bufferSize   = other.m_bufferSize;
    m_streamType   = other.m_streamType;

    other.m_vboId      = 0;
    other.m_vaoId      = 0;
    other.m_bufferSize = 0;
    other.m_streamType = 0;

    return *this;
}

VertexObject::~VertexObject()
{
    if (m_vaoId != 0 && isVAOSupported())
        glDeleteVertexArrays(1, &m_vaoId);

    if (m_vboId != 0)
        glDeleteBuffers(1, &m_vboId);
}

void VertexObject::bind() noexcept
{
    if ((m_state & State::Initialize) != 0)
    {
        if (isVAOSupported())
        {
            glGenVertexArrays(1, &m_vaoId);
            glBindVertexArray(m_vaoId);
        }
        glGenBuffers(1, &m_vboId);
        glBindBuffer(GL_ARRAY_BUFFER, m_vboId);
    }
    else
    {
        if (isVAOSupported())
        {
            glBindVertexArray(m_vaoId);
            if ((m_state & State::Update) != 0)
                glBindBuffer(GL_ARRAY_BUFFER, m_vboId);
        }
        else
        {
            glBindBuffer(GL_ARRAY_BUFFER, m_vboId);
            enableAttribArrays();
        }
    }
}

void VertexObject::bindWritable() noexcept
{
    m_state |= State::Update;
    bind();
}

void VertexObject::unbind() noexcept
{
    if ((m_state & State::Initialize) != 0)
        enableAttribArrays();

    if (isVAOSupported())
    {
        if ((m_state & (State::Initialize | State::Update)) != 0)
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        m_attribParams.clear(); // we don't need them anymore if VAO is supported
    }
    else
    {
        disableAttribArrays();
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    m_state = State::NormalState;
}

void VertexObject::allocate(const void* data) const noexcept
{
    glBufferData(GL_ARRAY_BUFFER, m_bufferSize, data, m_streamType);
}

void VertexObject::allocate(GLsizeiptr bufferSize, const void* data) noexcept
{
    m_bufferSize = bufferSize;
    allocate(data);
}

void VertexObject::allocate(GLsizeiptr bufferSize, const void* data, GLenum streamType) noexcept
{
    m_bufferSize = bufferSize;
    m_streamType = streamType;
    allocate(data);
}

void VertexObject::setBufferData(const void* data, GLintptr offset, GLsizeiptr size) const noexcept
{
    glBufferSubData(GL_ARRAY_BUFFER, offset, size == 0 ? m_bufferSize : size, data);
}

void VertexObject::draw(GLenum primitive, GLsizei count, GLint first) const noexcept
{
    if ((m_state & State::Initialize) != 0)
        enableAttribArrays();

    glDrawArrays(primitive, first, count);
}

struct VertexObject::PtrParams
{
    PtrParams(GLint location, GLsizeiptr offset, GLsizei stride, GLint count, GLenum type, bool normalized) :
        location(location), offset(offset), stride(stride), count(count), type(type), normalized(normalized)
    {}
    GLint      location;
    GLsizeiptr offset;
    GLsizei    stride;
    GLint      count;
    GLenum     type;
    bool       normalized;
};

void VertexObject::enableAttribArrays() const noexcept
{
    for (const auto& p : m_attribParams)
    {
        glEnableVertexAttribArray(p.location);
        glVertexAttribPointer(p.location, p.count, p.type, p.normalized, p.stride, (GLvoid*) p.offset);
    }
}

void VertexObject::disableAttribArrays() const noexcept
{
    for (const auto& p : m_attribParams)
        glDisableVertexAttribArray(p.location);
}

void
VertexObject::disableVertexAttribArray(GLint location) const noexcept
{
    glDisableVertexAttribArray(location);
}

void
VertexObject::setVertexAttribConstant(GLint location, float value) const noexcept
{
    disableVertexAttribArray(location);
    glVertexAttrib1f(location, value);
}

void
VertexObject::enableVertexAttribArray(GLint location) const noexcept
{
    glEnableVertexAttribArray(location);
}

void VertexObject::setVertexAttribArray(GLint location, GLint count, GLenum type, bool normalized, GLsizei stride, GLsizeiptr offset)
{
    assert(location >= 0);
    m_attribParams.emplace_back(location, offset, stride, count, type, normalized);
}

IndexedVertexObject::IndexedVertexObject(GLenum indexType) :
    VertexObject(),
    m_indexType(indexType)
{
}

IndexedVertexObject::IndexedVertexObject(GLsizeiptr bufferSize, GLenum streamType, GLenum indexType, GLsizeiptr indexSize) :
    VertexObject(bufferSize, streamType),
    m_indexType(indexType),
    m_indexStreamType(streamType),
    m_indexSize(indexSize)
{
}

IndexedVertexObject::IndexedVertexObject(GLsizeiptr bufferSize, GLenum streamType, GLenum indexType, GLsizeiptr indexSize, GLenum indexStreamType) :
    VertexObject(bufferSize, streamType),
    m_indexType(indexType),
    m_indexStreamType(indexStreamType),
    m_indexSize(indexSize)
{
}

IndexedVertexObject::~IndexedVertexObject()
{
    if (m_vioId != 0)
        glDeleteBuffers(1, &m_vioId);
}

void
IndexedVertexObject::bindWritable() noexcept
{
    m_state |= State::Update;
    bind();
}

void
IndexedVertexObject::bind() noexcept
{
    VertexObject::bind();

    if ((m_state & State::Initialize) != 0)
    {
        glGenBuffers(1, &m_vioId);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vioId);
    }

    // we can have streaming/dynamic VBO and static VIO so we have an additional check
    if (!isVAOSupported() ||
        ((m_state & State::Update) != 0 && m_indexStreamType != GL_STATIC_DRAW))
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vioId);
    }
}

void
IndexedVertexObject::unbind() noexcept
{
    VertexObject::unbind();
    if ((m_state & (State::Initialize | State::Update)) != 0)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void
IndexedVertexObject::draw(GLenum primitive, GLsizei count, GLint first) const noexcept
{
    if ((m_state & State::Initialize) != 0)
        enableAttribArrays();

    auto offset = first * (m_indexType == GL_UNSIGNED_INT ? sizeof(GLuint) : sizeof(GLushort));
    glDrawElements(primitive, count, m_indexType,
                   reinterpret_cast<const void*>(static_cast<std::intptr_t>(offset))); //NOSONAR
}

void
IndexedVertexObject::allocate(const void* data, const void* indices) const noexcept
{
    VertexObject::allocate(data);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indexSize, indices, m_indexStreamType);
}

void
IndexedVertexObject::setIndexBufferData(const void* data, GLintptr offset, GLsizeiptr size) const noexcept
{
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, size, data);
}

} // namespace
