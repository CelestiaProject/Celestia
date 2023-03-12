// vertexobject.h
//
// Copyright (C) 2023-present, Celestia Development Team.
//
// VAO wrapper.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "buffer.h"
#include "vertexobject.h"

#define GLENUM(p) (static_cast<GLenum>(p))
#define PTR(p) (reinterpret_cast<void*>(p))

namespace
{

[[ nodiscard ]] inline bool
isVAOSupported()
{
#ifdef GL_ES
    return celestia::gl::OES_vertex_array_object;
#else
    return celestia::gl::ARB_vertex_array_object;
#endif
}

} // anonymous namespace

namespace celestia::gl
{

VertexObject::VertexObject(util::NoCreateT)
{
}

VertexObject::VertexObject(VertexObject::Primitive primitive) :
    m_primitive(primitive)
{
    if (isVAOSupported())
        glGenVertexArrays(1, &m_id);
}

VertexObject::VertexObject(VertexObject &&other) noexcept :
    m_bufferDesc(std::move(other.m_bufferDesc)),
    m_primitive(other.m_primitive),
    m_count(other.m_count),
    m_id(other.m_id),
    m_idxBufferId(other.m_idxBufferId),
    m_indexType(other.m_indexType),
    m_currBuff(other.m_currBuff),
    m_initialized(other.m_initialized)
{
    other.m_primitive = Primitive::Triangles;
    other.m_count = 0;
    other.m_id = 0;
    other.m_idxBufferId = 0;
    other.m_indexType = IndexType::UnsignedShort;
    other.m_currBuff = 0;
    other.m_initialized = false;
}

VertexObject& VertexObject::operator=(VertexObject &&other) noexcept
{
    clear();

    m_bufferDesc = std::move(other.m_bufferDesc);
    m_primitive = other.m_primitive;
    m_count = other.m_count;
    m_id = other.m_id;
    m_idxBufferId = other.m_idxBufferId;
    m_indexType = other.m_indexType;
    m_currBuff = other.m_currBuff;
    m_initialized = other.m_initialized;

    other.m_primitive = Primitive::Triangles;
    other.m_count = 0;
    other.m_id = 0;
    other.m_idxBufferId = 0;
    other.m_indexType = IndexType::UnsignedShort;
    other.m_currBuff = 0;
    other.m_initialized = false;

    return *this;
}

VertexObject::~VertexObject()
{
    clear();
}

VertexObject&
VertexObject::setCount(int count)
{
    m_count = count;
    return *this;
}

VertexObject&
VertexObject::setPrimitive(VertexObject::Primitive primitive)
{
    m_primitive = primitive;
    return *this;
}

void
VertexObject::clear() noexcept
{
    if (m_id != 0 && isVAOSupported())
        glDeleteVertexArrays(1, &m_id);
    m_id = 0;
}

struct VertexObject::BufferDesc
{
    BufferDesc(
        GLsizeiptr    offset,
        GLuint        bufferId,
        std::uint16_t type,
        std::int16_t  location,
        std::uint8_t  elemSize,
        std::uint8_t  stride,
        bool          normalized) :
        offset(offset),
        bufferId(bufferId),
        type(type),
        location(location),
        elemSize(elemSize),
        stride(stride),
        normalized(normalized)
    {
    }
    GLsizeiptr      offset;
    GLuint          bufferId;
    std::uint16_t   type;           // all constants < 0xFFFF
    std::int16_t    location;
    std::uint8_t    elemSize;       // 1, 2, 3, 4
    std::uint8_t    stride;         // WebGL allows only 255 bytes max
    bool            normalized;
};

VertexObject&
VertexObject::addVertexBuffer(const Buffer &buffer, int location, int elemSize, VertexObject::DataType type, bool normalized, int stride, std::ptrdiff_t offset)
{
    if (buffer.targetHint() != Buffer::TargetHint::Array)
        return *this;

    m_bufferDesc.emplace_back(
        offset,
        buffer.id(),
        static_cast<std::uint16_t>(type),
        static_cast<std::uint16_t>(location),
        static_cast<std::uint8_t>(elemSize),
        static_cast<std::uint8_t>(stride),
        normalized
    );

    return *this;
}

VertexObject&
VertexObject::draw()
{
    return draw(m_primitive, m_count, 0);
}

VertexObject&
VertexObject::draw(int count, int first)
{
    return draw(m_primitive, count, first);
}

VertexObject&
VertexObject::draw(VertexObject::Primitive primitive, int count, int first)
{
    if (count == 0)
        return *this;

    bind();

    if (isIndexed())
    {
        auto offset = static_cast<std::ptrdiff_t>(first * (m_indexType == IndexType::UnsignedShort ? sizeof(GLushort) : sizeof(GLuint)));
        glDrawElements(GLENUM(primitive), count, GLENUM(m_indexType), PTR(offset));
    }
    else
    {
        glDrawArrays(GLENUM(primitive), first, count);
    }

    unbind();

    return *this;
}

VertexObject&
VertexObject::setIndexBuffer(const Buffer &buffer, std::ptrdiff_t /*offset*/, VertexObject::IndexType type)
{
    if (buffer.targetHint() != Buffer::TargetHint::ElementArray)
        return *this;

    m_idxBufferId = buffer.id();
    m_indexType = type;

    return *this;
}

void
VertexObject::enableAttribArrays()
{
    m_currBuff = 0; // other buffer can be mounted externally
    for (const auto& p : m_bufferDesc)
    {
        if (m_currBuff != p.bufferId)
        {
            glBindBuffer(GL_ARRAY_BUFFER, p.bufferId);
            m_currBuff = p.bufferId;
        }
        glEnableVertexAttribArray(p.location);
        glVertexAttribPointer(p.location, p.elemSize, p.type, p.normalized ? GL_TRUE : GL_FALSE, p.stride, PTR(p.offset));
    }

    if (isIndexed())
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_idxBufferId);
}

void
VertexObject::disableAttribArrays()
{
    for (const auto& p : m_bufferDesc)
        glDisableVertexAttribArray(p.location);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (isIndexed())
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    m_currBuff = 0;
}

void
VertexObject::bind()
{
    if (!m_initialized)
    {
        m_initialized = true;
        if (isVAOSupported())
        {
            glBindVertexArray(m_id);
            enableAttribArrays();
            m_bufferDesc.clear();
        }
    }
    else
    {
        if (isVAOSupported())
            glBindVertexArray(m_id);
        else
            enableAttribArrays();
    }
}

void
VertexObject::unbind()
{
    if (isVAOSupported())
        glBindVertexArray(0);
    else
        disableAttribArrays();
}

} // namespace celestia::gl
