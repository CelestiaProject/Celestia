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

#pragma once

#if !defined(__gles2_gl2_h_) && !defined(EPOXY_GL_H)
#error "glcompat.h must be included after GLES2/gl2.h"
#endif

#ifndef GL_PROJECTION
#define GL_PROJECTION 0x1701
#endif
#ifndef GL_MODELVIEW
#define GL_MODELVIEW 0x1700
#endif

#ifdef glMatrixMode
#undef glMatrixMode
#endif
#define glMatrixMode(m) glesMatrixMode(m)

#ifdef glPushMatrix
#undef glPushMatrix
#endif
#define glPushMatrix glesPushMatrix

#ifdef glPopMatrix
#undef glPopMatrix
#endif
#define glPopMatrix glesPopMatrix

#ifdef glLoadMatrixf
#undef glLoadMatrixf
#endif
#define glLoadMatrixf(m) glesLoadMatrix(m)

#ifdef glLoadIdentity
#undef glLoadIdentity
#endif
#define glLoadIdentity glesLoadIdentity

void initGLCompat() noexcept;
void glesMatrixMode(int _g_matrixMode) noexcept;
void glesPushMatrix() noexcept;
void glesPopMatrix() noexcept;
void glesLoadMatrix(const float *data) noexcept;
void glesLoadMatrix(int mode, const float *data) noexcept;
void glesGetMatrix(float *data) noexcept;
void glesGetMatrix(int mode, float *data) noexcept;
float* glesGetMatrix(int mode) noexcept;
void glesLoadIdentity() noexcept;
void glesTranslate(float x, float y, float z) noexcept;
