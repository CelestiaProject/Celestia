// gl.h
//
// Why is this file necessary?  Because Microsoft requires us to include
// windows.h before including the GL headers, even though GL is a
// cross-platform API.  So, we encapsulate the resulting #ifdef nonsense
// in this file.
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _GL_H_
#define _GL_H_

#ifdef _WIN32

#ifdef _MSC_VER
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <windows.h>
#endif // _MSC_VER

#endif

#ifndef TARGET_OS_MAC
#ifndef GL_ARB_multitexture
#define GL_ARB_multitexture
#endif
#include <GL/gl.h>
#undef GL_ARB_multitexture
#include <GL/glu.h>
#else
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#endif

#endif // _GL_H_

