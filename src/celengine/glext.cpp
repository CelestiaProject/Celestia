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

#ifndef _WIN32
// Assume that this is a UNIX/X11 system if it's not Windows.
#include "GL/glx.h"
#endif

#include "glext.h"

// ARB_texture_compression
PFNGLCOMPRESSEDTEXIMAGE3DARBPROC glCompressedTexImage3DARB;
PFNGLCOMPRESSEDTEXIMAGE2DARBPROC glCompressedTexImage2DARB;
PFNGLCOMPRESSEDTEXIMAGE1DARBPROC glCompressedTexImage1DARB;
PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC glCompressedTexSubImage3DARB;
PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC glCompressedTexSubImage2DARB;
PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC glCompressedTexSubImage1DARB;

// ARB_multitexture command function pointers
PFNGLMULTITEXCOORD2IARBPROC glMultiTexCoord2iARB;
PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB;
PFNGLMULTITEXCOORD3FARBPROC glMultiTexCoord3fARB;
PFNGLMULTITEXCOORD3FVARBPROC glMultiTexCoord3fvARB;
PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB;

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

// NV_register_combiners2 command function pointers
PFNGLCOMBINERSTAGEPARAMETERFVNVPROC glCombinerStageParameterfvNV;
PFNGLGETCOMBINERSTAGEPARAMETERFVNVPROC glGetCombinerStageParameterfvNV;

// NV_vertex_program function pointers
PFNGLAREPROGRAMSRESIDENTNVPROC glAreProgramsResidentNV ;
PFNGLBINDPROGRAMNVPROC glBindProgramNV ;
PFNGLDELETEPROGRAMSNVPROC glDeleteProgramsNV ;
PFNGLEXECUTEPROGRAMNVPROC glExecuteProgramNV ;
PFNGLGENPROGRAMSNVPROC glGenProgramsNV ;
PFNGLGETPROGRAMPARAMETERDVNVPROC glGetProgramParameterdvNV ;
PFNGLGETPROGRAMPARAMETERFVNVPROC glGetProgramParameterfvNV ;
PFNGLGETPROGRAMIVNVPROC glGetProgramivNV ;
PFNGLGETPROGRAMSTRINGNVPROC glGetProgramStringNV ;
PFNGLGETTRACKMATRIXIVNVPROC glGetTrackMatrixivNV ;
PFNGLGETVERTEXATTRIBDVNVPROC glGetVertexAttribdvNV ;
PFNGLGETVERTEXATTRIBFVNVPROC glGetVertexAttribfvNV ;
PFNGLGETVERTEXATTRIBIVNVPROC glGetVertexAttribivNV ;
PFNGLGETVERTEXATTRIBPOINTERVNVPROC glGetVertexAttribPointervNV ;
PFNGLISPROGRAMNVPROC glIsProgramNV ;
PFNGLLOADPROGRAMNVPROC glLoadProgramNV ;
PFNGLPROGRAMPARAMETER4DNVPROC glProgramParameter4dNV ;
PFNGLPROGRAMPARAMETER4DVNVPROC glProgramParameter4dvNV ;
PFNGLPROGRAMPARAMETER4FNVPROC glProgramParameter4fNV ;
PFNGLPROGRAMPARAMETER4FVNVPROC glProgramParameter4fvNV ;
PFNGLPROGRAMPARAMETERS4DVNVPROC glProgramParameters4dvNV ;
PFNGLPROGRAMPARAMETERS4FVNVPROC glProgramParameters4fvNV ;
PFNGLREQUESTRESIDENTPROGRAMSNVPROC glRequestResidentProgramsNV ;
PFNGLTRACKMATRIXNVPROC glTrackMatrixNV ;
PFNGLVERTEXATTRIBPOINTERNVPROC glVertexAttribPointerNV ;
PFNGLVERTEXATTRIB1DNVPROC glVertexAttrib1dNV ;
PFNGLVERTEXATTRIB1DVNVPROC glVertexAttrib1dvNV ;
PFNGLVERTEXATTRIB1FNVPROC glVertexAttrib1fNV ;
PFNGLVERTEXATTRIB1FVNVPROC glVertexAttrib1fvNV ;
PFNGLVERTEXATTRIB1SNVPROC glVertexAttrib1sNV ;
PFNGLVERTEXATTRIB1SVNVPROC glVertexAttrib1svNV ;
PFNGLVERTEXATTRIB2DNVPROC glVertexAttrib2dNV ;
PFNGLVERTEXATTRIB2DVNVPROC glVertexAttrib2dvNV ;
PFNGLVERTEXATTRIB2FNVPROC glVertexAttrib2fNV ;
PFNGLVERTEXATTRIB2FVNVPROC glVertexAttrib2fvNV ;
PFNGLVERTEXATTRIB2SNVPROC glVertexAttrib2sNV ;
PFNGLVERTEXATTRIB2SVNVPROC glVertexAttrib2svNV ;
PFNGLVERTEXATTRIB3DNVPROC glVertexAttrib3dNV ;
PFNGLVERTEXATTRIB3DVNVPROC glVertexAttrib3dvNV ;
PFNGLVERTEXATTRIB3FNVPROC glVertexAttrib3fNV ;
PFNGLVERTEXATTRIB3FVNVPROC glVertexAttrib3fvNV ;
PFNGLVERTEXATTRIB3SNVPROC glVertexAttrib3sNV ;
PFNGLVERTEXATTRIB3SVNVPROC glVertexAttrib3svNV ;
PFNGLVERTEXATTRIB4DNVPROC glVertexAttrib4dNV ;
PFNGLVERTEXATTRIB4DVNVPROC glVertexAttrib4dvNV ;
PFNGLVERTEXATTRIB4FNVPROC glVertexAttrib4fNV ;
PFNGLVERTEXATTRIB4FVNVPROC glVertexAttrib4fvNV ;
PFNGLVERTEXATTRIB4SNVPROC glVertexAttrib4sNV ;
PFNGLVERTEXATTRIB4SVNVPROC glVertexAttrib4svNV ;
PFNGLVERTEXATTRIB4UBVNVPROC glVertexAttrib4ubvNV ;
PFNGLVERTEXATTRIBS1DVNVPROC glVertexAttribs1dvNV ;
PFNGLVERTEXATTRIBS1FVNVPROC glVertexAttribs1fvNV ;
PFNGLVERTEXATTRIBS1SVNVPROC glVertexAttribs1svNV ;
PFNGLVERTEXATTRIBS2DVNVPROC glVertexAttribs2dvNV ;
PFNGLVERTEXATTRIBS2FVNVPROC glVertexAttribs2fvNV ;
PFNGLVERTEXATTRIBS2SVNVPROC glVertexAttribs2svNV ;
PFNGLVERTEXATTRIBS3DVNVPROC glVertexAttribs3dvNV ;
PFNGLVERTEXATTRIBS3FVNVPROC glVertexAttribs3fvNV ;
PFNGLVERTEXATTRIBS3SVNVPROC glVertexAttribs3svNV ;
PFNGLVERTEXATTRIBS4DVNVPROC glVertexAttribs4dvNV ;
PFNGLVERTEXATTRIBS4FVNVPROC glVertexAttribs4fvNV ;
PFNGLVERTEXATTRIBS4SVNVPROC glVertexAttribs4svNV ;
PFNGLVERTEXATTRIBS4UBVNVPROC glVertexAttribs4ubvNV ;

// EXT_paletted_texture command function pointers
PFNGLCOLORTABLEEXTPROC glColorTableEXT;

// EXT_blend_minmax command function pointers
PFNGLBLENDEQUATIONEXTPROC glBlendEquationEXT;

// WGL_EXT_swap_control command function pointers
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapIntervalEXT;

// extern void Alert(const char *szFormat, ...);

#if defined(_WIN32)

#define GET_GL_PROC_ADDRESS(name) wglGetProcAddress(name)

#else

#ifdef GLX_VERSION_1_4
extern "C" {
extern void (*glXGetProcAddressARB(const GLubyte *procName))();
}
#define GET_GL_PROC_ADDRESS(name) glXGetProcAddressARB((GLubyte*) name)
#else
#define GET_GL_PROC_ADDRESS(name) glXGetProcAddressARB((GLubyte*) name)
#endif

#endif // defined(WIN32)


void Alert(const char *szFormat, ...)
{
}


// ARB_multitexture
void InitExtMultiTexture()
{
// #ifndef GL_ARB_multitexture
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
// #endif // GL_ARB_multitexture
}


// ARB_texture_compression
void InitExtTextureCompression()
{
#if defined(GET_GL_PROC_ADDRESS)
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
#endif // GL_ARB_texture_compression
}


// NV_register_combiners
void InitExtRegisterCombiners()
{
#if defined(GET_GL_PROC_ADDRESS)
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


void InitExtRegisterCombiners2()
{
#if defined(GET_GL_PROC_ADDRESS)
    /* Retrieve all NV_register_combiners routines. */
    glCombinerStageParameterfvNV =
        (PFNGLCOMBINERSTAGEPARAMETERFVNVPROC)
        GET_GL_PROC_ADDRESS("glCombinerStageParameterfvNV");
    glGetCombinerStageParameterfvNV =
        (PFNGLGETCOMBINERSTAGEPARAMETERFVNVPROC)
        GET_GL_PROC_ADDRESS("glGetCombinerStageParameterfvNV");
#endif
}


void InitExtVertexProgram()
{
#if defined(GET_GL_PROC_ADDRESS)
    glAreProgramsResidentNV =
        (PFNGLAREPROGRAMSRESIDENTNVPROC)
        GET_GL_PROC_ADDRESS("glAreProgramsResidentNV");
    glBindProgramNV =
        (PFNGLBINDPROGRAMNVPROC)
        GET_GL_PROC_ADDRESS("glBindProgramNV");
    glDeleteProgramsNV =
        (PFNGLDELETEPROGRAMSNVPROC)
        GET_GL_PROC_ADDRESS("glDeleteProgramsNV");
    glExecuteProgramNV =
        (PFNGLEXECUTEPROGRAMNVPROC)
        GET_GL_PROC_ADDRESS("glExecuteProgramNV");
    glGenProgramsNV =
        (PFNGLGENPROGRAMSNVPROC)
        GET_GL_PROC_ADDRESS("glGenProgramsNV");
    glGetProgramParameterdvNV =
        (PFNGLGETPROGRAMPARAMETERDVNVPROC)
        GET_GL_PROC_ADDRESS("glGetProgramParameterdvNV");
    glGetProgramParameterfvNV =
        (PFNGLGETPROGRAMPARAMETERFVNVPROC)
        GET_GL_PROC_ADDRESS("glGetProgramParameterfvNV");
    glGetProgramivNV =
        (PFNGLGETPROGRAMIVNVPROC)
        GET_GL_PROC_ADDRESS("glGetProgramivNV");
    glGetProgramStringNV =
        (PFNGLGETPROGRAMSTRINGNVPROC)
        GET_GL_PROC_ADDRESS("glGetProgramStringNV");
    glGetTrackMatrixivNV =
        (PFNGLGETTRACKMATRIXIVNVPROC)
        GET_GL_PROC_ADDRESS("glGetTrackMatrixivNV");
    glGetVertexAttribdvNV =
        (PFNGLGETVERTEXATTRIBDVNVPROC)
        GET_GL_PROC_ADDRESS("glGetVertexAttribdvNV");
    glGetVertexAttribfvNV =
        (PFNGLGETVERTEXATTRIBFVNVPROC)
        GET_GL_PROC_ADDRESS("glGetVertexAttribfvNV");
    glGetVertexAttribivNV =
        (PFNGLGETVERTEXATTRIBIVNVPROC)
        GET_GL_PROC_ADDRESS("glGetVertexAttribivNV");
    glGetVertexAttribPointervNV =
        (PFNGLGETVERTEXATTRIBPOINTERVNVPROC)
        GET_GL_PROC_ADDRESS("glGetVertexAttribPointervNV");
    glIsProgramNV =
        (PFNGLISPROGRAMNVPROC)
        GET_GL_PROC_ADDRESS("glIsProgramNV");
    glLoadProgramNV =
        (PFNGLLOADPROGRAMNVPROC)
        GET_GL_PROC_ADDRESS("glLoadProgramNV");
    glProgramParameter4dNV =
        (PFNGLPROGRAMPARAMETER4DNVPROC)
        GET_GL_PROC_ADDRESS("glProgramParameter4dNV");
    glProgramParameter4dvNV =
        (PFNGLPROGRAMPARAMETER4DVNVPROC)
        GET_GL_PROC_ADDRESS("glProgramParameter4dvNV");
    glProgramParameter4fNV =
        (PFNGLPROGRAMPARAMETER4FNVPROC)
        GET_GL_PROC_ADDRESS("glProgramParameter4fNV");
    glProgramParameter4fvNV =
        (PFNGLPROGRAMPARAMETER4FVNVPROC)
        GET_GL_PROC_ADDRESS("glProgramParameter4fvNV");
    glProgramParameters4dvNV =
        (PFNGLPROGRAMPARAMETERS4DVNVPROC)
        GET_GL_PROC_ADDRESS("glProgramParameters4dvNV");
    glProgramParameters4fvNV =
        (PFNGLPROGRAMPARAMETERS4FVNVPROC)
        GET_GL_PROC_ADDRESS("glProgramParameters4fvNV");
    glRequestResidentProgramsNV =
        (PFNGLREQUESTRESIDENTPROGRAMSNVPROC)
        GET_GL_PROC_ADDRESS("glRequestResidentProgramsNV");
    glTrackMatrixNV =
        (PFNGLTRACKMATRIXNVPROC)
        GET_GL_PROC_ADDRESS("glTrackMatrixNV");
    glVertexAttribPointerNV =
        (PFNGLVERTEXATTRIBPOINTERNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribPointerNV");
    glVertexAttrib1dNV =
        (PFNGLVERTEXATTRIB1DNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib1dNV");
    glVertexAttrib1dvNV =
        (PFNGLVERTEXATTRIB1DVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib1dvNV");
    glVertexAttrib1fNV =
        (PFNGLVERTEXATTRIB1FNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib1fNV");
    glVertexAttrib1fvNV =
        (PFNGLVERTEXATTRIB1FVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib1fvNV");
    glVertexAttrib1sNV =
        (PFNGLVERTEXATTRIB1SNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib1sNV");
    glVertexAttrib1svNV =
        (PFNGLVERTEXATTRIB1SVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib1svNV");
    glVertexAttrib2dNV =
        (PFNGLVERTEXATTRIB2DNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib2dNV");
    glVertexAttrib2dvNV =
        (PFNGLVERTEXATTRIB2DVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib2dvNV");
    glVertexAttrib2fNV =
        (PFNGLVERTEXATTRIB2FNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib2fNV");
    glVertexAttrib2fvNV =
        (PFNGLVERTEXATTRIB2FVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib2fvNV");
    glVertexAttrib2sNV =
        (PFNGLVERTEXATTRIB2SNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib2sNV");
    glVertexAttrib2svNV =
        (PFNGLVERTEXATTRIB2SVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib2svNV");
    glVertexAttrib3dNV =
        (PFNGLVERTEXATTRIB3DNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib3dNV");
    glVertexAttrib3dvNV =
        (PFNGLVERTEXATTRIB3DVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib3dvNV");
    glVertexAttrib3fNV =
        (PFNGLVERTEXATTRIB3FNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib3fNV");
    glVertexAttrib3fvNV =
        (PFNGLVERTEXATTRIB3FVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib3fvNV");
    glVertexAttrib3sNV =
        (PFNGLVERTEXATTRIB3SNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib3sNV");
    glVertexAttrib3svNV =
        (PFNGLVERTEXATTRIB3SVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib3svNV");
    glVertexAttrib4dNV =
        (PFNGLVERTEXATTRIB4DNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib4dNV");
    glVertexAttrib4dvNV =
        (PFNGLVERTEXATTRIB4DVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib4dvNV");
    glVertexAttrib4fNV =
        (PFNGLVERTEXATTRIB4FNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib4fNV");
    glVertexAttrib4fvNV =
        (PFNGLVERTEXATTRIB4FVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib4fvNV");
    glVertexAttrib4sNV =
        (PFNGLVERTEXATTRIB4SNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib4sNV");
    glVertexAttrib4svNV =
        (PFNGLVERTEXATTRIB4SVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib4svNV");
    glVertexAttrib4ubvNV =
        (PFNGLVERTEXATTRIB4UBVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib4ubvNV");
    glVertexAttribs1dvNV =
        (PFNGLVERTEXATTRIBS1DVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs1dvNV");
    glVertexAttribs1fvNV =
        (PFNGLVERTEXATTRIBS1FVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs1fvNV");
    glVertexAttribs1svNV =
        (PFNGLVERTEXATTRIBS1SVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs1svNV");
    glVertexAttribs2dvNV =
        (PFNGLVERTEXATTRIBS2DVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs2dvNV");
    glVertexAttribs2fvNV =
        (PFNGLVERTEXATTRIBS2FVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs2fvNV");
    glVertexAttribs2svNV =
        (PFNGLVERTEXATTRIBS2SVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs2svNV");
    glVertexAttribs3dvNV =
        (PFNGLVERTEXATTRIBS3DVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs3dvNV");
    glVertexAttribs3fvNV =
        (PFNGLVERTEXATTRIBS3FVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs3fvNV");
    glVertexAttribs3svNV =
        (PFNGLVERTEXATTRIBS3SVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs3svNV");
    glVertexAttribs4dvNV =
        (PFNGLVERTEXATTRIBS4DVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs4dvNV");
    glVertexAttribs4fvNV =
        (PFNGLVERTEXATTRIBS4FVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs4fvNV");
    glVertexAttribs4svNV =
        (PFNGLVERTEXATTRIBS4SVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs4svNV");
    glVertexAttribs4ubvNV =
        (PFNGLVERTEXATTRIBS4UBVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs4ubvNV");
#endif
}


void InitExtPalettedTexture()
{
#if defined(GET_GL_PROC_ADDRESS)
    glColorTableEXT = (PFNGLCOLORTABLEEXTPROC) GET_GL_PROC_ADDRESS("glColorTableEXT");
#endif
}


void InitExtBlendMinmax()
{
#if defined(GET_GL_PROC_ADDRESS)
    glBlendEquationEXT = (PFNGLBLENDEQUATIONEXTPROC) GET_GL_PROC_ADDRESS("glBlendEquationEXT");
#endif
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
