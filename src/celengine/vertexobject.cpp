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

#include "vertexobject.h"

namespace celgl
{
VertexObject::VertexObject(GLenum bufferType) :
    m_bufferType(bufferType)
{
}

VertexObject::VertexObject(GLenum bufferType, GLsizeiptr bufferSize, GLenum streamType) :
    m_bufferType(bufferType),
    m_bufferSize(bufferSize),
    m_streamType(streamType)
{
}

VertexObject::~VertexObject()
{
    delete m_attribParams;

    if (GLEW_ARB_vertex_array_object)
        glDeleteVertexArrays(1, &m_vaoId);

    glDeleteBuffers(1, &m_vboId);
}

void VertexObject::bind()
{
    if (m_firstBind)
    {
        if (GLEW_ARB_vertex_array_object)
        {
            glGenVertexArrays(1, &m_vaoId);
            glBindVertexArray(m_vaoId);
        }
        glGenBuffers(1, &m_vboId);
        glBindBuffer(m_bufferType, m_vboId);
    }
    else
    {
        if (GLEW_ARB_vertex_array_object)
        {
            glBindVertexArray(m_vaoId);
        }
        else
        {
            glBindBuffer(m_bufferType, m_vboId);
            enableAttribArrays();
        }
    }
}

void VertexObject::unbind()
{
    if (GLEW_ARB_vertex_array_object)
    {
        if (m_firstBind)
            glBindBuffer(m_bufferType, 0);
        glBindVertexArray(0);
    }
    else
    {
        disableAttribArrays();
        glBindBuffer(m_bufferType, 0);
    }
    m_firstBind = false;
}

bool VertexObject::allocate(const void* data)
{
    glBufferData(m_bufferType, m_bufferSize, data, m_streamType);
    return glGetError() != GL_NO_ERROR;
}

bool VertexObject::allocate(GLsizeiptr bufferSize, const void* data)
{
    m_bufferSize = bufferSize;
    return allocate(data);
}

bool VertexObject::allocate(GLenum bufferType, GLsizeiptr bufferSize, const void* data, GLenum streamType)
{
    m_bufferType = bufferType;
    m_bufferSize = bufferSize;
    m_streamType = streamType;
    return allocate(data);
}

bool VertexObject::setBufferData(const void* data, GLintptr offset, GLsizeiptr size)
{
    glBufferSubData(m_bufferType, offset, size == 0 ? m_bufferSize : size, data);
    return glGetError() != GL_NO_ERROR;
}

void VertexObject::draw(GLenum primitive, GLsizei count, GLint first)
{
    if (m_firstBind)
        enableAttribArrays();

    glDrawArrays(primitive, first, count);
}

void VertexObject::enableAttribArrays()
{
     glBindBuffer(m_bufferType, m_vboId);

     if (m_attrIndexes & AttrType::Vertices)
     {
        const auto& p = m_params[0];
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(p.count, p.type, p.stride, (GLvoid*) p.offset);
    }

    if (m_attrIndexes & AttrType::Normal)
    {
        const auto& p = m_params[1];
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(p.type, p.stride, (GLvoid*) p.offset);
    }

    if (m_attrIndexes & AttrType::Color)
    {
        const auto& p = m_params[2];
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(p.count, p.type, p.stride, (GLvoid*) p.offset);
    }

    if (m_attrIndexes & AttrType::Index)
    {
       const auto& p = m_params[3];
       glEnableClientState(GL_INDEX_ARRAY);
       glIndexPointer(p.type, p.stride, (GLvoid*) p.offset);
    }

    if (m_attrIndexes & AttrType::Texture)
    {
        const auto& p = m_params[4];
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(p.count, p.type, p.stride, (GLvoid*) p.offset);
    }

    if (m_attrIndexes & AttrType::EdgeFlag)
    {
       const auto& p = m_params[5];
       glEnableClientState(GL_EDGE_FLAG_ARRAY);
       glEdgeFlagPointer(p.stride, (GLvoid*) p.offset);
    }

    if (m_attrIndexes & AttrType::Tangent)
    {
        const auto& p = m_params[6];
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, p.count, p.type, p.normalized, p.stride, (GLvoid*) p.offset);
    }

    if (m_attrIndexes & AttrType::PointSize)
    {
        const auto& p = m_params[7];
        glEnableVertexAttribArray(7);
        glVertexAttribPointer(7, p.count, p.type, p.normalized, p.stride, (GLvoid*) p.offset);
    }

    if (m_attribParams != nullptr)
    {
        for (const auto& t : *m_attribParams)
        {
            auto  n = t.first;
            auto& p = t.second;
            glEnableVertexAttribArray(n);
            glVertexAttribPointer(n, p.count, p.type, p.normalized, p.stride, (GLvoid*) p.offset);
        }
    }
}

void VertexObject::disableAttribArrays()
{
    if (m_attrIndexes & AttrType::Vertices)
        glDisableClientState(GL_VERTEX_ARRAY);

    if (m_attrIndexes & AttrType::Normal)
        glDisableClientState(GL_NORMAL_ARRAY);

    if (m_attrIndexes & AttrType::Color)
        glDisableClientState(GL_COLOR_ARRAY);

    if (m_attrIndexes & AttrType::Index)
        glDisableClientState(GL_INDEX_ARRAY);

    if (m_attrIndexes & AttrType::Texture)
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    if (m_attrIndexes & AttrType::EdgeFlag)
        glDisableClientState(GL_EDGE_FLAG_ARRAY);

    if (m_attrIndexes & AttrType::Tangent)
        glDisableVertexAttribArray(6);

    if (m_attrIndexes & AttrType::PointSize)
        glDisableVertexAttribArray(7);

    if (m_attribParams != nullptr)
    {
        for (const auto& t : *m_attribParams)
            glDisableVertexAttribArray(t.first);
    }

    glBindBuffer(m_bufferType, 0);
}

void VertexObject::setVertices(GLint count, GLenum type, bool normalized, GLsizei stride, GLsizeiptr offset)
{
    m_attrIndexes |= AttrType::Vertices;
    m_params[0] = { offset, stride, count, type, normalized };
}

void VertexObject::setNormals(GLint count, GLenum type, bool normalized, GLsizei stride, GLsizeiptr offset)
{
    m_attrIndexes |= AttrType::Normal;
    m_params[1] = { offset, stride, count, type, normalized };
}

void VertexObject::setColors(GLint count, GLenum type, bool normalized, GLsizei stride, GLsizeiptr offset)
{
    m_attrIndexes |= AttrType::Color;
    m_params[2] = { offset, stride, count, type, normalized };
}

void VertexObject::setIndexes(GLint count, GLenum type, bool normalized, GLsizei stride, GLsizeiptr offset)
{
    m_attrIndexes |= AttrType::Index;
    m_params[3] = { offset, stride, count, type, normalized };
}

void VertexObject::setTextureCoords(GLint count, GLenum type, bool normalized, GLsizei stride, GLsizeiptr offset)
{
    m_attrIndexes |= AttrType::Texture;
    m_params[4] = { offset, stride, count, type, normalized };
}

void VertexObject::setEdgeFlags(GLint count, GLenum type, bool normalized, GLsizei stride, GLsizeiptr offset)
{
    m_attrIndexes |= AttrType::EdgeFlag;
    m_params[5] = { offset, stride, count, type, normalized };
}

void VertexObject::setTangents(GLint count, GLenum type, bool normalized, GLsizei stride, GLsizeiptr offset)
{
    m_attrIndexes |= AttrType::Tangent;
    m_params[6] = { offset, stride, count, type, normalized };
}

void VertexObject::setPointSizes(GLint count, GLenum type, bool normalized, GLsizei stride, GLsizeiptr offset)
{
    m_attrIndexes |= AttrType::PointSize;
    m_params[7] = { offset, stride, count, type, normalized };
}

void VertexObject::setVertexAttrib(GLint location, GLint count, GLenum type, bool normalized, GLsizei stride, GLsizeiptr offset)
{
    if (m_attribParams == nullptr)
        m_attribParams = new std::map<GLint, PtrParams>;

    PtrParams p = { offset, stride, count, type, normalized };
    (*m_attribParams)[location] = p;
}
}; // namespace
