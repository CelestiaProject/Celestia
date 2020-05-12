// glcompat.h
//
// Copyright (C) 2020, the Celestia Development Team
//
// OpenGL 2.1 compatibility layer for OpenGL ES 2.0
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include <cstring> // for memcpy
#include "glcompat.h"


constexpr const int MODELVIEW_STACK_DEPTH = 8;
constexpr const int PROJECTION_STACK_DEPTH = 2;
static float g_modelViewStack[MODELVIEW_STACK_DEPTH][16];
static float g_projectionStack[PROJECTION_STACK_DEPTH][16];
static size_t g_modelViewPosition = 0;
static size_t g_projectionPosition = 0;
static int g_matrixMode = GL_MODELVIEW;

void
initGLCompat() noexcept
{
    g_matrixMode = GL_PROJECTION;
    for (size_t i = 0; i < PROJECTION_STACK_DEPTH; i++)
    {
        g_projectionPosition = i;
        glesLoadIdentity();
    }
    g_projectionPosition = 0;

    g_matrixMode = GL_MODELVIEW;
    for (size_t i = 0; i < MODELVIEW_STACK_DEPTH; i++)
    {
        g_modelViewPosition = i;
        glesLoadIdentity();
    }
    g_modelViewPosition = 0;
}

void
glesMatrixMode(int _g_matrixMode) noexcept
{
    g_matrixMode = _g_matrixMode;
}

void
glesPushMatrix() noexcept
{
    switch (g_matrixMode)
    {
    case GL_MODELVIEW:
        if (g_modelViewPosition < MODELVIEW_STACK_DEPTH - 1)
            g_modelViewPosition++;
        else
            assert(0 && "Matrix stack overflow");
        break;

    case GL_PROJECTION:
        if (g_projectionPosition < PROJECTION_STACK_DEPTH - 1)
            g_projectionPosition++;
        else
            assert(0 && "Matrix stack overflow");
        break;

    default:
        assert(0 && "Incorrect matrix g_matrixMode");
    }
}
void
glesPopMatrix() noexcept
{
    switch (g_matrixMode)
    {
    case GL_MODELVIEW:
        if (g_modelViewPosition > 0)
            g_modelViewPosition--;
        else
            assert(0 && "Matrix stack underflow");
        break;

    case GL_PROJECTION:
        if (g_projectionPosition > 0)
            g_projectionPosition--;
        else
            assert(0 && "Matrix stack underflow");
        break;

    default:
        assert(0 && "Incorrect matrix g_matrixMode");
    }
}

void
glesLoadMatrix(const float *data) noexcept
{
    glesLoadMatrix(g_matrixMode, data);
}

void
glesLoadMatrix(int mode, const float *data) noexcept
{
    switch (mode)
    {
    case GL_MODELVIEW:
        memcpy(g_modelViewStack[g_modelViewPosition], data, sizeof(float)*16);
        break;
    case GL_PROJECTION:
        memcpy(g_projectionStack[g_projectionPosition], data, sizeof(float)*16);
        break;
    default:
        assert("Incorrect matrix g_matrixMode");
    }
}

float*
glesGetMatrix(int mode) noexcept
{
    switch (mode)
    {
    case GL_MODELVIEW:
        assert(g_modelViewPosition < MODELVIEW_STACK_DEPTH);
        if (g_modelViewPosition < MODELVIEW_STACK_DEPTH)
            return g_modelViewStack[g_modelViewPosition];
        break;

    case GL_PROJECTION:
        assert(g_projectionPosition < PROJECTION_STACK_DEPTH);
        if (g_projectionPosition < PROJECTION_STACK_DEPTH)
            return g_projectionStack[g_projectionPosition];
        break;

    default:
        assert(0 && "Incorrect matrix mode");
    }
    return nullptr;
}

void
glesGetMatrix(float *data) noexcept
{
    glesGetMatrix(g_matrixMode, data);
}

void
glesGetMatrix(int mode, float *data) noexcept
{
    switch (mode)
    {
    case GL_MODELVIEW:
        assert(g_modelViewPosition < MODELVIEW_STACK_DEPTH);
        if (g_modelViewPosition < MODELVIEW_STACK_DEPTH)
            memcpy(data, g_modelViewStack[g_modelViewPosition], sizeof(float)*16);
        break;

    case GL_PROJECTION:
        assert(g_projectionPosition < PROJECTION_STACK_DEPTH);
        if (g_projectionPosition < PROJECTION_STACK_DEPTH)
            memcpy(data, g_projectionStack[g_projectionPosition], sizeof(float)*16);
        break;

    default:
        assert(0 && "Incorrect matrix mode");
    }
}

void
glesLoadIdentity() noexcept
{
    float *p = nullptr;

    switch (g_matrixMode)
    {
    case GL_MODELVIEW:
        assert(g_modelViewPosition < MODELVIEW_STACK_DEPTH);
        if (g_modelViewPosition < MODELVIEW_STACK_DEPTH)
            p = g_modelViewStack[g_modelViewPosition];
        break;

    case GL_PROJECTION:
        assert(g_projectionPosition < PROJECTION_STACK_DEPTH);
        if (g_projectionPosition < PROJECTION_STACK_DEPTH)
            p = g_projectionStack[g_projectionPosition];
        break;

    default:
        assert(0 && "Incorrect matrix mode");
    }
    if (p != nullptr)
    {
        for (size_t i = 0; i < 4; i++)
           for (size_t j = 0; j < 4; j++)
               p[i * 4 + j] = i == j ? 1.0f : 0.0f;
    }
}
