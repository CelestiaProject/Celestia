// buffer.cpp
//
// Copyright (C) 2023-present, Celestia Development Team.
//
// VBO wrapper.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "buffer.h"

#include "binder.h"

namespace celestia::gl
{

Buffer::Buffer(GLuint id, Buffer::TargetHint targetHint) :
    m_id(id),
    m_targetHint(targetHint)
{
}

Buffer::~Buffer()
{
    destroy();
}

void
Buffer::destroy() noexcept
{
    if (m_id != 0)
    {
        unbind(); // bind operations for wrapped buffers are performed externally
        glDeleteBuffers(1, &m_id);
    }
    m_id = 0;
}

void
Buffer::clear()
{
    m_bufferSize = 0;
    m_id         = 0;
    m_targetHint = TargetHint::Array;
    m_usage      = BufferUsage::StaticDraw;
}

Buffer&
Buffer::bind()
{
    Binder::get().bind(*this);
    return *this;
}

void
Buffer::unbind() const
{
    Binder::get().unbind(*this);
}

void
Buffer::unbind(Buffer::TargetHint target)
{
    Binder::get().unbind(target);
}

Buffer&
Buffer::setData(util::array_view<void> data, Buffer::BufferUsage usage)
{
    setData(data.data(), static_cast<GLsizeiptr>(data.size()), usage);
    return *this;
}

void
Buffer::setData(const void* data, GLsizeiptr size, Buffer::BufferUsage usage) //NOSONAR
{
    m_bufferSize = size;
    m_usage = usage;
    Binder::get().bind(*this);
    glBufferData(static_cast<GLenum>(m_targetHint),
                 size,
                 data,
                 static_cast<GLenum>(m_usage));
}

Buffer&
Buffer::setSubData(GLintptr offset, util::array_view<void> data)
{
    Binder::get().bind(*this);
    glBufferSubData(static_cast<GLenum>(m_targetHint), offset, data.size(), data.data());
    return *this;
}

Buffer&
Buffer::invalidateData()
{
    setData(nullptr, m_bufferSize, m_usage);
    return *this;
}

Buffer::SharedPtr
Buffer::create(TargetHint targetHint)
{
    GLuint id;
    glGenBuffers(1, &id);
    return Buffer::SharedPtr(new Buffer(id, targetHint), false);
}

Buffer::SharedPtr
Buffer::create(TargetHint targetHint,
               GLsizeiptr size,
               BufferUsage usage)
{
    auto buffer = create(targetHint);
    if (buffer)
        buffer->setData(nullptr, size, usage);

    return buffer;
}

Buffer::SharedPtr
Buffer::create(TargetHint targetHint,
               util::array_view<void> data,
               BufferUsage usage)
{
    auto buffer = create(targetHint);
    if (buffer)
        buffer->setData(data, usage);

    return buffer;
}

} // namespace celestia::gl
