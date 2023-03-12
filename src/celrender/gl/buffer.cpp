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

#define GLENUM(p) (static_cast<GLenum>(p))

namespace celestia::gl
{

Buffer::Buffer(util::NoCreateT)
{
}

Buffer::Buffer(Buffer::TargetHint targetHint) :
    m_targetHint(targetHint)
{
    glGenBuffers(1, &m_id);
}

Buffer::Buffer(Buffer::TargetHint targetHint, util::array_view<const void> data, Buffer::BufferUsage usage) :
    Buffer(targetHint)
{
    bind();
    setData(data, usage);
}

Buffer::Buffer(Buffer &&other) noexcept :
    m_bufferSize(other.m_bufferSize),
    m_id(other.m_id),
    m_targetHint(other.m_targetHint),
    m_usage(other.m_usage)
{
    other.m_bufferSize = 0;
    other.m_id         = 0;
    other.m_targetHint = TargetHint::Array;
    other.m_usage      = BufferUsage::StaticDraw;
}

Buffer& Buffer::operator=(Buffer &&other) noexcept
{
    clear();

    m_bufferSize = other.m_bufferSize;
    m_id         = other.m_id;
    m_targetHint = other.m_targetHint;
    m_usage      = other.m_usage;

    other.m_bufferSize = 0;
    other.m_id         = 0;
    other.m_targetHint = TargetHint::Array;
    other.m_usage      = BufferUsage::StaticDraw;

    return *this;
}

Buffer::~Buffer()
{
    clear();
}

void
Buffer::clear() noexcept
{
    if (m_id != 0)
        glDeleteBuffers(1, &m_id);
    m_id = 0;
}

Buffer&
Buffer::bind()
{
    glBindBuffer(GLENUM(m_targetHint), m_id);
    return *this;
}

void
Buffer::unbind() const
{
    glBindBuffer(GLENUM(m_targetHint), 0);
}

void
Buffer::unbind(Buffer::TargetHint target)
{
    glBindBuffer(GLENUM(target), 0);
}

Buffer&
Buffer::setData(util::array_view<const void> data, Buffer::BufferUsage usage)
{
    m_bufferSize = data.size();
    m_usage      = usage;
    glBufferData(GLENUM(m_targetHint), m_bufferSize, data.data(), GLENUM(m_usage));
    return *this;
}

Buffer&
Buffer::setSubData(GLintptr offset, util::array_view<const void> data)
{
    glBufferSubData(GLENUM(m_targetHint), offset, data.size(), data.data());
    return *this;
}


Buffer&
Buffer::invalidateData()
{
    glBufferData(GLENUM(m_targetHint), m_bufferSize, nullptr, GLENUM(m_usage));
    return *this;
}

Buffer&
Buffer::setTargetHint(Buffer::TargetHint targetHint)
{
    m_targetHint = targetHint;
    return *this;
}

} // namespace celestia::gl
