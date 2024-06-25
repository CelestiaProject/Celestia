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

#include "binder.h"
#include "buffer.h"
#include "vertexobject.h"

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
    m_indexBuffer(std::move(other.m_indexBuffer)),
    m_indexType(other.m_indexType),
    m_primitive(other.m_primitive),
    m_count(other.m_count),
    m_id(other.m_id),
    m_initialized(other.m_initialized)
{
    other.clear();
}

VertexObject &VertexObject::operator=(VertexObject &&other) noexcept
{
    destroy();

    m_bufferDesc = std::move(other.m_bufferDesc);
    m_indexBuffer = std::move(other.m_indexBuffer);
    m_indexType = other.m_indexType;
    m_primitive = other.m_primitive;
    m_count = other.m_count;
    m_id = other.m_id;
    m_initialized = other.m_initialized;

    other.clear();

    return *this;
}

VertexObject::~VertexObject()
{
    destroy();
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
VertexObject::destroy() noexcept
{
    unbind();
    if (m_id != 0 && isVAOSupported())
        glDeleteVertexArrays(1, &m_id);
    m_id = 0;
}

void
VertexObject::clear()
{
    m_primitive = Primitive::Triangles;
    m_count = 0;
    m_id = 0;
    m_indexType = IndexType::UnsignedShort;
    m_initialized = false;
}

struct VertexObject::BufferDesc
{
    BufferDesc(GLsizeiptr    offset,
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
    GLsizeiptr    offset;
    GLuint        bufferId;
    std::uint16_t type;       // all constants < 0xFFFF
    std::int16_t  location;
    std::uint8_t  elemSize;   // 1, 2, 3, 4
    std::uint8_t  stride;     // WebGL allows only 255 bytes max
    bool          normalized;
};

VertexObject&
VertexObject::addVertexBuffer(const Buffer &buffer, int location, int elemSize, VertexObject::DataType type, bool normalized, int stride, std::ptrdiff_t offset)
{
    if (buffer.targetHint() != Buffer::TargetHint::Array)
        return *this;

    m_bufferDesc.emplace_back(offset,
                              buffer.id(),
                              static_cast<std::uint16_t>(type),
                              static_cast<std::uint16_t>(location),
                              static_cast<std::uint8_t>(elemSize),
                              static_cast<std::uint8_t>(stride),
                              normalized);

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
        glDrawElements(GLenum(primitive), count, GLenum(m_indexType), PTR(offset));
    }
    else
    {
        glDrawArrays(GLenum(primitive), first, count);
    }

    unbind();

    return *this;
}

VertexObject&
VertexObject::setIndexBuffer(const Buffer &buffer, std::ptrdiff_t /*offset*/, VertexObject::IndexType type)
{
    if (buffer.targetHint() != Buffer::TargetHint::ElementArray)
        return *this;

    m_indexBuffer = Buffer::wrap(buffer.id(), Buffer::TargetHint::ElementArray);
    m_indexType = type;

    return *this;
}

VertexObject&
VertexObject::setIndexBuffer(Buffer &&buffer, std::ptrdiff_t /*offset*/, VertexObject::IndexType type)
{
    if (buffer.targetHint() != Buffer::TargetHint::ElementArray)
        return *this;

    m_indexBuffer = std::move(buffer);
    m_indexType = type;

    return *this;
}

void
VertexObject::enableAttribArrays() const
{
    auto &binder = Binder::get();

    for (const auto &p : m_bufferDesc)
    {
        binder.bind(Buffer::wrap(p.bufferId, Buffer::TargetHint::Array));

        glEnableVertexAttribArray(p.location);
        glVertexAttribPointer(p.location, p.elemSize, p.type, p.normalized ? GL_TRUE : GL_FALSE, p.stride, PTR(p.offset));
    }

    if (isIndexed())
        binder.bind(m_indexBuffer);
}

void
VertexObject::disableAttribArrays() const
{
    auto &binder = Binder::get();

    for (const auto &p : m_bufferDesc)
        glDisableVertexAttribArray(p.location);

    binder.unbind(Buffer::TargetHint::Array);

    if (isIndexed())
        binder.unbind(Buffer::TargetHint::ElementArray);
}

void
VertexObject::bind()
{
    if (isVAOSupported())
        Binder::get().bind(*this);
    else
        enableAttribArrays();

    if (!m_initialized)
    {
        m_initialized = true;
        if (isVAOSupported())
        {
            enableAttribArrays();
            m_bufferDesc.clear();
        }
    }
}

void
VertexObject::unbind() const noexcept
{
    if (isVAOSupported())
        Binder::get().unbind(*this);
    else
        disableAttribArrays();
}

} // namespace celestia::gl
