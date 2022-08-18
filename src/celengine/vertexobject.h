// vertexobject.h
//
// Copyright (C) 2019-present, the Celestia Development Team
//
// VBO/VAO wrappper class. Currently GL2/GL2+VAO only.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include "glsupport.h"
#include <vector>

namespace celgl
{
class VertexObject
{
 public:
    VertexObject(const VertexObject&) = delete;
    VertexObject(VertexObject&&) = delete;
    VertexObject& operator=(const VertexObject&) = delete;
    VertexObject& operator=(VertexObject&&) = delete;

    VertexObject() = default;
    VertexObject(GLenum bufferType);
    VertexObject(GLenum bufferType, GLsizeiptr bufferSize, GLenum streamType);
    ~VertexObject();

    void bind() noexcept;
    void bindWritable() noexcept;
    void unbind() noexcept;
    void draw(GLenum primitive, GLsizei count, GLint first = 0) const noexcept;
    void allocate(const void* data = nullptr) const noexcept;
    void allocate(GLsizeiptr bufferSize, const void* data = nullptr) noexcept;
    void allocate(GLenum bufferType, GLsizeiptr bufferSize, const void* data, GLenum streamType) noexcept;
    void setBufferData(const void* data, GLintptr offset = 0, GLsizeiptr size = 0) const noexcept;
    void setVertexAttribArray(GLint location, GLint count, GLenum type, bool normalized = false, GLsizei stride = 0, GLsizeiptr offset = 0);
    inline bool initialized() const noexcept
    {
        return (m_state & State::Initialize) == 0;
    }
    GLenum getBufferType() const noexcept              { return m_bufferType; }
    void setBufferType(GLenum bufferType) noexcept     { m_bufferType = bufferType; }
    GLsizeiptr getBufferSize() const noexcept          { return m_bufferSize; }
    void setBufferSize(GLsizeiptr bufferSize) noexcept { m_bufferSize = bufferSize; }
    GLenum getStreamType() const noexcept              { return m_streamType; }
    void setStreamType(GLenum streamType) noexcept     { m_streamType = streamType; }

 private:
    enum State : uint16_t
    {
        NormalState = 0x0000,
        Initialize  = 0x0001,
        Update      = 0x0002
    };

    struct PtrParams;

    void enableAttribArrays() const noexcept;
    void disableAttribArrays() const noexcept;

    std::vector<PtrParams> m_attribParams;

    GLuint     m_vboId{ 0 };
    GLuint     m_vaoId{ 0 };

    GLsizeiptr m_bufferSize{ 0 };
    GLenum     m_bufferType{ 0 };
    GLenum     m_streamType{ 0 };

    uint16_t   m_state{ State::Initialize };
};
} // namespace
