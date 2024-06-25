// binder.cpp
//
// Copyright (C) 2023-present, Celestia Development Team.
//
// VAO wrapper.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
#include "binder.h"

namespace celestia::gl
{

Binder&
Binder::get()
{
    static Binder binder;
    return binder;
}

Binder&
Binder::bind(const Buffer &bo)
{
    return bindVBO(bo.targetHint(), bo.id());
}

Binder&
Binder::unbind(const Buffer &bo)
{
    switch (bo.targetHint())
    {
    case Buffer::TargetHint::Array:
        if (m_boundVbo == bo.id())
            return bindVBO(Buffer::TargetHint::Array, 0u);
    case Buffer::TargetHint::ElementArray:
        if (m_boundIbo == bo.id())
            return bindVBO(Buffer::TargetHint::ElementArray, 0u);
    default:
        break;
    }
    return *this;
}

template<> Binder&
Binder::unbind<Buffer>()
{
    return bindVBO(Buffer::TargetHint::Array, 0u);
}

Binder &
Binder::unbind(Buffer::TargetHint target)
{
    return bindVBO(target, 0u);
}

Binder&
Binder::bind(const VertexObject &vo)
{
    return bindVAO(vo.id());
}

Binder&
Binder::unbind(const VertexObject &vo)
{
    if (m_boundVao == vo.id())
        return bindVAO(0u);
    return *this;
}

template<> Binder&
Binder::unbind<VertexObject>()
{
    return bindVAO(0u);
}

Binder&
Binder::bindVAO(GLuint id)
{
    if (m_boundVao != id)
    {
        glBindVertexArray(id);
        m_boundVao = id;
        m_boundVbo = 0u;
        m_boundIbo = 0u;
    }
    return *this;
}

Binder&
Binder::bindVBO(Buffer::TargetHint target, GLuint id)
{
    glBindBuffer(static_cast<GLenum>(target), id);
    switch (target)
    {
    case Buffer::TargetHint::Array:
        if (m_boundVbo != id)
        {
            m_boundVbo = id;
            glBindBuffer(GL_ARRAY_BUFFER, id);
        }
        break;
    case Buffer::TargetHint::ElementArray:
        if (m_boundIbo != id)
        {
            m_boundIbo = id;
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
        }
        break;
    default:
        break;
    }
    return *this;
}

}
