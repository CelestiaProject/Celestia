// celx_gl.cpp
//
// Copyright (C) 2003-2008, the Celestia Development Team
//
// Lua script extensions for Celestia: OpenGL functions
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "celx.h"
#include "celx_internal.h"
#include "celx_object.h"
#include <celengine/gl.h>


// ==================== OpenGL ====================

static int glu_LookAt(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(9, 9, "Nine arguments expected for glu.LookAt()");
    float ix = (float)celx.safeGetNumber(1, WrongType, "argument 1 to gl.Frustum must be a number", 0.0);
    float iy = (float)celx.safeGetNumber(2, WrongType, "argument 2 to gl.Frustum must be a number", 0.0);
    float iz = (float)celx.safeGetNumber(3, WrongType, "argument 3 to gl.Frustum must be a number", 0.0);
    float cx = (float)celx.safeGetNumber(4, WrongType, "argument 4 to gl.Frustum must be a number", 0.0);
    float cy = (float)celx.safeGetNumber(5, WrongType, "argument 5 to gl.Frustum must be a number", 0.0);
    float cz = (float)celx.safeGetNumber(6, WrongType, "argument 6 to gl.Frustum must be a number", 0.0);
    float ux = (float)celx.safeGetNumber(7, WrongType, "argument 4 to gl.Frustum must be a number", 0.0);
    float uy = (float)celx.safeGetNumber(8, WrongType, "argument 5 to gl.Frustum must be a number", 0.0);
    float uz = (float)celx.safeGetNumber(9, WrongType, "argument 6 to gl.Frustum must be a number", 0.0);
	gluLookAt(ix,iy,iz,cx,cy,cz,ux,uy,uz);
    return 0;
}

static int gl_Frustum(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(6, 6, "Six arguments expected for gl.Frustum()");
    float ll = (float)celx.safeGetNumber(1, WrongType, "argument 1 to gl.Frustum must be a number", 0.0);
    float r = (float)celx.safeGetNumber(2, WrongType, "argument 2 to gl.Frustum must be a number", 0.0);
    float b = (float)celx.safeGetNumber(3, WrongType, "argument 3 to gl.Frustum must be a number", 0.0);
    float t = (float)celx.safeGetNumber(4, WrongType, "argument 4 to gl.Frustum must be a number", 0.0);
    float n = (float)celx.safeGetNumber(5, WrongType, "argument 5 to gl.Frustum must be a number", 0.0);
    float f = (float)celx.safeGetNumber(6, WrongType, "argument 6 to gl.Frustum must be a number", 0.0);
	glFrustum(ll,r,b,t,n,f);
    return 0;
}

static int gl_Ortho(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(6, 6, "Six arguments expected for gl.Ortho()");
    float ll = (float)celx.safeGetNumber(1, WrongType, "argument 1 to gl.Ortho must be a number", 0.0);
    float r = (float)celx.safeGetNumber(2, WrongType, "argument 2 to gl.Ortho must be a number", 0.0);
    float b = (float)celx.safeGetNumber(3, WrongType, "argument 3 to gl.Ortho must be a number", 0.0);
    float t = (float)celx.safeGetNumber(4, WrongType, "argument 4 to gl.Ortho must be a number", 0.0);
    float n = (float)celx.safeGetNumber(5, WrongType, "argument 5 to gl.Ortho must be a number", 0.0);
    float f = (float)celx.safeGetNumber(6, WrongType, "argument 6 to gl.Ortho must be a number", 0.0);
	glOrtho(ll,r,b,t,n,f);
    return 0;
}

static int glu_Ortho2D(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(4, 4, "Six arguments expected for gl.Ortho2D()");
    float ll = (float)celx.safeGetNumber(1, WrongType, "argument 1 to gl.Ortho must be a number", 0.0);
    float r = (float)celx.safeGetNumber(2, WrongType, "argument 2 to gl.Ortho must be a number", 0.0);
    float b = (float)celx.safeGetNumber(3, WrongType, "argument 3 to gl.Ortho must be a number", 0.0);
    float t = (float)celx.safeGetNumber(4, WrongType, "argument 4 to gl.Ortho must be a number", 0.0);
	gluOrtho2D(ll,r,b,t);
    return 0;
}

static int gl_TexCoord(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "Two arguments expected for gl.TexCoord()");
    float x = (float)celx.safeGetNumber(1, WrongType, "argument 1 to gl.TexCoord must be a number", 0.0);
    float y = (float)celx.safeGetNumber(2, WrongType, "argument 2 to gl.TexCoord must be a number", 0.0);
	glTexCoord2f(x,y);
    return 0;
}

static int gl_TexParameter(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(3, 3, "Three arguments expected for gl.TexParameter()");
    
    // TODO: Need to ensure that these values are integers, or better yet use
    // names.
    float x = (float)celx.safeGetNumber(1, WrongType, "argument 1 to gl.TexParameter must be a number", 0.0);
    float y = (float)celx.safeGetNumber(2, WrongType, "argument 2 to gl.TexParameter must be a number", 0.0);
    float z = (float)celx.safeGetNumber(3, WrongType, "argument 3 to gl.TexParameter must be a number", 0.0);
    glTexParameteri((GLint) x, (GLenum) y, (GLenum) z);
    return 0;
}

static int gl_Vertex(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "Two arguments expected for gl.Vertex()");
    float x = (float)celx.safeGetNumber(1, WrongType, "argument 1 to gl.Vertex must be a number", 0.0);
    float y = (float)celx.safeGetNumber(2, WrongType, "argument 2 to gl.Vertex must be a number", 0.0);
	glVertex2f(x,y);
    return 0;
}

static int gl_Color(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(4, 4, "Four arguments expected for gl.Color()");
    float r = (float)celx.safeGetNumber(1, WrongType, "argument 1 to gl.Color must be a number", 0.0);
    float g = (float)celx.safeGetNumber(2, WrongType, "argument 2 to gl.Color must be a number", 0.0);
    float b = (float)celx.safeGetNumber(3, WrongType, "argument 3 to gl.Color must be a number", 0.0);
    float a = (float)celx.safeGetNumber(4, WrongType, "argument 4 to gl.Color must be a number", 0.0);
	glColor4f(r,g,b,a);
    //	glColor4f(0.8f, 0.5f, 0.5f, 1.0f);
    return 0;
}

static int gl_LineWidth(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "One argument expected for gl.LineWidth()");
    float n = (float)celx.safeGetNumber(1, WrongType, "argument 1 to gl.LineWidth must be a number", 1.0);
    glLineWidth(n);
    return 0;
}

static int gl_Translate(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "Two arguments expected for gl.Translate()");
    float x = (float)celx.safeGetNumber(1, WrongType, "argument 1 to gl.Translate must be a number", 0.0);
    float y = (float)celx.safeGetNumber(2, WrongType, "argument 2 to gl.Translate must be a number", 0.0);
	glTranslatef(x,y,0.0f);
    return 0;
}

static int gl_BlendFunc(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "Two arguments expected for gl.BlendFunc()");
    int i = (int)celx.safeGetNumber(1, WrongType, "argument 1 to gl.BlendFunc must be a number", 0.0);
    int j = (int)celx.safeGetNumber(2, WrongType, "argument 2 to gl.BlendFunc must be a number", 0.0);
	glBlendFunc(i,j);
    return 0;
}

static int gl_Begin(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "One argument expected for gl.Begin()");
    int i = (int)celx.safeGetNumber(1, WrongType, "argument 1 to gl.Begin must be a number", 0.0);
	glBegin(i);
    return 0;
}

static int gl_End(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(0, 0, "No arguments expected for gl.PopMatrix()");
    glEnd();
    return 0;
}

static int gl_Enable(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "One argument expected for gl.Enable()");
    int i = (int)celx.safeGetNumber(1, WrongType, "argument 1 to gl.Enable must be a number", 0.0);
	glEnable(i);
    return 0;
}

static int gl_Disable(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "One argument expected for gl.Disable()");
    int i = (int)celx.safeGetNumber(1, WrongType, "argument 1 to gl.Disable must be a number", 0.0);
	glDisable(i);
    return 0;
}

static int gl_MatrixMode(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "One argument expected for gl.MatrixMode()");
    int i = (int) celx.safeGetNumber(1, WrongType, "argument 1 to gl.MatrixMode must be a number", 0.0);
	glMatrixMode(i);
    return 0;
}

static int gl_PopMatrix(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(0, 0, "No arguments expected for gl.PopMatrix()");
    glPopMatrix();
    return 0;
}

static int gl_LoadIdentity(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(0, 0, "No arguments expected for gl.LoadIdentity()");
    glLoadIdentity();
    return 0;
}

static int gl_PushMatrix(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(0, 0, "No arguments expected for gl.PushMatrix()");
    glPushMatrix();
    return 0;
}

void LoadLuaGraphicsLibrary(lua_State* l)
{
    CelxLua celx(l);
    lua_pushstring(l, "gl");
    lua_newtable(l);
    
    celx.registerMethod("Frustum", gl_Frustum);
    celx.registerMethod("Ortho", gl_Ortho);
    celx.registerMethod("Color", gl_Color);
    celx.registerMethod("LineWidth", gl_LineWidth);
    celx.registerMethod("TexCoord", gl_TexCoord);
    celx.registerMethod("TexParameter", gl_TexParameter);
    celx.registerMethod("Vertex", gl_Vertex);
    celx.registerMethod("Translate", gl_Translate);
    celx.registerMethod("BlendFunc", gl_BlendFunc);
    celx.registerMethod("Begin", gl_Begin);
    celx.registerMethod("End", gl_End);
    celx.registerMethod("Enable", gl_Enable);
    celx.registerMethod("Disable", gl_Disable);
    celx.registerMethod("MatrixMode", gl_MatrixMode);
    celx.registerMethod("PopMatrix", gl_PopMatrix);
    celx.registerMethod("LoadIdentity", gl_LoadIdentity);
    celx.registerMethod("PushMatrix", gl_PushMatrix);
    
    celx.registerValue("QUADS", GL_QUADS);
    celx.registerValue("LIGHTING", GL_LIGHTING);
    celx.registerValue("POINTS", GL_POINTS);
    celx.registerValue("LINES", GL_LINES);
    celx.registerValue("LINE_LOOP", GL_LINE_LOOP);
    celx.registerValue("LINE_SMOOTH", GL_LINE_SMOOTH);
    celx.registerValue("POLYGON", GL_POLYGON);
    celx.registerValue("PROJECTION", GL_PROJECTION);
    celx.registerValue("MODELVIEW", GL_MODELVIEW);
    celx.registerValue("BLEND", GL_BLEND);
    celx.registerValue("TEXTURE_2D", GL_TEXTURE_2D);
    celx.registerValue("TEXTURE_MAG_FILTER", GL_TEXTURE_MAG_FILTER);
    celx.registerValue("TEXTURE_MIN_FILTER", GL_TEXTURE_MIN_FILTER);
    celx.registerValue("LINEAR", GL_LINEAR);
    celx.registerValue("NEAREST", GL_NEAREST);
    celx.registerValue("SRC_ALPHA", GL_SRC_ALPHA);
    celx.registerValue("ONE_MINUS_SRC_ALPHA", GL_ONE_MINUS_SRC_ALPHA);
    lua_settable(l, LUA_GLOBALSINDEX);
    
    lua_pushstring(l, "glu");
    lua_newtable(l);
    celx.registerMethod("LookAt", glu_LookAt);
    celx.registerMethod("Ortho2D", glu_Ortho2D);
    lua_settable(l, LUA_GLOBALSINDEX);
}
