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

#include <GL/glew.h>
#include <array>

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

    void bind();
    void unbind();
    void draw(GLenum primitive, GLsizei count, GLint first = 0);
    bool allocate(const void* data = nullptr);
    bool allocate(GLsizeiptr bufferSize, const void* data = nullptr);
    bool allocate(GLenum bufferType, GLsizeiptr bufferSize, const void* data, GLenum streamType);
    bool setBufferData(const void* data, GLintptr offset = 0, GLsizeiptr size = 0);
    void setVertices(GLint count, GLenum type, bool normalized = false, GLsizei stride = 0, GLsizeiptr offset = 0);
    void setNormals(GLint count, GLenum type, bool normalized = false, GLsizei stride = 0, GLsizeiptr offset = 0);
    void setColors(GLint count, GLenum type, bool normalized = false, GLsizei stride = 0, GLsizeiptr offset = 0);
    void setIndexes(GLint count, GLenum type, bool normalized = false, GLsizei stride = 0, GLsizeiptr offset = 0);
    void setTextureCoords(GLint count, GLenum type, bool normalized = false, GLsizei stride = 0, GLsizeiptr offset = 0);
    void setEdgeFlags(GLint count, GLenum type, bool normalized = false, GLsizei stride = 0, GLsizeiptr offset = 0);
    void setTangents(GLint count, GLenum type, bool normalized = false, GLsizei stride = 0, GLsizeiptr offset = 0);
    void setPointSizes(GLint count, GLenum type, bool normalized = false, GLsizei stride = 0, GLsizeiptr offset = 0);
    bool initialized() const { return !m_firstBind; }
    void setBufferType(GLenum bufferType)     { m_bufferType = bufferType; }
    void setBufferSize(GLsizeiptr bufferSize) { m_bufferSize = bufferSize; }
    void setStreamType(GLenum streamType)     { m_streamType = streamType; }

 private:
    enum AttrType
    {
        Vertices  = 0x00000001,
        Normal    = 0x00000002,
        Color     = 0x00000004,
        Index     = 0x00000008,
        Texture   = 0x00000010,
        EdgeFlag  = 0x00000020,
        Tangent   = 0x00000040,
        PointSize = 0x00000080
    };

    struct PtrParams
    {
        GLsizeiptr offset;
        GLsizei    stride;
        GLint      count;
        GLenum     type;
        bool       normalized;
    };

    void enableAttribArrays();
    void disableAttribArrays();

    GLuint     m_vboId{ 0 };
    GLuint     m_vaoId{ 0 };
    uint16_t   m_attrIndexes{ 0 };

    GLsizeiptr m_bufferSize{ 0 };
    GLenum     m_bufferType{ 0 };
    GLenum     m_streamType{ 0 };
    std::array<PtrParams, 8> m_params {};

    bool       m_firstBind{ true };
};
}; // namespace
