// glcompat.h
//
// Copyright (C) 2020-present, the Celestia Development Team
//
// OpenGL 2.1 compatibility layer for OpenGL ES 2.0
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#ifndef EPOXY_GL_H
#error "glcompat.h must be included after epoxy/gl.h"
#endif

#ifndef GL_PROJECTION
#define GL_PROJECTION 0x1701
#endif
#ifndef GL_MODELVIEW
#define GL_MODELVIEW 0x1700
#endif
#ifndef GL_POLYGON
#define GL_POLYGON 0x0009
#endif
#ifndef GL_QUADS
#define GL_QUADS 0x0007
#endif
#ifndef GL_LIGHTING
#define GL_LIGHTING 0x0B50
#endif
#ifndef GL_LINE_SMOOTH
#define GL_LINE_SMOOTH 0x0B20
#endif
#ifndef GL_TEXTURE_2D
#define GL_TEXTURE_2D 0x0DE1
#endif
#ifndef GL_PROJECTION_MATRIX
#define GL_PROJECTION_MATRIX 0x0BA7
#endif
#ifndef GL_MODELVIEW_MATRIX
#define GL_MODELVIEW_MATRIX 0x0BA6
#endif

#ifdef glMatrixMode
#undef glMatrixMode
#endif
#define glMatrixMode(m) fpcMatrixMode(m)

#ifdef glPushMatrix
#undef glPushMatrix
#endif
#define glPushMatrix fpcPushMatrix

#ifdef glPopMatrix
#undef glPopMatrix
#endif
#define glPopMatrix fpcPopMatrix

#ifdef glLoadMatrixf
#undef glLoadMatrixf
#endif
#define glLoadMatrixf(m) fpcLoadMatrixf(m)

#ifdef glLoadIdentity
#undef glLoadIdentity
#endif
#define glLoadIdentity fpcLoadIdentity

#ifdef glMultMatrixf
#undef glMultMatrixf
#endif
#define glMultMatrixf(m) fpcMultMatrixf(m)

#ifdef glTranslatef
#undef glTranslatef
#endif
#define glTranslatef(x,y,z) fpcTranslatef(x,y,z)

#ifdef glFrustum
#undef glFrustum
#endif
#define glFrustum(l,r,b,t,n,f) fpcFrustum(l,r,b,t,n,f)

#ifdef glOrtho
#undef glOrtho
#endif
#define glOrtho(l,r,b,t,n,f) fpcOrtho(l,r,b,t,n,f)

#define orig_glGetFloatv(n,m) epoxy_glGetFloatv(n,m)

#ifdef glGetFloatv
#undef glGetFloatv
#endif
#define glGetFloatv(n,m) fpcGetFloatv(n,m)

#define orig_glEnable(i) epoxy_glEnable(i)

#ifdef glEnable
#undef glEnable
#endif
#define glEnable(i) fpcEnable(i)

#define orig_glDisable(i) epoxy_glDisable(i)

#ifdef glDisable
#undef glDisable
#endif
#define glDisable(i) fpcDisable(i)

#ifdef glBegin
#undef glBegin
#endif
#define glBegin(i) fpcBegin(i)

#ifdef glEnd
#undef glEnd
#endif
#define glEnd fpcEnd

#ifdef glColor4f
#undef glColor4f
#endif
#define glColor4f(r,g,b,a) fpcColor4f(r,g,b,a)

#ifdef glVertex2f
#undef glVertex2f
#endif
#define glVertex2f(x,y) fpcVertex2f(x,y)

#ifdef glTexCoord2f
#undef glTexCoord2f
#endif
#define glTexCoord2f(x,y) fpcTexCoord2f(x,y)

#ifdef gluLookAt
#undef gluLookAt
#endif
#define gluLookAt(ix,iy,iz,cx,cy,cz,ux,uy,uz) fpcLookAt(ix,iy,iz,cx,cy,cz,ux,uy,uz)

void fpcMatrixMode(int _g_matrixMode) noexcept;
void fpcPushMatrix() noexcept;
void fpcPopMatrix() noexcept;
void fpcLoadMatrixf(const float *data) noexcept;
void fpcLoadIdentity() noexcept;
void fpcTranslatef(float x, float y, float z) noexcept;
void fpcMultMatrixf(const float *m) noexcept;
void fpcFrustum(float l, float r, float b, float t, float n, float f) noexcept;
void fpcOrtho(float l, float r, float b, float t, float n, float f) noexcept;
void fpcGetFloatv(GLenum pname, GLfloat *params) noexcept;
void fpcEnable(GLenum param) noexcept;
void fpcDisable(GLenum param) noexcept;
void fpcBegin(GLenum param) noexcept;
void fpcEnd() noexcept;
void fpcColor4f(float r, float g, float b, float a) noexcept;
void fpcVertex2f(float x, float y) noexcept;
void fpcTexCoord2f(float x, float y) noexcept;
void fpcLookAt(float ix, float iy, float iz, float cx, float cy, float cz, float ux, float uy, float uz) noexcept;
