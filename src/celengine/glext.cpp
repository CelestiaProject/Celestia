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
// Assume that this is a UNIX/X11 system if it's not Windows or Mac OS X.
#ifndef MACOSX
#include "GL/glx.h"
#endif /* ! MACOSX */
#endif /* ! _WIN32 */

#include "glext.h"
// ARB_texture_compression
PFNGLCOMPRESSEDTEXIMAGE3DARBPROC EXTglCompressedTexImage3DARB;
PFNGLCOMPRESSEDTEXIMAGE2DARBPROC EXTglCompressedTexImage2DARB;
PFNGLCOMPRESSEDTEXIMAGE1DARBPROC EXTglCompressedTexImage1DARB;
PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC EXTglCompressedTexSubImage3DARB;
PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC EXTglCompressedTexSubImage2DARB;
PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC EXTglCompressedTexSubImage1DARB;

// ARB_multitexture command function pointers
PFNGLMULTITEXCOORD2IARBPROC EXTglMultiTexCoord2iARB;
PFNGLMULTITEXCOORD2FARBPROC EXTglMultiTexCoord2fARB;
PFNGLMULTITEXCOORD3FARBPROC EXTglMultiTexCoord3fARB;
PFNGLMULTITEXCOORD3FVARBPROC EXTglMultiTexCoord3fvARB;
PFNGLACTIVETEXTUREARBPROC EXTglActiveTextureARB;
PFNGLCLIENTACTIVETEXTUREARBPROC EXTglClientActiveTextureARB;

// NV_register_combiners command function pointers
PFNGLCOMBINERPARAMETERFVNVPROC EXTglCombinerParameterfvNV;
PFNGLCOMBINERPARAMETERIVNVPROC EXTglCombinerParameterivNV;
PFNGLCOMBINERPARAMETERFNVPROC EXTglCombinerParameterfNV;
PFNGLCOMBINERPARAMETERINVPROC EXTglCombinerParameteriNV;
PFNGLCOMBINERINPUTNVPROC EXTglCombinerInputNV;
PFNGLCOMBINEROUTPUTNVPROC EXTglCombinerOutputNV;
PFNGLFINALCOMBINERINPUTNVPROC EXTglFinalCombinerInputNV;
PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC EXTglGetCombinerInputParameterfvNV;
PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC EXTglGetCombinerInputParameterivNV;
PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC EXTglGetCombinerOutputParameterfvNV;
PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC EXTglGetCombinerOutputParameterivNV;
PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC EXTglGetFinalCombinerInputParameterfvNV;
PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC EXTglGetFinalCombinerInputParameterivNV;

// NV_register_combiners2 command function pointers
PFNGLCOMBINERSTAGEPARAMETERFVNVPROC EXTglCombinerStageParameterfvNV;
PFNGLGETCOMBINERSTAGEPARAMETERFVNVPROC EXTglGetCombinerStageParameterfvNV;

// NV_vertex_program function pointers
PFNGLAREPROGRAMSRESIDENTNVPROC EXTglAreProgramsResidentNV ;
PFNGLBINDPROGRAMNVPROC EXTglBindProgramNV ;
PFNGLDELETEPROGRAMSNVPROC EXTglDeleteProgramsNV ;
PFNGLEXECUTEPROGRAMNVPROC EXTglExecuteProgramNV ;
PFNGLGENPROGRAMSNVPROC EXTglGenProgramsNV ;
PFNGLGETPROGRAMPARAMETERDVNVPROC EXTglGetProgramParameterdvNV ;
PFNGLGETPROGRAMPARAMETERFVNVPROC EXTglGetProgramParameterfvNV ;
PFNGLGETPROGRAMIVNVPROC EXTglGetProgramivNV ;
PFNGLGETPROGRAMSTRINGNVPROC EXTglGetProgramStringNV ;
PFNGLGETTRACKMATRIXIVNVPROC EXTglGetTrackMatrixivNV ;
PFNGLGETVERTEXATTRIBDVNVPROC EXTglGetVertexAttribdvNV ;
PFNGLGETVERTEXATTRIBFVNVPROC EXTglGetVertexAttribfvNV ;
PFNGLGETVERTEXATTRIBIVNVPROC EXTglGetVertexAttribivNV ;
PFNGLGETVERTEXATTRIBPOINTERVNVPROC EXTglGetVertexAttribPointervNV ;
PFNGLISPROGRAMNVPROC EXTglIsProgramNV ;
PFNGLLOADPROGRAMNVPROC EXTglLoadProgramNV ;
PFNGLPROGRAMPARAMETER4DNVPROC EXTglProgramParameter4dNV ;
PFNGLPROGRAMPARAMETER4DVNVPROC EXTglProgramParameter4dvNV ;
PFNGLPROGRAMPARAMETER4FNVPROC EXTglProgramParameter4fNV ;
PFNGLPROGRAMPARAMETER4FVNVPROC EXTglProgramParameter4fvNV ;
PFNGLPROGRAMPARAMETERS4DVNVPROC EXTglProgramParameters4dvNV ;
PFNGLPROGRAMPARAMETERS4FVNVPROC EXTglProgramParameters4fvNV ;
PFNGLREQUESTRESIDENTPROGRAMSNVPROC EXTglRequestResidentProgramsNV ;
PFNGLTRACKMATRIXNVPROC EXTglTrackMatrixNV ;
PFNGLVERTEXATTRIBPOINTERNVPROC EXTglVertexAttribPointerNV ;
PFNGLVERTEXATTRIB1DNVPROC EXTglVertexAttrib1dNV ;
PFNGLVERTEXATTRIB1DVNVPROC EXTglVertexAttrib1dvNV ;
PFNGLVERTEXATTRIB1FNVPROC EXTglVertexAttrib1fNV ;
PFNGLVERTEXATTRIB1FVNVPROC EXTglVertexAttrib1fvNV ;
PFNGLVERTEXATTRIB1SNVPROC EXTglVertexAttrib1sNV ;
PFNGLVERTEXATTRIB1SVNVPROC EXTglVertexAttrib1svNV ;
PFNGLVERTEXATTRIB2DNVPROC EXTglVertexAttrib2dNV ;
PFNGLVERTEXATTRIB2DVNVPROC EXTglVertexAttrib2dvNV ;
PFNGLVERTEXATTRIB2FNVPROC EXTglVertexAttrib2fNV ;
PFNGLVERTEXATTRIB2FVNVPROC EXTglVertexAttrib2fvNV ;
PFNGLVERTEXATTRIB2SNVPROC EXTglVertexAttrib2sNV ;
PFNGLVERTEXATTRIB2SVNVPROC EXTglVertexAttrib2svNV ;
PFNGLVERTEXATTRIB3DNVPROC EXTglVertexAttrib3dNV ;
PFNGLVERTEXATTRIB3DVNVPROC EXTglVertexAttrib3dvNV ;
PFNGLVERTEXATTRIB3FNVPROC EXTglVertexAttrib3fNV ;
PFNGLVERTEXATTRIB3FVNVPROC EXTglVertexAttrib3fvNV ;
PFNGLVERTEXATTRIB3SNVPROC EXTglVertexAttrib3sNV ;
PFNGLVERTEXATTRIB3SVNVPROC EXTglVertexAttrib3svNV ;
PFNGLVERTEXATTRIB4DNVPROC EXTglVertexAttrib4dNV ;
PFNGLVERTEXATTRIB4DVNVPROC EXTglVertexAttrib4dvNV ;
PFNGLVERTEXATTRIB4FNVPROC EXTglVertexAttrib4fNV ;
PFNGLVERTEXATTRIB4FVNVPROC EXTglVertexAttrib4fvNV ;
PFNGLVERTEXATTRIB4SNVPROC EXTglVertexAttrib4sNV ;
PFNGLVERTEXATTRIB4SVNVPROC EXTglVertexAttrib4svNV ;
PFNGLVERTEXATTRIB4UBVNVPROC EXTglVertexAttrib4ubvNV ;
PFNGLVERTEXATTRIBS1DVNVPROC EXTglVertexAttribs1dvNV ;
PFNGLVERTEXATTRIBS1FVNVPROC EXTglVertexAttribs1fvNV ;
PFNGLVERTEXATTRIBS1SVNVPROC EXTglVertexAttribs1svNV ;
PFNGLVERTEXATTRIBS2DVNVPROC EXTglVertexAttribs2dvNV ;
PFNGLVERTEXATTRIBS2FVNVPROC EXTglVertexAttribs2fvNV ;
PFNGLVERTEXATTRIBS2SVNVPROC EXTglVertexAttribs2svNV ;
PFNGLVERTEXATTRIBS3DVNVPROC EXTglVertexAttribs3dvNV ;
PFNGLVERTEXATTRIBS3FVNVPROC EXTglVertexAttribs3fvNV ;
PFNGLVERTEXATTRIBS3SVNVPROC EXTglVertexAttribs3svNV ;
PFNGLVERTEXATTRIBS4DVNVPROC EXTglVertexAttribs4dvNV ;
PFNGLVERTEXATTRIBS4FVNVPROC EXTglVertexAttribs4fvNV ;
PFNGLVERTEXATTRIBS4SVNVPROC EXTglVertexAttribs4svNV ;
PFNGLVERTEXATTRIBS4UBVNVPROC EXTglVertexAttribs4ubvNV ;

// EXT_paletted_texture command function pointers
PFNGLCOLORTABLEEXTPROC EXTglColorTableEXT;

// EXT_blend_minmax command function pointers
PFNGLBLENDEQUATIONEXTPROC EXTglBlendEquationEXT;

// WGL_EXT_swap_control command function pointers
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapIntervalEXT;

// extern void Alert(const char *szFormat, ...);

#ifndef MACOSX
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
#endif /* !MACOSX */
#ifdef MACOSX
#include <mach-o/dyld.h>
#include <stdio.h>
typedef void (*FUNCS) (void);
const struct mach_header *openGLImagePtr = NULL;
FUNCS osxGetProcAddress(const GLubyte *procName) {
    char myProcName[128];
    NSSymbol mySymbol = NULL;
    FUNCS myPtr = NULL;
    if (openGLImagePtr == NULL) {
        openGLImagePtr = NSAddImage("/System/Library/Frameworks/OpenGL.framework/Versions/A/OpenGL",NSADDIMAGE_OPTION_RETURN_ON_ERROR);
#if 0
        unsigned long i;
        unsigned long imageCount = _dyld_image_count();
        for (i=0;i<imageCount;++i) {
            printf("Image[%d] = %s\n",(int)i,_dyld_get_image_name(i));
            if (!strcmp(_dyld_get_image_name(i),"/System/Library/Frameworks/OpenGL.framework/Versions/A/OpenGL")) {
                printf("Found OpenGL image.\n");
                openGLImagePtr = _dyld_get_image_header(i);
                break;
            }
        }
#endif
    }
    if (openGLImagePtr == NULL) {
        printf("Can't find OpenGL??\n");
        return NULL;
    }
    strcpy(myProcName,"_");
    
    /* sanity check */
    if (strlen((char *)procName)>125) return NULL;
    strcat(myProcName,(char *)procName);
    //printf("%s\n",myProcName);
    //if (NSIsSymbolNameDefinedInImage(openGLImagePtr,myProcName) != FALSE) {
    mySymbol = NSLookupSymbolInImage(openGLImagePtr, myProcName, NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW | NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
    if (mySymbol != NULL)
        myPtr = (FUNCS)NSAddressOfSymbol(mySymbol);
    //printf("  (symbol, address) -> (%08x -> %08x)\n",(unsigned int)mySymbol,(unsigned int)myPtr);
    return myPtr;
    //}
}
#define GET_GL_PROC_ADDRESS(name) osxGetProcAddress((GLubyte *)name)
#endif /* MACOSX */


void Alert(const char *szFormat, ...)
{
}


// ARB_multitexture
void InitExtMultiTexture()
{
// #ifndef GL_ARB_multitexture
#ifdef GET_GL_PROC_ADDRESS
    EXTglMultiTexCoord2iARB =
        (PFNGLMULTITEXCOORD2IARBPROC) GET_GL_PROC_ADDRESS("glMultiTexCoord2iARB");
    EXTglMultiTexCoord2fARB =
        (PFNGLMULTITEXCOORD2FARBPROC) GET_GL_PROC_ADDRESS("glMultiTexCoord2fARB");
    EXTglMultiTexCoord3fARB =
        (PFNGLMULTITEXCOORD3FARBPROC) GET_GL_PROC_ADDRESS("glMultiTexCoord3fARB");
    EXTglMultiTexCoord3fvARB =
        (PFNGLMULTITEXCOORD3FVARBPROC) GET_GL_PROC_ADDRESS("glMultiTexCoord3fvARB");
    EXTglActiveTextureARB =
        (PFNGLACTIVETEXTUREARBPROC) GET_GL_PROC_ADDRESS("glActiveTextureARB");
    EXTglClientActiveTextureARB =
        (PFNGLCLIENTACTIVETEXTUREARBPROC) GET_GL_PROC_ADDRESS("glClientActiveTextureARB");
#endif // GET_GL_PROC_ADDRESS
// #endif // GL_ARB_multitexture
}


// ARB_texture_compression
void InitExtTextureCompression()
{
#if defined(GET_GL_PROC_ADDRESS)
    EXTglCompressedTexImage3DARB =
        (PFNGLCOMPRESSEDTEXIMAGE3DARBPROC)
        GET_GL_PROC_ADDRESS("glCompressedTexImage3DARB");
    EXTglCompressedTexImage2DARB =
        (PFNGLCOMPRESSEDTEXIMAGE2DARBPROC)
        GET_GL_PROC_ADDRESS("glCompressedTexImage2DARB");
    EXTglCompressedTexImage1DARB =
        (PFNGLCOMPRESSEDTEXIMAGE1DARBPROC)
        GET_GL_PROC_ADDRESS("glCompressedTexImage1DARB");
    EXTglCompressedTexSubImage3DARB =
        (PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC)
        GET_GL_PROC_ADDRESS("glCompressedTexSubImage3DARB");
    EXTglCompressedTexSubImage2DARB =
        (PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC)
        GET_GL_PROC_ADDRESS("glCompressedTexSubImage2DARB");
    EXTglCompressedTexSubImage1DARB =
        (PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC)
        GET_GL_PROC_ADDRESS("glCompressedTexSubImage1DARB");
#endif // GL_ARB_texture_compression
}


// NV_register_combiners
void InitExtRegisterCombiners()
{
#if defined(GET_GL_PROC_ADDRESS)
  /* Retrieve all NV_register_combiners routines. */
  EXTglCombinerParameterfvNV =
    (PFNGLCOMBINERPARAMETERFVNVPROC)
    GET_GL_PROC_ADDRESS("glCombinerParameterfvNV");
  EXTglCombinerParameterivNV =
    (PFNGLCOMBINERPARAMETERIVNVPROC)
    GET_GL_PROC_ADDRESS("glCombinerParameterivNV");
  EXTglCombinerParameterfNV =
    (PFNGLCOMBINERPARAMETERFNVPROC)
    GET_GL_PROC_ADDRESS("glCombinerParameterfNV");
  EXTglCombinerParameteriNV =
    (PFNGLCOMBINERPARAMETERINVPROC)
    GET_GL_PROC_ADDRESS("glCombinerParameteriNV");
  EXTglCombinerInputNV =
    (PFNGLCOMBINERINPUTNVPROC)
    GET_GL_PROC_ADDRESS("glCombinerInputNV");
  EXTglCombinerOutputNV =
    (PFNGLCOMBINEROUTPUTNVPROC)
    GET_GL_PROC_ADDRESS("glCombinerOutputNV");
  EXTglFinalCombinerInputNV =
    (PFNGLFINALCOMBINERINPUTNVPROC)
    GET_GL_PROC_ADDRESS("glFinalCombinerInputNV");
  EXTglGetCombinerInputParameterfvNV =
    (PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC)
    GET_GL_PROC_ADDRESS("glGetCombinerInputParameterfvNV");
  EXTglGetCombinerInputParameterivNV =
    (PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC)
    GET_GL_PROC_ADDRESS("glGetCombinerInputParameterivNV");
  EXTglGetCombinerOutputParameterfvNV =
    (PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC)
    GET_GL_PROC_ADDRESS("glGetCombinerOutputParameterfvNV");
  EXTglGetCombinerOutputParameterivNV =
    (PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC)
    GET_GL_PROC_ADDRESS("glGetCombinerOutputParameterivNV");
  EXTglGetFinalCombinerInputParameterfvNV =
    (PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC)
    GET_GL_PROC_ADDRESS("glGetFinalCombinerInputParameterfvNV");
  EXTglGetFinalCombinerInputParameterivNV =
    (PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC)
    GET_GL_PROC_ADDRESS("glGetFinalCombinerInputParameterivNV");
#endif // GET_GL_PROC_ADDRESS
}


void InitExtRegisterCombiners2()
{
#if defined(GET_GL_PROC_ADDRESS)
    /* Retrieve all NV_register_combiners routines. */
    EXTglCombinerStageParameterfvNV =
        (PFNGLCOMBINERSTAGEPARAMETERFVNVPROC)
        GET_GL_PROC_ADDRESS("glCombinerStageParameterfvNV");
    EXTglGetCombinerStageParameterfvNV =
        (PFNGLGETCOMBINERSTAGEPARAMETERFVNVPROC)
        GET_GL_PROC_ADDRESS("glGetCombinerStageParameterfvNV");
#endif
}


void InitExtVertexProgram()
{
#if defined(GET_GL_PROC_ADDRESS)
    EXTglAreProgramsResidentNV =
        (PFNGLAREPROGRAMSRESIDENTNVPROC)
        GET_GL_PROC_ADDRESS("glAreProgramsResidentNV");
    EXTglBindProgramNV =
        (PFNGLBINDPROGRAMNVPROC)
        GET_GL_PROC_ADDRESS("glBindProgramNV");
    EXTglDeleteProgramsNV =
        (PFNGLDELETEPROGRAMSNVPROC)
        GET_GL_PROC_ADDRESS("glDeleteProgramsNV");
    EXTglExecuteProgramNV =
        (PFNGLEXECUTEPROGRAMNVPROC)
        GET_GL_PROC_ADDRESS("glExecuteProgramNV");
    EXTglGenProgramsNV =
        (PFNGLGENPROGRAMSNVPROC)
        GET_GL_PROC_ADDRESS("glGenProgramsNV");
    EXTglGetProgramParameterdvNV =
        (PFNGLGETPROGRAMPARAMETERDVNVPROC)
        GET_GL_PROC_ADDRESS("glGetProgramParameterdvNV");
    EXTglGetProgramParameterfvNV =
        (PFNGLGETPROGRAMPARAMETERFVNVPROC)
        GET_GL_PROC_ADDRESS("glGetProgramParameterfvNV");
    EXTglGetProgramivNV =
        (PFNGLGETPROGRAMIVNVPROC)
        GET_GL_PROC_ADDRESS("glGetProgramivNV");
    EXTglGetProgramStringNV =
        (PFNGLGETPROGRAMSTRINGNVPROC)
        GET_GL_PROC_ADDRESS("glGetProgramStringNV");
    EXTglGetTrackMatrixivNV =
        (PFNGLGETTRACKMATRIXIVNVPROC)
        GET_GL_PROC_ADDRESS("glGetTrackMatrixivNV");
    EXTglGetVertexAttribdvNV =
        (PFNGLGETVERTEXATTRIBDVNVPROC)
        GET_GL_PROC_ADDRESS("glGetVertexAttribdvNV");
    EXTglGetVertexAttribfvNV =
        (PFNGLGETVERTEXATTRIBFVNVPROC)
        GET_GL_PROC_ADDRESS("glGetVertexAttribfvNV");
    EXTglGetVertexAttribivNV =
        (PFNGLGETVERTEXATTRIBIVNVPROC)
        GET_GL_PROC_ADDRESS("glGetVertexAttribivNV");
    EXTglGetVertexAttribPointervNV =
        (PFNGLGETVERTEXATTRIBPOINTERVNVPROC)
        GET_GL_PROC_ADDRESS("glGetVertexAttribPointervNV");
    EXTglIsProgramNV =
        (PFNGLISPROGRAMNVPROC)
        GET_GL_PROC_ADDRESS("glIsProgramNV");
    EXTglLoadProgramNV =
        (PFNGLLOADPROGRAMNVPROC)
        GET_GL_PROC_ADDRESS("glLoadProgramNV");
    EXTglProgramParameter4dNV =
        (PFNGLPROGRAMPARAMETER4DNVPROC)
        GET_GL_PROC_ADDRESS("glProgramParameter4dNV");
    EXTglProgramParameter4dvNV =
        (PFNGLPROGRAMPARAMETER4DVNVPROC)
        GET_GL_PROC_ADDRESS("glProgramParameter4dvNV");
    EXTglProgramParameter4fNV =
        (PFNGLPROGRAMPARAMETER4FNVPROC)
        GET_GL_PROC_ADDRESS("glProgramParameter4fNV");
    EXTglProgramParameter4fvNV =
        (PFNGLPROGRAMPARAMETER4FVNVPROC)
        GET_GL_PROC_ADDRESS("glProgramParameter4fvNV");
    EXTglProgramParameters4dvNV =
        (PFNGLPROGRAMPARAMETERS4DVNVPROC)
        GET_GL_PROC_ADDRESS("glProgramParameters4dvNV");
    EXTglProgramParameters4fvNV =
        (PFNGLPROGRAMPARAMETERS4FVNVPROC)
        GET_GL_PROC_ADDRESS("glProgramParameters4fvNV");
    EXTglRequestResidentProgramsNV =
        (PFNGLREQUESTRESIDENTPROGRAMSNVPROC)
        GET_GL_PROC_ADDRESS("glRequestResidentProgramsNV");
    EXTglTrackMatrixNV =
        (PFNGLTRACKMATRIXNVPROC)
        GET_GL_PROC_ADDRESS("glTrackMatrixNV");
    EXTglVertexAttribPointerNV =
        (PFNGLVERTEXATTRIBPOINTERNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribPointerNV");
    EXTglVertexAttrib1dNV =
        (PFNGLVERTEXATTRIB1DNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib1dNV");
    EXTglVertexAttrib1dvNV =
        (PFNGLVERTEXATTRIB1DVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib1dvNV");
    EXTglVertexAttrib1fNV =
        (PFNGLVERTEXATTRIB1FNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib1fNV");
    EXTglVertexAttrib1fvNV =
        (PFNGLVERTEXATTRIB1FVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib1fvNV");
    EXTglVertexAttrib1sNV =
        (PFNGLVERTEXATTRIB1SNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib1sNV");
    EXTglVertexAttrib1svNV =
        (PFNGLVERTEXATTRIB1SVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib1svNV");
    EXTglVertexAttrib2dNV =
        (PFNGLVERTEXATTRIB2DNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib2dNV");
    EXTglVertexAttrib2dvNV =
        (PFNGLVERTEXATTRIB2DVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib2dvNV");
    EXTglVertexAttrib2fNV =
        (PFNGLVERTEXATTRIB2FNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib2fNV");
    EXTglVertexAttrib2fvNV =
        (PFNGLVERTEXATTRIB2FVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib2fvNV");
    EXTglVertexAttrib2sNV =
        (PFNGLVERTEXATTRIB2SNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib2sNV");
    EXTglVertexAttrib2svNV =
        (PFNGLVERTEXATTRIB2SVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib2svNV");
    EXTglVertexAttrib3dNV =
        (PFNGLVERTEXATTRIB3DNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib3dNV");
    EXTglVertexAttrib3dvNV =
        (PFNGLVERTEXATTRIB3DVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib3dvNV");
    EXTglVertexAttrib3fNV =
        (PFNGLVERTEXATTRIB3FNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib3fNV");
    EXTglVertexAttrib3fvNV =
        (PFNGLVERTEXATTRIB3FVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib3fvNV");
    EXTglVertexAttrib3sNV =
        (PFNGLVERTEXATTRIB3SNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib3sNV");
    EXTglVertexAttrib3svNV =
        (PFNGLVERTEXATTRIB3SVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib3svNV");
    EXTglVertexAttrib4dNV =
        (PFNGLVERTEXATTRIB4DNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib4dNV");
    EXTglVertexAttrib4dvNV =
        (PFNGLVERTEXATTRIB4DVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib4dvNV");
    EXTglVertexAttrib4fNV =
        (PFNGLVERTEXATTRIB4FNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib4fNV");
    EXTglVertexAttrib4fvNV =
        (PFNGLVERTEXATTRIB4FVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib4fvNV");
    EXTglVertexAttrib4sNV =
        (PFNGLVERTEXATTRIB4SNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib4sNV");
    EXTglVertexAttrib4svNV =
        (PFNGLVERTEXATTRIB4SVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib4svNV");
    EXTglVertexAttrib4ubvNV =
        (PFNGLVERTEXATTRIB4UBVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib4ubvNV");
    EXTglVertexAttribs1dvNV =
        (PFNGLVERTEXATTRIBS1DVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs1dvNV");
    EXTglVertexAttribs1fvNV =
        (PFNGLVERTEXATTRIBS1FVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs1fvNV");
    EXTglVertexAttribs1svNV =
        (PFNGLVERTEXATTRIBS1SVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs1svNV");
    EXTglVertexAttribs2dvNV =
        (PFNGLVERTEXATTRIBS2DVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs2dvNV");
    EXTglVertexAttribs2fvNV =
        (PFNGLVERTEXATTRIBS2FVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs2fvNV");
    EXTglVertexAttribs2svNV =
        (PFNGLVERTEXATTRIBS2SVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs2svNV");
    EXTglVertexAttribs3dvNV =
        (PFNGLVERTEXATTRIBS3DVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs3dvNV");
    EXTglVertexAttribs3fvNV =
        (PFNGLVERTEXATTRIBS3FVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs3fvNV");
    EXTglVertexAttribs3svNV =
        (PFNGLVERTEXATTRIBS3SVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs3svNV");
    EXTglVertexAttribs4dvNV =
        (PFNGLVERTEXATTRIBS4DVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs4dvNV");
    EXTglVertexAttribs4fvNV =
        (PFNGLVERTEXATTRIBS4FVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs4fvNV");
    EXTglVertexAttribs4svNV =
        (PFNGLVERTEXATTRIBS4SVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs4svNV");
    EXTglVertexAttribs4ubvNV =
        (PFNGLVERTEXATTRIBS4UBVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs4ubvNV");
#endif
}


void InitExtPalettedTexture()
{
#if defined(GET_GL_PROC_ADDRESS)
    EXTglColorTableEXT = (PFNGLCOLORTABLEEXTPROC) GET_GL_PROC_ADDRESS("glColorTableEXT");
#endif
}


void InitExtBlendMinmax()
{
#if defined(GET_GL_PROC_ADDRESS)
    EXTglBlendEquationEXT = (PFNGLBLENDEQUATIONEXTPROC) GET_GL_PROC_ADDRESS("glBlendEquationEXT");
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
