// vertexobject.h
//
// Copyright (C) 2019, the Celestia Development Team
//
// VBO/VAO wrappper class. Currently GL2/GL2+VAO only.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include "glsupport.h"
#include <array>
#include <map>

namespace celgl
{
class VertexObject
{
 public:
    VertexObject(const VertexObject&) = delete;
    VertexObject(VertexObject&&) = delete;
    VertexObject operator=(const VertexObject&) = delete;
    VertexObject operator=(VertexObject&&) = delete;

    VertexObject() = default;
    VertexObject(GLenum bufferType);
    VertexObject(GLenum bufferType, GLsizeiptr bufferSize, GLenum streamType);
    ~VertexObject();

    enum class AttributesType
    {
        Default      = 0,
        Alternative1 = 1,
        Count        = 2,
        Invalid      = -1,
    };

    void bind(AttributesType attributes = AttributesType::Default) noexcept;
    void bindWritable(AttributesType attributes = AttributesType::Default) noexcept;
    void unbind() noexcept;
    void draw(GLenum primitive, GLsizei count, GLint first = 0) noexcept;
    bool allocate(const void* data = nullptr) noexcept;
    bool allocate(GLsizeiptr bufferSize, const void* data = nullptr) noexcept;
    bool allocate(GLenum bufferType, GLsizeiptr bufferSize, const void* data, GLenum streamType) noexcept;
    bool setBufferData(const void* data, GLintptr offset = 0, GLsizeiptr size = 0) noexcept;
    void setVertices(GLint count, GLenum type, bool normalized = false, GLsizei stride = 0, GLsizeiptr offset = 0, AttributesType attributes = AttributesType::Default) noexcept;
    void setNormals(GLint count, GLenum type, bool normalized = false, GLsizei stride = 0, GLsizeiptr offset = 0, AttributesType attributes = AttributesType::Default) noexcept;
    void setColors(GLint count, GLenum type, bool normalized = false, GLsizei stride = 0, GLsizeiptr offset = 0, AttributesType attributes = AttributesType::Default) noexcept;
    void setTextureCoords(GLint count, GLenum type, bool normalized = false, GLsizei stride = 0, GLsizeiptr offset = 0, AttributesType attributes = AttributesType::Default) noexcept;
    void setTangents(GLint count, GLenum type, bool normalized = false, GLsizei stride = 0, GLsizeiptr offset = 0, AttributesType attributes = AttributesType::Default) noexcept;
    void setPointSizes(GLint count, GLenum type, bool normalized = false, GLsizei stride = 0, GLsizeiptr offset = 0, AttributesType attributes = AttributesType::Default) noexcept;
    void setVertexAttribArray(GLint location, GLint count, GLenum type, bool normalized = false, GLsizei stride = 0, GLsizeiptr offset = 0, AttributesType attributes = AttributesType::Default);
    inline bool initialized() const noexcept
    {
        return (m_state & State::Initialize) == 0;
    }
    void setBufferType(GLenum bufferType) noexcept     { m_bufferType = bufferType; }
    void setBufferSize(GLsizeiptr bufferSize) noexcept { m_bufferSize = bufferSize; }
    void setStreamType(GLenum streamType) noexcept     { m_streamType = streamType; }

 private:
    inline bool isVAOSupported() const
    {
#ifndef GL_ES
        return celestia::gl::ARB_vertex_array_object;
#else
        return celestia::gl::OES_vertex_array_object;
#endif
    }

    enum State : uint16_t
    {
        NormalState = 0x0000,
        Initialize  = 0x0001,
        Update      = 0x0002
    };

    struct PtrParams
    {
        GLsizeiptr offset;
        GLsizei    stride;
        GLint      count;
        GLenum     type;
        bool       normalized;
    };

    void enableAttribArrays() noexcept;
    void disableAttribArrays() noexcept;

    GLuint     m_vboId{ 0 };
    GLuint     m_vaoId{ 0 };
    uint16_t   m_state{ State::Initialize };

    GLsizeiptr m_bufferSize{ 0 };
    GLenum     m_bufferType{ 0 };
    GLenum     m_streamType{ 0 };

    AttributesType m_currentAttributes { AttributesType::Invalid };
    std::array<std::map<GLint, PtrParams>, (unsigned int)AttributesType::Count> m_attribParams;
};
} // namespace
