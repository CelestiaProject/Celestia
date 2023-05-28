// binder.h
//
// Copyright (C) 2023-present, Celestia Development Team.
//
// VAO wrapper.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <celengine/glsupport.h>

#include "buffer.h"
#include "vertexobject.h"

namespace celestia::gl
{

class Binder
{
public:
    /**
     * Get a Binder singleton.
     *
     * @return reference to Binder
     */
    static Binder &get();

    /**
     * @brief Bind a Buffer.
     *
     * Bind a Buffer to a target specified by targetHint.
     *
     * @param bo Buffer to bind.
     * @param targetHint a target to bind the Buffer to.
     * @return self
     */
    Binder &bind(const Buffer &bo);

    /**
     * @brief Unbind the currently bound Buffer.
     *
     * Unbind the currently bound Buffer specified by its targetHint.
     *
     * @param bo a Buffer to unbind.
     * @return self
     */
    Binder &unbind(const Buffer &bo);

    /**
     * @brief Bind a VertexObject.
     *
     * @param vo a VertexObject to bind.
     * @return self
     */
    Binder &bind(const VertexObject &vo);

    /**
     * @brief Unbind the currently bound VertexObject.
     *
     * @param vo a VertexObject to unbind.
     * @return self
     */
    Binder &unbind(const VertexObject &vo);

    /**
     * @brief Unbind the currently bound object.
     *
     * @return self
     */
    template<typename T> Binder &unbind();

    /**
     * @brief Unbind the currently bound Buffer.
     *
     * @param target a target to unbind.
     * @return self
     */
    Binder &unbind(Buffer::TargetHint target);

    //! Destructor.
    ~Binder() = default;

    //! Copying is prohibited.
    Binder(const Binder &) = delete;

    //! Moving is prohibited.
    Binder(Binder &&) = delete;

    //! Copying is prohibited.
    Binder &operator=(const Binder &) = delete;

    //! Moving is prohibited.
    Binder &operator=(Binder &&) = delete;

private:
    Binder() = default;

    Binder &bindVAO(GLuint id);
    Binder &bindVBO(Buffer::TargetHint target, GLuint id);

    GLuint m_boundVbo{ 0 };
    GLuint m_boundIbo{ 0 };
    GLuint m_boundVao{ 0 };
};

}
