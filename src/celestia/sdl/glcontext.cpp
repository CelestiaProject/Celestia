// glcontext.cpp
//
// Copyright (C) 2025-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "glcontext.h"

namespace celestia::sdl
{

UniqueGLContext::UniqueGLContext(SDL_GLContext context) : m_context(context)
{
}

UniqueGLContext::~UniqueGLContext()
{
    if (m_context != nullptr)
        SDL_GL_DeleteContext(m_context);
}

UniqueGLContext::UniqueGLContext(UniqueGLContext&& other) noexcept
    : m_context(other.m_context)
{
    other.m_context = nullptr;
}

UniqueGLContext&
UniqueGLContext::operator=(UniqueGLContext&& other) noexcept
{
    if (this != &other)
    {
        m_context = other.m_context;
        other.m_context = nullptr;
    }

    return *this;
}

} // end namespace celestia::sdl
