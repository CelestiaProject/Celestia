// glext.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <string.h>
#include "gl.h"
#include "glext.h"

#ifndef _WIN32
// Assume that this is a UNIX/X11 system if it's not Windows.
#include "GL/glx.h"
#endif

// ARB_texture_compression
PFNGLCOMPRESSEDTEXIMAGE3DARBPROC glCompressedTexImage3DARB;
PFNGLCOMPRESSEDTEXIMAGE2DARBPROC glCompressedTexImage2DARB;
PFNGLCOMPRESSEDTEXIMAGE1DARBPROC glCompressedTexImage1DARB;
PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC glCompressedTexSubImage3DARB;
PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC glCompressedTexSubImage2DARB;
PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC glCompressedTexSubImage1DARB;

// ARB_multitexture command function pointers
#ifndef GL_ARB_multitexture
PFNGLMULTITEXCOORD2IARBPROC glMultiTexCoord2iARB;
PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB;
PFNGLMULTITEXCOORD3FARBPROC glMultiTexCoord3fARB;
PFNGLMULTITEXCOORD3FVARBPROC glMultiTexCoord3fvARB;
PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB;
#endif

// NV_register_combiners command function pointers
PFNGLCOMBINERPARAMETERFVNVPROC glCombinerParameterfvNV;
PFNGLCOMBINERPARAMETERIVNVPROC glCombinerParameterivNV;
PFNGLCOMBINERPARAMETERFNVPROC glCombinerParameterfNV;
PFNGLCOMBINERPARAMETERINVPROC glCombinerParameteriNV;
PFNGLCOMBINERINPUTNVPROC glCombinerInputNV;
PFNGLCOMBINEROUTPUTNVPROC glCombinerOutputNV;
PFNGLFINALCOMBINERINPUTNVPROC glFinalCombinerInputNV;
PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC glGetCombinerInputParameterfvNV;
PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC glGetCombinerInputParameterivNV;
PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC glGetCombinerOutputParameterfvNV;
PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC glGetCombinerOutputParameterivNV;
PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC glGetFinalCombinerInputParameterfvNV;
PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC glGetFinalCombinerInputParameterivNV;

// EXT_paletted_texture command function pointers
PFNGLCOLORTABLEEXTPROC glColorTableEXT;

// WGL_EXT_swap_control command function pointers
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapIntervalEXT;

// extern void Alert(const char *szFormat, ...);

#if defined(_WIN32)
#define GET_GL_PROC_ADDRESS(name) wglGetProcAddress(name)
#elif defined(GLX_ARB_get_proc_address)
#define GET_GL_PROC_ADDRESS(name) glXGetProcAddressARB((GLubyte*) name)
#endif


void Alert(const char *szFormat, ...)
{
}

#if 0
// Check for required extensions and initialize them if present
bool InitGLExtensions(void)
{
#if 0
    if (!extensionSupported("GL_EXT_bgra")) {
        Alert("Required OpenGL extension GL_EXT_bgra not supported");
        return false;
    }
#endif

    if (!extensionSupported("GL_ARB_multitexture")) {
        Alert("Required OpenGL extension GL_ARB_multitexture not supported");
        return false;
    }

#if 0
    if (!extensionSupported("GL_EXT_texture_env_combine")) {
	Alert("Required OpenGL extension GL_EXT_texture_env_combine not supported");
	return false;
    }

    if (!extensionSupported("GL_NV_register_combiners")) {
        Alert("Required OpenGL extension GL_NV_register_combiners not supported");
        return false;
    }

    if (!extensionSupported("GL_EXT_texture_cube_map")) {
        Alert("Required OpenGL extension GL_EXT_texture_cube_map not supported");
        return false;
    }

    if (!extensionSupported("GL_EXT_separate_specular_color")) {
        Alert("Required OpenGL extension GL_EXT_separate_specular_color not supported");
        return false;
    }

    if (!extensionSupported("WGL_EXT_swap_control")) {
        Alert("Required OpenGL extension WGL_EXT_swap_control not supported");
        return false;
    }
#endif

    initMultiTexture();
#if 0
    initRegisterCombiners();
    initSwapControl();
#endif

    return true;
}
#endif

// ARB_multitexture
void InitExtMultiTexture()
{
#ifndef GL_ARB_multitexture
#ifdef GET_GL_PROC_ADDRESS
    glMultiTexCoord2iARB =
        (PFNGLMULTITEXCOORD2IARBPROC) GET_GL_PROC_ADDRESS("glMultiTexCoord2iARB");
    glMultiTexCoord2fARB =
        (PFNGLMULTITEXCOORD2FARBPROC) GET_GL_PROC_ADDRESS("glMultiTexCoord2fARB");
    glMultiTexCoord3fARB =
        (PFNGLMULTITEXCOORD3FARBPROC) GET_GL_PROC_ADDRESS("glMultiTexCoord3fARB");
    glMultiTexCoord3fvARB =
        (PFNGLMULTITEXCOORD3FVARBPROC) GET_GL_PROC_ADDRESS("glMultiTexCoord3fvARB");
    glActiveTextureARB =
        (PFNGLACTIVETEXTUREARBPROC) GET_GL_PROC_ADDRESS("glActiveTextureARB");
    glClientActiveTextureARB =
        (PFNGLCLIENTACTIVETEXTUREARBPROC) GET_GL_PROC_ADDRESS("glClientActiveTextureARB");
#endif // GET_GL_PROC_ADDRESS
#endif // GL_ARB_multitexture
}


// ARB_texture_compression
void InitExtTextureCompression()
{
#ifdef GET_GL_PROC_ADDRESS
    glCompressedTexImage3DARB =
        (PFNGLCOMPRESSEDTEXIMAGE3DARBPROC)
        GET_GL_PROC_ADDRESS("glCompressedTexImage3DARB");
    glCompressedTexImage2DARB =
        (PFNGLCOMPRESSEDTEXIMAGE2DARBPROC)
        GET_GL_PROC_ADDRESS("glCompressedTexImage2DARB");
    glCompressedTexImage1DARB =
        (PFNGLCOMPRESSEDTEXIMAGE1DARBPROC)
        GET_GL_PROC_ADDRESS("glCompressedTexImage1DARB");
    glCompressedTexSubImage3DARB =
        (PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC)
        GET_GL_PROC_ADDRESS("glCompressedTexSubImage3DARB");
    glCompressedTexSubImage2DARB =
        (PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC)
        GET_GL_PROC_ADDRESS("glCompressedTexSubImage2DARB");
    glCompressedTexSubImage1DARB =
        (PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC)
        GET_GL_PROC_ADDRESS("glCompressedTexSubImage1DARB");
#endif // GET_GL_PROC_ADDRESS
}


// NV_register_combiners
void InitExtRegisterCombiners()
{
#ifdef GET_GL_PROC_ADDRESS
  /* Retrieve all NV_register_combiners routines. */
  glCombinerParameterfvNV =
    (PFNGLCOMBINERPARAMETERFVNVPROC)
    GET_GL_PROC_ADDRESS("glCombinerParameterfvNV");
  glCombinerParameterivNV =
    (PFNGLCOMBINERPARAMETERIVNVPROC)
    GET_GL_PROC_ADDRESS("glCombinerParameterivNV");
  glCombinerParameterfNV =
    (PFNGLCOMBINERPARAMETERFNVPROC)
    GET_GL_PROC_ADDRESS("glCombinerParameterfNV");
  glCombinerParameteriNV =
    (PFNGLCOMBINERPARAMETERINVPROC)
    GET_GL_PROC_ADDRESS("glCombinerParameteriNV");
  glCombinerInputNV =
    (PFNGLCOMBINERINPUTNVPROC)
    GET_GL_PROC_ADDRESS("glCombinerInputNV");
  glCombinerOutputNV =
    (PFNGLCOMBINEROUTPUTNVPROC)
    GET_GL_PROC_ADDRESS("glCombinerOutputNV");
  glFinalCombinerInputNV =
    (PFNGLFINALCOMBINERINPUTNVPROC)
    GET_GL_PROC_ADDRESS("glFinalCombinerInputNV");
  glGetCombinerInputParameterfvNV =
    (PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC)
    GET_GL_PROC_ADDRESS("glGetCombinerInputParameterfvNV");
  glGetCombinerInputParameterivNV =
    (PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC)
    GET_GL_PROC_ADDRESS("glGetCombinerInputParameterivNV");
  glGetCombinerOutputParameterfvNV =
    (PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC)
    GET_GL_PROC_ADDRESS("glGetCombinerOutputParameterfvNV");
  glGetCombinerOutputParameterivNV =
    (PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC)
    GET_GL_PROC_ADDRESS("glGetCombinerOutputParameterivNV");
  glGetFinalCombinerInputParameterfvNV =
    (PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC)
    GET_GL_PROC_ADDRESS("glGetFinalCombinerInputParameterfvNV");
  glGetFinalCombinerInputParameterivNV =
    (PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC)
    GET_GL_PROC_ADDRESS("glGetFinalCombinerInputParameterivNV");
#endif // GET_GL_PROC_ADDRESS
}


void InitExtPalettedTexture()
{
#ifdef GET_GL_PROC_ADDRESS
    glColorTableEXT = (PFNGLCOLORTABLEEXTPROC) GET_GL_PROC_ADDRESS("glColorTableEXT");
#endif // _GET_GL_PROC_ADDRESS
}


void InitExtSwapControl()
{
#ifdef _WIN32
    wglSwapIntervalEXT = 
        (PFNWGLSWAPINTERVALEXTPROC) GET_GL_PROC_ADDRESS("wglSwapIntervalEXT");
    wglGetSwapIntervalEXT =
        (PFNWGLGETSWAPINTERVALEXTPROC) GET_GL_PROC_ADDRESS("wglGetSwapIntervalEXT");
#endif // _WIN32
}


bool ExtensionSupported(char *ext)
{
    char *extensions = (char *) glGetString(GL_EXTENSIONS);

    if (extensions == NULL)
        return false;

    int len = strlen(ext);
    for (;;) {
        if (strncmp(extensions, ext, len) == 0)
            return true;
        extensions = strchr(extensions, ' ');
        if  (extensions != NULL)
            extensions++;
        else
            break;
    }

    return false;
}
