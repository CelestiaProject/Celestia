// celx_gl.cpp
//
// Copyright (C) 2003-2026 the Celestia Development Team
//
// Lua script extensions for Celestia: OpenGL functions
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <celengine/glsupport.h>
#include "glcompat.h"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

namespace
{

bool nonNumbers(const sol::variadic_args &va)
{
    return std::any_of(va.cbegin(), va.cend(), [](sol::object o) { return o.get_type() != sol::type::number; });
}

void
glu_LookAt(sol::this_state s, sol::variadic_args va)
{
    if (va.size() != 9 || !nonNumbers(va))
    {
        // TODO: abort
        std::cerr << "glu.LookAt() requires 9 float arguments" << '\n';
        return;
    }
    gluLookAt(va[0].as<float>(), va[1].as<float>(), va[2].as<float>(),
              va[3].as<float>(), va[4].as<float>(), va[6].as<float>(),
              va[6].as<float>(), va[7].as<float>(), va[8].as<float>());
}

void
glu_Ortho2D(sol::this_state s, sol::variadic_args va)
{
    if (va.size() != 4 || !nonNumbers(va))
    {
        // TODO: abort
        std::cerr << "glu.Ortho2D() requires 4 float arguments" << '\n';
        return;
    }
    glOrtho(va[0].as<float>(), va[1].as<float>(), va[2].as<float>(),
            va[3].as<float>(), -1.0, 1.0);
}

void
gl_Frustum(float l, float r, float b, float t, float n, float f)
{
    glFrustum(l, r, b, t, n, f);
}

void
gl_Ortho(float l, float r, float b, float t, float n, float f)
{
    glOrtho(l, r, b, t, n, f);
}

void
gl_TexCoord(float x, float y)
{
    glTexCoord2f(x, y);
}

void
gl_TexParameter(int x, int y, int z)
{
    // TODO: Need to ensure that these values are integers, or better yet use
    // names.
    glTexParameteri((GLint)x, (GLenum)y, (GLenum)z);
}

void
gl_Vertex(float x, float y)
{
    glVertex2f(x, y);
}

void
gl_Color(float r, float g, float b, float a)
{
    glColor4f(r, g, b, a);
}

void
gl_LineWidth(float n)
{
    glLineWidth(n);
}

void
gl_PointSize(float n)
{
    glPointSize(n);
}

void
gl_Translate(float x, float y)
{
    glTranslatef(x, y, 0.0f);
}

void
gl_BlendFunc(int i, int j)
{
    glBlendFunc(static_cast<GLenum>(i), static_cast<GLenum>(j));
}

void
gl_Begin(int i)
{
    glBegin(i);
}

void
gl_End()
{
    glEnd();
}

void
gl_Enable(int i)
{
    glEnable(i);
}

void
gl_Disable(int i)
{
    glDisable(i);
}

void
gl_MatrixMode(int i)
{
    glMatrixMode(i);
}

void
gl_PopMatrix()
{
    glPopMatrix();
}

void
gl_LoadIdentity()
{
    glLoadIdentity();
}

void
gl_PushMatrix()
{
    glPushMatrix();
}

} // namespace

void
LoadLuaGraphicsLibrary(sol::state_view lua)
{
    // GL
    auto gl = lua["testgl"].get_or_create<sol::table>();

    gl["Frustum"]      = gl_Frustum;
    gl["Ortho"]        = gl_Ortho;
    gl["Color"]        = gl_Color;
    gl["LineWidth"]    = gl_LineWidth;
    gl["PointSize"]    = gl_PointSize;
    gl["TexCoord"]     = gl_TexCoord;
    gl["TexParameter"] = gl_TexParameter;
    gl["Vertex"]       = gl_Vertex;
    gl["Translate"]    = gl_Translate;
    gl["BlendFunc"]    = gl_BlendFunc;
    gl["Begin"]        = gl_Begin;
    gl["End"]          = gl_End;
    gl["Enable"]       = gl_Enable;
    gl["Disable"]      = gl_Disable;
    gl["MatrixMode"]   = gl_MatrixMode;
    gl["PopMatrix"]    = gl_PopMatrix;
    gl["LoadIdentity"] = gl_LoadIdentity;
    gl["PushMatrix"]   = gl_PushMatrix;
    // constants
    gl["QUADS"]               = GL_QUADS;
    gl["LIGHTING"]            = GL_LIGHTING;
    gl["POINTS"]              = GL_POINTS;
    gl["LINES"]               = GL_LINES;
    gl["LINE_LOOP"]           = GL_LINE_LOOP;
    gl["POLYGON"]             = GL_POLYGON;
    gl["PROJECTION"]          = GL_PROJECTION;
    gl["MODELVIEW"]           = GL_MODELVIEW;
    gl["BLEND"]               = GL_BLEND;
    gl["TEXTURE_2D"]          = GL_TEXTURE_2D;
    gl["TEXTURE_MAG_FILTER"]  = GL_TEXTURE_MAG_FILTER;
    gl["TEXTURE_MIN_FILTER"]  = GL_TEXTURE_MIN_FILTER;
    gl["LINEAR"]              = GL_LINEAR;
    gl["NEAREST"]             = GL_NEAREST;
    gl["SRC_ALPHA"]           = GL_SRC_ALPHA;
    gl["ONE_MINUS_SRC_ALPHA"] = GL_ONE_MINUS_SRC_ALPHA;

    // GLU
    auto glu = lua["testglu"].get_or_create<sol::table>();

    glu["LookAt"]  = glu_LookAt;
    glu["Ortho2D"] = glu_Ortho2D;
}
