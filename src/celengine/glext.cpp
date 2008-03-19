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
#ifndef TARGET_OS_MAC
#include "GL/glx.h"
#include <dlfcn.h>
#endif /* ! TARGET_OS_MAC */
#endif /* ! _WIN32 */

#include "glext.h"

// ARB_texture_compression
glx::PFNGLCOMPRESSEDTEXIMAGE3DARBPROC glx::glCompressedTexImage3DARB;
glx::PFNGLCOMPRESSEDTEXIMAGE2DARBPROC glx::glCompressedTexImage2DARB;
glx::PFNGLCOMPRESSEDTEXIMAGE1DARBPROC glx::glCompressedTexImage1DARB;
glx::PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC glx::glCompressedTexSubImage3DARB;
glx::PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC glx::glCompressedTexSubImage2DARB;
glx::PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC glx::glCompressedTexSubImage1DARB;

// ARB_multitexture command function pointers
glx::PFNGLMULTITEXCOORD2IARBPROC glx::glMultiTexCoord2iARB;
glx::PFNGLMULTITEXCOORD2FARBPROC glx::glMultiTexCoord2fARB;
glx::PFNGLMULTITEXCOORD3FARBPROC glx::glMultiTexCoord3fARB;
glx::PFNGLMULTITEXCOORD3FVARBPROC glx::glMultiTexCoord3fvARB;
glx::PFNGLACTIVETEXTUREARBPROC glx::glActiveTextureARB;
glx::PFNGLCLIENTACTIVETEXTUREARBPROC glx::glClientActiveTextureARB;

// NV_register_combiners command function pointers
glx::PFNGLCOMBINERPARAMETERFVNVPROC glx::glCombinerParameterfvNV;
glx::PFNGLCOMBINERPARAMETERIVNVPROC glx::glCombinerParameterivNV;
glx::PFNGLCOMBINERPARAMETERFNVPROC glx::glCombinerParameterfNV;
glx::PFNGLCOMBINERPARAMETERINVPROC glx::glCombinerParameteriNV;
glx::PFNGLCOMBINERINPUTNVPROC glx::glCombinerInputNV;
glx::PFNGLCOMBINEROUTPUTNVPROC glx::glCombinerOutputNV;
glx::PFNGLFINALCOMBINERINPUTNVPROC glx::glFinalCombinerInputNV;
glx::PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC glx::glGetCombinerInputParameterfvNV;
glx::PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC glx::glGetCombinerInputParameterivNV;
glx::PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC glx::glGetCombinerOutputParameterfvNV;
glx::PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC glx::glGetCombinerOutputParameterivNV;
glx::PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC glx::glGetFinalCombinerInputParameterfvNV;
glx::PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC glx::glGetFinalCombinerInputParameterivNV;

// NV_register_combiners2 command function pointers
glx::PFNGLCOMBINERSTAGEPARAMETERFVNVPROC glx::glCombinerStageParameterfvNV;
glx::PFNGLGETCOMBINERSTAGEPARAMETERFVNVPROC glx::glGetCombinerStageParameterfvNV;

// NV_vertex_program function pointers
glx::PFNGLAREPROGRAMSRESIDENTNVPROC glx::glAreProgramsResidentNV ;
glx::PFNGLBINDPROGRAMNVPROC glx::glBindProgramNV ;
glx::PFNGLDELETEPROGRAMSNVPROC glx::glDeleteProgramsNV ;
glx::PFNGLEXECUTEPROGRAMNVPROC glx::glExecuteProgramNV ;
glx::PFNGLGENPROGRAMSNVPROC glx::glGenProgramsNV ;
glx::PFNGLGETPROGRAMPARAMETERDVNVPROC glx::glGetProgramParameterdvNV ;
glx::PFNGLGETPROGRAMPARAMETERFVNVPROC glx::glGetProgramParameterfvNV ;
glx::PFNGLGETPROGRAMIVNVPROC glx::glGetProgramivNV ;
glx::PFNGLGETPROGRAMSTRINGNVPROC glx::glGetProgramStringNV ;
glx::PFNGLGETTRACKMATRIXIVNVPROC glx::glGetTrackMatrixivNV ;
glx::PFNGLGETVERTEXATTRIBDVNVPROC glx::glGetVertexAttribdvNV ;
glx::PFNGLGETVERTEXATTRIBFVNVPROC glx::glGetVertexAttribfvNV ;
glx::PFNGLGETVERTEXATTRIBIVNVPROC glx::glGetVertexAttribivNV ;
glx::PFNGLGETVERTEXATTRIBPOINTERVNVPROC glx::glGetVertexAttribPointervNV ;
glx::PFNGLISPROGRAMNVPROC glx::glIsProgramNV ;
glx::PFNGLLOADPROGRAMNVPROC glx::glLoadProgramNV ;
glx::PFNGLPROGRAMPARAMETER4DNVPROC glx::glProgramParameter4dNV ;
glx::PFNGLPROGRAMPARAMETER4DVNVPROC glx::glProgramParameter4dvNV ;
glx::PFNGLPROGRAMPARAMETER4FNVPROC glx::glProgramParameter4fNV ;
glx::PFNGLPROGRAMPARAMETER4FVNVPROC glx::glProgramParameter4fvNV ;
glx::PFNGLPROGRAMPARAMETERS4DVNVPROC glx::glProgramParameters4dvNV ;
glx::PFNGLPROGRAMPARAMETERS4FVNVPROC glx::glProgramParameters4fvNV ;
glx::PFNGLREQUESTRESIDENTPROGRAMSNVPROC glx::glRequestResidentProgramsNV ;
glx::PFNGLTRACKMATRIXNVPROC glx::glTrackMatrixNV ;
glx::PFNGLVERTEXATTRIBPOINTERNVPROC glx::glVertexAttribPointerNV ;
glx::PFNGLVERTEXATTRIB1DNVPROC glx::glVertexAttrib1dNV ;
glx::PFNGLVERTEXATTRIB1DVNVPROC glx::glVertexAttrib1dvNV ;
glx::PFNGLVERTEXATTRIB1FNVPROC glx::glVertexAttrib1fNV ;
glx::PFNGLVERTEXATTRIB1FVNVPROC glx::glVertexAttrib1fvNV ;
glx::PFNGLVERTEXATTRIB1SNVPROC glx::glVertexAttrib1sNV ;
glx::PFNGLVERTEXATTRIB1SVNVPROC glx::glVertexAttrib1svNV ;
glx::PFNGLVERTEXATTRIB2DNVPROC glx::glVertexAttrib2dNV ;
glx::PFNGLVERTEXATTRIB2DVNVPROC glx::glVertexAttrib2dvNV ;
glx::PFNGLVERTEXATTRIB2FNVPROC glx::glVertexAttrib2fNV ;
glx::PFNGLVERTEXATTRIB2FVNVPROC glx::glVertexAttrib2fvNV ;
glx::PFNGLVERTEXATTRIB2SNVPROC glx::glVertexAttrib2sNV ;
glx::PFNGLVERTEXATTRIB2SVNVPROC glx::glVertexAttrib2svNV ;
glx::PFNGLVERTEXATTRIB3DNVPROC glx::glVertexAttrib3dNV ;
glx::PFNGLVERTEXATTRIB3DVNVPROC glx::glVertexAttrib3dvNV ;
glx::PFNGLVERTEXATTRIB3FNVPROC glx::glVertexAttrib3fNV ;
glx::PFNGLVERTEXATTRIB3FVNVPROC glx::glVertexAttrib3fvNV ;
glx::PFNGLVERTEXATTRIB3SNVPROC glx::glVertexAttrib3sNV ;
glx::PFNGLVERTEXATTRIB3SVNVPROC glx::glVertexAttrib3svNV ;
glx::PFNGLVERTEXATTRIB4DNVPROC glx::glVertexAttrib4dNV ;
glx::PFNGLVERTEXATTRIB4DVNVPROC glx::glVertexAttrib4dvNV ;
glx::PFNGLVERTEXATTRIB4FNVPROC glx::glVertexAttrib4fNV ;
glx::PFNGLVERTEXATTRIB4FVNVPROC glx::glVertexAttrib4fvNV ;
glx::PFNGLVERTEXATTRIB4SNVPROC glx::glVertexAttrib4sNV ;
glx::PFNGLVERTEXATTRIB4SVNVPROC glx::glVertexAttrib4svNV ;
glx::PFNGLVERTEXATTRIB4UBVNVPROC glx::glVertexAttrib4ubvNV ;
glx::PFNGLVERTEXATTRIBS1DVNVPROC glx::glVertexAttribs1dvNV ;
glx::PFNGLVERTEXATTRIBS1FVNVPROC glx::glVertexAttribs1fvNV ;
glx::PFNGLVERTEXATTRIBS1SVNVPROC glx::glVertexAttribs1svNV ;
glx::PFNGLVERTEXATTRIBS2DVNVPROC glx::glVertexAttribs2dvNV ;
glx::PFNGLVERTEXATTRIBS2FVNVPROC glx::glVertexAttribs2fvNV ;
glx::PFNGLVERTEXATTRIBS2SVNVPROC glx::glVertexAttribs2svNV ;
glx::PFNGLVERTEXATTRIBS3DVNVPROC glx::glVertexAttribs3dvNV ;
glx::PFNGLVERTEXATTRIBS3FVNVPROC glx::glVertexAttribs3fvNV ;
glx::PFNGLVERTEXATTRIBS3SVNVPROC glx::glVertexAttribs3svNV ;
glx::PFNGLVERTEXATTRIBS4DVNVPROC glx::glVertexAttribs4dvNV ;
glx::PFNGLVERTEXATTRIBS4FVNVPROC glx::glVertexAttribs4fvNV ;
glx::PFNGLVERTEXATTRIBS4SVNVPROC glx::glVertexAttribs4svNV ;
glx::PFNGLVERTEXATTRIBS4UBVNVPROC glx::glVertexAttribs4ubvNV ;

// EXT_paletted_texture command function pointers
glx::PFNGLCOLORTABLEEXTPROC glx::glColorTableEXT;

// EXT_blend_minmax command function pointers
glx::PFNGLBLENDEQUATIONEXTPROC glx::glBlendEquationEXT;

// WGL_EXT_swap_control command function pointers
glx::PFNWGLSWAPINTERVALEXTPROC glx::wglSwapIntervalEXT;
glx::PFNWGLGETSWAPINTERVALEXTPROC glx::wglGetSwapIntervalEXT;

// GLX_SGI_video_sync command function pointers
glx::PFNGLXGETVIDEOSYNCSGIPROC glx::glXGetVideoSyncSGI;
glx::PFNGLXWAITVIDEOSYNCSGIPROC glx::glXWaitVideoSyncSGI;
glx::PFNGLXGETREFRESHRATESGIPROC glx::glXGetRefreshRateSGI;

// ARB_vertex_program function pointers
glx::PFNGLBINDPROGRAMARBPROC glx::glBindProgramARB;
glx::PFNGLDELETEPROGRAMSARBPROC glx::glDeleteProgramsARB;
glx::PFNGLGENPROGRAMSARBPROC glx::glGenProgramsARB;
glx::PFNGLISPROGRAMARBPROC glx::glIsProgramARB;
glx::PFNGLVERTEXATTRIB1SARBPROC glx::glVertexAttrib1sARB;
glx::PFNGLVERTEXATTRIB1FARBPROC glx::glVertexAttrib1fARB;
glx::PFNGLVERTEXATTRIB1DARBPROC glx::glVertexAttrib1dARB;
glx::PFNGLVERTEXATTRIB2SARBPROC glx::glVertexAttrib2sARB;
glx::PFNGLVERTEXATTRIB2FARBPROC glx::glVertexAttrib2fARB;
glx::PFNGLVERTEXATTRIB2DARBPROC glx::glVertexAttrib2dARB;
glx::PFNGLVERTEXATTRIB3SARBPROC glx::glVertexAttrib3sARB;
glx::PFNGLVERTEXATTRIB3FARBPROC glx::glVertexAttrib3fARB;
glx::PFNGLVERTEXATTRIB3DARBPROC glx::glVertexAttrib3dARB;
glx::PFNGLVERTEXATTRIB4SARBPROC glx::glVertexAttrib4sARB;
glx::PFNGLVERTEXATTRIB4FARBPROC glx::glVertexAttrib4fARB;
glx::PFNGLVERTEXATTRIB4DARBPROC glx::glVertexAttrib4dARB;
glx::PFNGLVERTEXATTRIB4NUBARBPROC glx::glVertexAttrib4NubARB;
glx::PFNGLVERTEXATTRIB1SVARBPROC glx::glVertexAttrib1svARB;
glx::PFNGLVERTEXATTRIB1FVARBPROC glx::glVertexAttrib1fvARB;
glx::PFNGLVERTEXATTRIB1DVARBPROC glx::glVertexAttrib1dvARB;
glx::PFNGLVERTEXATTRIB2SVARBPROC glx::glVertexAttrib2svARB;
glx::PFNGLVERTEXATTRIB2FVARBPROC glx::glVertexAttrib2fvARB;
glx::PFNGLVERTEXATTRIB2DVARBPROC glx::glVertexAttrib2dvARB;
glx::PFNGLVERTEXATTRIB3SVARBPROC glx::glVertexAttrib3svARB;
glx::PFNGLVERTEXATTRIB3FVARBPROC glx::glVertexAttrib3fvARB;
glx::PFNGLVERTEXATTRIB3DVARBPROC glx::glVertexAttrib3dvARB;
glx::PFNGLVERTEXATTRIB4BVARBPROC glx::glVertexAttrib4bvARB;
glx::PFNGLVERTEXATTRIB4SVARBPROC glx::glVertexAttrib4svARB;
glx::PFNGLVERTEXATTRIB4IVARBPROC glx::glVertexAttrib4ivARB;
glx::PFNGLVERTEXATTRIB4UBVARBPROC glx::glVertexAttrib4ubvARB;
glx::PFNGLVERTEXATTRIB4USVARBPROC glx::glVertexAttrib4usvARB;
glx::PFNGLVERTEXATTRIB4UIVARBPROC glx::glVertexAttrib4uivARB;
glx::PFNGLVERTEXATTRIB4FVARBPROC glx::glVertexAttrib4fvARB;
glx::PFNGLVERTEXATTRIB4DVARBPROC glx::glVertexAttrib4dvARB;
glx::PFNGLVERTEXATTRIB4NBVARBPROC glx::glVertexAttrib4NbvARB;
glx::PFNGLVERTEXATTRIB4NSVARBPROC glx::glVertexAttrib4NsvARB;
glx::PFNGLVERTEXATTRIB4NIVARBPROC glx::glVertexAttrib4NivARB;
glx::PFNGLVERTEXATTRIB4NUBVARBPROC glx::glVertexAttrib4NubvARB;
glx::PFNGLVERTEXATTRIB4NUSVARBPROC glx::glVertexAttrib4NusvARB;
glx::PFNGLVERTEXATTRIB4NUIVARBPROC glx::glVertexAttrib4NuivARB;
glx::PFNGLVERTEXATTRIBPOINTERARBPROC glx::glVertexAttribPointerARB;
glx::PFNGLENABLEVERTEXATTRIBARRAYARBPROC glx::glEnableVertexAttribArrayARB;
glx::PFNGLDISABLEVERTEXATTRIBARRAYARBPROC glx::glDisableVertexAttribArrayARB;
glx::PFNGLGETVERTEXATTRIBDVARBPROC glx::glGetVertexAttribdvARB;
glx::PFNGLGETVERTEXATTRIBFVARBPROC glx::glGetVertexAttribfvARB;
glx::PFNGLGETVERTEXATTRIBIVARBPROC glx::glGetVertexAttribivARB;
glx::PFNGLGETVERTEXATTRIBPOINTERVARBPROC glx::glGetVertexAttribPointervARB;
glx::PFNGLPROGRAMENVPARAMETER4DARBPROC glx::glProgramEnvParameter4dARB;
glx::PFNGLPROGRAMENVPARAMETER4DVARBPROC glx::glProgramEnvParameter4dvARB;
glx::PFNGLPROGRAMENVPARAMETER4FARBPROC glx::glProgramEnvParameter4fARB;
glx::PFNGLPROGRAMENVPARAMETER4FVARBPROC glx::glProgramEnvParameter4fvARB;
glx::PFNGLPROGRAMLOCALPARAMETER4DARBPROC glx::glProgramLocalParameter4dARB;
glx::PFNGLPROGRAMLOCALPARAMETER4DVARBPROC glx::glProgramLocalParameter4dvARB;
glx::PFNGLPROGRAMLOCALPARAMETER4FARBPROC glx::glProgramLocalParameter4fARB;
glx::PFNGLPROGRAMLOCALPARAMETER4FVARBPROC glx::glProgramLocalParameter4fvARB;
glx::PFNGLGETPROGRAMENVPARAMETERDVARBPROC glx::glGetProgramEnvParameterdvARB;
glx::PFNGLGETPROGRAMENVPARAMETERFVARBPROC glx::glGetProgramEnvParameterfvARB;
glx::PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC glx::glGetProgramLocalParameterdvARB;
glx::PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC glx::glGetProgramLocalParameterfvARB;
glx::PFNGLPROGRAMSTRINGARBPROC glx::glProgramStringARB;
glx::PFNGLGETPROGRAMSTRINGARBPROC glx::glGetProgramStringARB;
glx::PFNGLGETPROGRAMIVARBPROC glx::glGetProgramivARB;

// NV_fragment_program function pointers
glx::PFNGLPROGRAMNAMEDPARAMETER4FNVPROC    glx::glProgramNamedParameter4fNV;
glx::PFNGLPROGRAMNAMEDPARAMETER4DNVPROC    glx::glProgramNamedParameter4dNV;
glx::PFNGLPROGRAMNAMEDPARAMETER4FVNVPROC   glx::glProgramNamedParameter4fvNV;
glx::PFNGLPROGRAMNAMEDPARAMETER4DVNVPROC   glx::glProgramNamedParameter4dvNV;
glx::PFNGLGETPROGRAMNAMEDPARAMETERFVNVPROC glx::glGetProgramNamedParameterfvNV;
glx::PFNGLGETPROGRAMNAMEDPARAMETERDVNVPROC glx::glGetProgramNamedParameterdvNV;
glx::PFNGLPROGRAMLOCALPARAMETER4FNVPROC    glx::glProgramLocalParameter4fNV;
glx::PFNGLPROGRAMLOCALPARAMETER4DNVPROC    glx::glProgramLocalParameter4dNV;
glx::PFNGLPROGRAMLOCALPARAMETER4FVNVPROC   glx::glProgramLocalParameter4fvNV;
glx::PFNGLPROGRAMLOCALPARAMETER4DVNVPROC   glx::glProgramLocalParameter4dvNV;
glx::PFNGLGETPROGRAMLOCALPARAMETERFVNVPROC glx::glGetProgramLocalParameterfvNV;
glx::PFNGLGETPROGRAMLOCALPARAMETERDVNVPROC glx::glGetProgramLocalParameterdvNV;

// ARB_vertex_buffer_object function pointers
glx::PFNGLBINDBUFFERARBPROC                glx::glBindBufferARB;
glx::PFNGLDELETEBUFFERSARBPROC             glx::glDeleteBuffersARB;
glx::PFNGLGENBUFFERSARBPROC                glx::glGenBuffersARB;
glx::PFNGLISBUFFERARBPROC                  glx::glIsBufferARB;
glx::PFNGLBUFFERDATAARBPROC                glx::glBufferDataARB;
glx::PFNGLBUFFERSUBDATAARBPROC             glx::glBufferSubDataARB;
glx::PFNGLGETBUFFERSUBDATAARBPROC          glx::glGetBufferSubDataARB;
glx::PFNGLMAPBUFFERARBPROC                 glx::glMapBufferARB;
glx::PFNGLUNMAPBUFFERARBPROC               glx::glUnmapBufferARB;
glx::PFNGLGETBUFFERPARAMETERIVARBPROC      glx::glGetBufferParameterivARB;
glx::PFNGLGETBUFFERPOINTERVARBPROC         glx::glGetBufferPointervARB;

// ARB_vertex_shader function pointers
glx::PFNGLBINDATTRIBLOCATIONARBPROC        glx::glBindAttribLocationARB;
glx::PFNGLGETACTIVEATTRIBARBPROC           glx::glGetActiveAttribARB;
glx::PFNGLGETATTRIBLOCATIONARBPROC         glx::glGetAttribLocationARB;

// ARB_shader_objects function pointers
glx::PFNGLDELETEOBJECTARBPROC              glx::glDeleteObjectARB;
glx::PFNGLGETHANDLEARBPROC                 glx::glGetHandleARB;
glx::PFNGLDETACHOBJECTARBPROC              glx::glDetachObjectARB;
glx::PFNGLCREATESHADEROBJECTARBPROC        glx::glCreateShaderObjectARB;
glx::PFNGLSHADERSOURCEARBPROC              glx::glShaderSourceARB;
glx::PFNGLCOMPILESHADERARBPROC             glx::glCompileShaderARB;
glx::PFNGLCREATEPROGRAMOBJECTARBPROC       glx::glCreateProgramObjectARB;
glx::PFNGLATTACHOBJECTARBPROC              glx::glAttachObjectARB;
glx::PFNGLLINKPROGRAMARBPROC               glx::glLinkProgramARB;
glx::PFNGLUSEPROGRAMOBJECTARBPROC          glx::glUseProgramObjectARB;
glx::PFNGLVALIDATEPROGRAMARBPROC           glx::glValidateProgramARB;
glx::PFNGLUNIFORM1FARBPROC                 glx::glUniform1fARB;
glx::PFNGLUNIFORM2FARBPROC                 glx::glUniform2fARB;
glx::PFNGLUNIFORM3FARBPROC                 glx::glUniform3fARB;
glx::PFNGLUNIFORM4FARBPROC                 glx::glUniform4fARB;
glx::PFNGLUNIFORM1IARBPROC                 glx::glUniform1iARB;
glx::PFNGLUNIFORM2IARBPROC                 glx::glUniform2iARB;
glx::PFNGLUNIFORM3IARBPROC                 glx::glUniform3iARB;
glx::PFNGLUNIFORM4IARBPROC                 glx::glUniform4iARB;
glx::PFNGLUNIFORM1FVARBPROC                glx::glUniform1fvARB;
glx::PFNGLUNIFORM2FVARBPROC                glx::glUniform2fvARB;
glx::PFNGLUNIFORM3FVARBPROC                glx::glUniform3fvARB;
glx::PFNGLUNIFORM4FVARBPROC                glx::glUniform4fvARB;
glx::PFNGLUNIFORM1IVARBPROC                glx::glUniform1ivARB;
glx::PFNGLUNIFORM2IVARBPROC                glx::glUniform2ivARB;
glx::PFNGLUNIFORM3IVARBPROC                glx::glUniform3ivARB;
glx::PFNGLUNIFORM4IVARBPROC                glx::glUniform4ivARB;
glx::PFNGLUNIFORMMATRIX2FVARBPROC          glx::glUniformMatrix2fvARB;
glx::PFNGLUNIFORMMATRIX3FVARBPROC          glx::glUniformMatrix3fvARB;
glx::PFNGLUNIFORMMATRIX4FVARBPROC          glx::glUniformMatrix4fvARB;
glx::PFNGLGETOBJECTPARAMETERFVARBPROC      glx::glGetObjectParameterfvARB;
glx::PFNGLGETOBJECTPARAMETERIVARBPROC      glx::glGetObjectParameterivARB;
glx::PFNGLGETINFOLOGARBPROC                glx::glGetInfoLogARB;
glx::PFNGLGETATTACHEDOBJECTSARBPROC        glx::glGetAttachedObjectsARB;
glx::PFNGLGETUNIFORMLOCATIONARBPROC        glx::glGetUniformLocationARB;
glx::PFNGLGETACTIVEUNIFORMARBPROC          glx::glGetActiveUniformARB;
glx::PFNGLGETUNIFORMFVARBPROC              glx::glGetUniformfvARB;
glx::PFNGLGETUNIFORMIVARBPROC              glx::glGetUniformivARB;
glx::PFNGLGETSHADERSOURCEARBPROC           glx::glGetShaderSourceARB;

// ARB_color_buffer_float
glx::PFNGLCLAMPCOLORARBPROC                glx::glClampColorARB;

// EXT_framebuffer_object
glx::PFNGLISRENDERBUFFEREXTPROC                       glx::glIsRenderbufferEXT;
glx::PFNGLBINDRENDERBUFFEREXTPROC                     glx::glBindRenderbufferEXT;
glx::PFNGLDELETERENDERBUFFERSEXTPROC                  glx::glDeleteRenderbuffersEXT;
glx::PFNGLGENRENDERBUFFERSEXTPROC                     glx::glGenRenderbuffersEXT;
glx::PFNGLRENDERBUFFERSTORAGEEXTPROC                  glx::glRenderbufferStorageEXT;
glx::PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC           glx::glGetRenderbufferParameterivEXT;
glx::PFNGLISFRAMEBUFFEREXTPROC                        glx::glIsFramebufferEXT;
glx::PFNGLBINDFRAMEBUFFEREXTPROC                      glx::glBindFramebufferEXT;
glx::PFNGLDELETEFRAMEBUFFERSEXTPROC                   glx::glDeleteFramebuffersEXT;
glx::PFNGLGENFRAMEBUFFERSEXTPROC                      glx::glGenFramebuffersEXT;
glx::PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC               glx::glCheckFramebufferStatusEXT;
glx::PFNGLFRAMEBUFFERTEXTURE1DEXTPROC                 glx::glFramebufferTexture1DEXT;
glx::PFNGLFRAMEBUFFERTEXTURE2DEXTPROC                 glx::glFramebufferTexture2DEXT;
glx::PFNGLFRAMEBUFFERTEXTURE3DEXTPROC                 glx::glFramebufferTexture3DEXT;
glx::PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC              glx::glFramebufferRenderbufferEXT;
glx::PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC  glx::glGetFramebufferAttachmentParameterivEXT;
glx::PFNGLGENERATEMIPMAPEXTPROC                       glx::glGenerateMipmapEXT;


// extern void Alert(const char *szFormat, ...);

#if defined(QT_OPENGL_LIB) && defined(_WIN32) && !defined(_MSC_VER)

// TODO: We should use something other than a global GL context
#include <QGLContext>
extern QGLContext* glctx;
#define GET_GL_PROC_ADDRESS(name) glctx->getProcAddress(name)

#else

#ifndef TARGET_OS_MAC
#if defined(_WIN32)

#define GET_GL_PROC_ADDRESS(name) wglGetProcAddress(name)

#else

#ifdef GLX_VERSION_1_4
extern "C" {
extern void (*glXGetProcAddressARB(const GLubyte *procName))();
}
#define GET_GL_PROC_ADDRESS(name) GetGLProcAddress((GLubyte*) (name))
#else
#define GET_GL_PROC_ADDRESS(name) GetGLProcAddress((GLubyte*) (name))
#endif

typedef void (*FUNCS) (void);

// Works around an apparent problem in ATI's libGL.so where glXGetProcAddress
// returns NULL.
FUNCS GetGLProcAddress(const GLubyte* procName)
{
    FUNCS func = glXGetProcAddressARB(procName);
    if (!func)
    {
        void* libGL = dlopen(0, RTLD_LAZY);
        func = (FUNCS) dlsym(libGL, (const char*) procName);
        dlclose(libGL);
    }

    return func;
}

#endif // defined(WIN32)
#endif // !TARGET_OS_MAC

#endif // QT_CORE_LIB


#ifdef TARGET_OS_MAC
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
#endif /* TARGET_OS_MAC */


void Alert(const char /* *szFormat*/, ...)
{
}


// ARB_multitexture
static void InitExt_ARB_multitexture()
{
// #ifndef GL_ARB_multitexture
#ifdef GET_GL_PROC_ADDRESS
    glx::glMultiTexCoord2iARB = (glx::PFNGLMULTITEXCOORD2IARBPROC) GET_GL_PROC_ADDRESS("glMultiTexCoord2iARB");
    glx::glMultiTexCoord2fARB = (glx::PFNGLMULTITEXCOORD2FARBPROC) GET_GL_PROC_ADDRESS("glMultiTexCoord2fARB");
    glx::glMultiTexCoord3fARB = (glx::PFNGLMULTITEXCOORD3FARBPROC) GET_GL_PROC_ADDRESS("glMultiTexCoord3fARB");
    glx::glMultiTexCoord3fvARB = (glx::PFNGLMULTITEXCOORD3FVARBPROC) GET_GL_PROC_ADDRESS("glMultiTexCoord3fvARB");
    glx::glActiveTextureARB = (glx::PFNGLACTIVETEXTUREARBPROC) GET_GL_PROC_ADDRESS("glActiveTextureARB");
    glx::glClientActiveTextureARB = (glx::PFNGLCLIENTACTIVETEXTUREARBPROC) GET_GL_PROC_ADDRESS("glClientActiveTextureARB");
#endif // GET_GL_PROC_ADDRESS
// #endif // GL_ARB_multitexture
}


static void InitExt_ARB_texture_compression()
{
#if defined(GET_GL_PROC_ADDRESS)
    glx::glCompressedTexImage3DARB =
        (glx::PFNGLCOMPRESSEDTEXIMAGE3DARBPROC)
        GET_GL_PROC_ADDRESS("glCompressedTexImage3DARB");
    glx::glCompressedTexImage2DARB =
        (glx::PFNGLCOMPRESSEDTEXIMAGE2DARBPROC)
        GET_GL_PROC_ADDRESS("glCompressedTexImage2DARB");
    glx::glCompressedTexImage1DARB =
        (glx::PFNGLCOMPRESSEDTEXIMAGE1DARBPROC)
        GET_GL_PROC_ADDRESS("glCompressedTexImage1DARB");
    glx::glCompressedTexSubImage3DARB =
        (glx::PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC)
        GET_GL_PROC_ADDRESS("glCompressedTexSubImage3DARB");
    glx::glCompressedTexSubImage2DARB =
        (glx::PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC)
        GET_GL_PROC_ADDRESS("glCompressedTexSubImage2DARB");
    glx::glCompressedTexSubImage1DARB =
        (glx::PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC)
        GET_GL_PROC_ADDRESS("glCompressedTexSubImage1DARB");
#endif // GL_ARB_texture_compression
}


// NV_register_combiners
static void InitExt_NV_register_combiners()
{
#if defined(GET_GL_PROC_ADDRESS)
  /* Retrieve all NV_register_combiners routines. */
  glx::glCombinerParameterfvNV = (glx::PFNGLCOMBINERPARAMETERFVNVPROC) GET_GL_PROC_ADDRESS("glCombinerParameterfvNV");
  glx::glCombinerParameterivNV = (glx::PFNGLCOMBINERPARAMETERIVNVPROC) GET_GL_PROC_ADDRESS("glCombinerParameterivNV");
  glx::glCombinerParameterfNV = (glx::PFNGLCOMBINERPARAMETERFNVPROC) GET_GL_PROC_ADDRESS("glCombinerParameterfNV");
  glx::glCombinerParameteriNV = (glx::PFNGLCOMBINERPARAMETERINVPROC) GET_GL_PROC_ADDRESS("glCombinerParameteriNV");
  glx::glCombinerInputNV = (glx::PFNGLCOMBINERINPUTNVPROC) GET_GL_PROC_ADDRESS("glCombinerInputNV");
  glx::glCombinerOutputNV = (glx::PFNGLCOMBINEROUTPUTNVPROC) GET_GL_PROC_ADDRESS("glCombinerOutputNV");
  glx::glFinalCombinerInputNV = (glx::PFNGLFINALCOMBINERINPUTNVPROC) GET_GL_PROC_ADDRESS("glFinalCombinerInputNV");
  glx::glGetCombinerInputParameterfvNV = (glx::PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC) GET_GL_PROC_ADDRESS("glGetCombinerInputParameterfvNV");
  glx::glGetCombinerInputParameterivNV = (glx::PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC) GET_GL_PROC_ADDRESS("glGetCombinerInputParameterivNV");
  glx::glGetCombinerOutputParameterfvNV = (glx::PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC) GET_GL_PROC_ADDRESS("glGetCombinerOutputParameterfvNV");
  glx::glGetCombinerOutputParameterivNV = (glx::PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC) GET_GL_PROC_ADDRESS("glGetCombinerOutputParameterivNV");
  glx::glGetFinalCombinerInputParameterfvNV = (glx::PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC) GET_GL_PROC_ADDRESS("glGetFinalCombinerInputParameterfvNV");
  glx::glGetFinalCombinerInputParameterivNV = (glx::PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC) GET_GL_PROC_ADDRESS("glGetFinalCombinerInputParameterivNV");
#endif // GET_GL_PROC_ADDRESS
}


static void InitExt_NV_register_combiners2()
{
#if defined(GET_GL_PROC_ADDRESS)
    /* Retrieve all NV_register_combiners routines. */
    glx::glCombinerStageParameterfvNV = (glx::PFNGLCOMBINERSTAGEPARAMETERFVNVPROC) GET_GL_PROC_ADDRESS("glCombinerStageParameterfvNV");
    glx::glGetCombinerStageParameterfvNV = (glx::PFNGLGETCOMBINERSTAGEPARAMETERFVNVPROC) GET_GL_PROC_ADDRESS("glGetCombinerStageParameterfvNV");
#endif
}


static void InitExt_NV_vertex_program()
{
#if defined(GET_GL_PROC_ADDRESS)
    glx::glAreProgramsResidentNV =
        (glx::PFNGLAREPROGRAMSRESIDENTNVPROC)
        GET_GL_PROC_ADDRESS("glAreProgramsResidentNV");
    glx::glBindProgramNV =
        (glx::PFNGLBINDPROGRAMNVPROC)
        GET_GL_PROC_ADDRESS("glBindProgramNV");
    glx::glDeleteProgramsNV =
        (glx::PFNGLDELETEPROGRAMSNVPROC)
        GET_GL_PROC_ADDRESS("glDeleteProgramsNV");
    glx::glExecuteProgramNV =
        (glx::PFNGLEXECUTEPROGRAMNVPROC)
        GET_GL_PROC_ADDRESS("glExecuteProgramNV");
    glx::glGenProgramsNV =
        (glx::PFNGLGENPROGRAMSNVPROC)
        GET_GL_PROC_ADDRESS("glGenProgramsNV");
    glx::glGetProgramParameterdvNV =
        (glx::PFNGLGETPROGRAMPARAMETERDVNVPROC)
        GET_GL_PROC_ADDRESS("glGetProgramParameterdvNV");
    glx::glGetProgramParameterfvNV =
        (glx::PFNGLGETPROGRAMPARAMETERFVNVPROC)
        GET_GL_PROC_ADDRESS("glGetProgramParameterfvNV");
    glx::glGetProgramivNV =
        (glx::PFNGLGETPROGRAMIVNVPROC)
        GET_GL_PROC_ADDRESS("glGetProgramivNV");
    glx::glGetProgramStringNV =
        (glx::PFNGLGETPROGRAMSTRINGNVPROC)
        GET_GL_PROC_ADDRESS("glGetProgramStringNV");
    glx::glGetTrackMatrixivNV =
        (glx::PFNGLGETTRACKMATRIXIVNVPROC)
        GET_GL_PROC_ADDRESS("glGetTrackMatrixivNV");
    glx::glGetVertexAttribdvNV =
        (glx::PFNGLGETVERTEXATTRIBDVNVPROC)
        GET_GL_PROC_ADDRESS("glGetVertexAttribdvNV");
    glx::glGetVertexAttribfvNV =
        (glx::PFNGLGETVERTEXATTRIBFVNVPROC)
        GET_GL_PROC_ADDRESS("glGetVertexAttribfvNV");
    glx::glGetVertexAttribivNV =
        (glx::PFNGLGETVERTEXATTRIBIVNVPROC)
        GET_GL_PROC_ADDRESS("glGetVertexAttribivNV");
    glx::glGetVertexAttribPointervNV =
        (glx::PFNGLGETVERTEXATTRIBPOINTERVNVPROC)
        GET_GL_PROC_ADDRESS("glGetVertexAttribPointervNV");
    glx::glIsProgramNV =
        (glx::PFNGLISPROGRAMNVPROC)
        GET_GL_PROC_ADDRESS("glIsProgramNV");
    glx::glLoadProgramNV =
        (glx::PFNGLLOADPROGRAMNVPROC)
        GET_GL_PROC_ADDRESS("glLoadProgramNV");
    glx::glProgramParameter4dNV =
        (glx::PFNGLPROGRAMPARAMETER4DNVPROC)
        GET_GL_PROC_ADDRESS("glProgramParameter4dNV");
    glx::glProgramParameter4dvNV =
        (glx::PFNGLPROGRAMPARAMETER4DVNVPROC)
        GET_GL_PROC_ADDRESS("glProgramParameter4dvNV");
    glx::glProgramParameter4fNV =
        (glx::PFNGLPROGRAMPARAMETER4FNVPROC)
        GET_GL_PROC_ADDRESS("glProgramParameter4fNV");
    glx::glProgramParameter4fvNV =
        (glx::PFNGLPROGRAMPARAMETER4FVNVPROC)
        GET_GL_PROC_ADDRESS("glProgramParameter4fvNV");
    glx::glProgramParameters4dvNV =
        (glx::PFNGLPROGRAMPARAMETERS4DVNVPROC)
        GET_GL_PROC_ADDRESS("glProgramParameters4dvNV");
    glx::glProgramParameters4fvNV =
        (glx::PFNGLPROGRAMPARAMETERS4FVNVPROC)
        GET_GL_PROC_ADDRESS("glProgramParameters4fvNV");
    glx::glRequestResidentProgramsNV =
        (glx::PFNGLREQUESTRESIDENTPROGRAMSNVPROC)
        GET_GL_PROC_ADDRESS("glRequestResidentProgramsNV");
    glx::glTrackMatrixNV =
        (glx::PFNGLTRACKMATRIXNVPROC)
        GET_GL_PROC_ADDRESS("glTrackMatrixNV");
    glx::glVertexAttribPointerNV =
        (glx::PFNGLVERTEXATTRIBPOINTERNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribPointerNV");
    glx::glVertexAttrib1dNV =
        (glx::PFNGLVERTEXATTRIB1DNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib1dNV");
    glx::glVertexAttrib1dvNV =
        (glx::PFNGLVERTEXATTRIB1DVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib1dvNV");
    glx::glVertexAttrib1fNV =
        (glx::PFNGLVERTEXATTRIB1FNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib1fNV");
    glx::glVertexAttrib1fvNV =
        (glx::PFNGLVERTEXATTRIB1FVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib1fvNV");
    glx::glVertexAttrib1sNV =
        (glx::PFNGLVERTEXATTRIB1SNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib1sNV");
    glx::glVertexAttrib1svNV =
        (glx::PFNGLVERTEXATTRIB1SVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib1svNV");
    glx::glVertexAttrib2dNV =
        (glx::PFNGLVERTEXATTRIB2DNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib2dNV");
    glx::glVertexAttrib2dvNV =
        (glx::PFNGLVERTEXATTRIB2DVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib2dvNV");
    glx::glVertexAttrib2fNV =
        (glx::PFNGLVERTEXATTRIB2FNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib2fNV");
    glx::glVertexAttrib2fvNV =
        (glx::PFNGLVERTEXATTRIB2FVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib2fvNV");
    glx::glVertexAttrib2sNV =
        (glx::PFNGLVERTEXATTRIB2SNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib2sNV");
    glx::glVertexAttrib2svNV =
        (glx::PFNGLVERTEXATTRIB2SVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib2svNV");
    glx::glVertexAttrib3dNV =
        (glx::PFNGLVERTEXATTRIB3DNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib3dNV");
    glx::glVertexAttrib3dvNV =
        (glx::PFNGLVERTEXATTRIB3DVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib3dvNV");
    glx::glVertexAttrib3fNV =
        (glx::PFNGLVERTEXATTRIB3FNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib3fNV");
    glx::glVertexAttrib3fvNV =
        (glx::PFNGLVERTEXATTRIB3FVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib3fvNV");
    glx::glVertexAttrib3sNV =
        (glx::PFNGLVERTEXATTRIB3SNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib3sNV");
    glx::glVertexAttrib3svNV =
        (glx::PFNGLVERTEXATTRIB3SVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib3svNV");
    glx::glVertexAttrib4dNV =
        (glx::PFNGLVERTEXATTRIB4DNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib4dNV");
    glx::glVertexAttrib4dvNV =
        (glx::PFNGLVERTEXATTRIB4DVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib4dvNV");
    glx::glVertexAttrib4fNV =
        (glx::PFNGLVERTEXATTRIB4FNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib4fNV");
    glx::glVertexAttrib4fvNV =
        (glx::PFNGLVERTEXATTRIB4FVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib4fvNV");
    glx::glVertexAttrib4sNV =
        (glx::PFNGLVERTEXATTRIB4SNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib4sNV");
    glx::glVertexAttrib4svNV =
        (glx::PFNGLVERTEXATTRIB4SVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib4svNV");
    glx::glVertexAttrib4ubvNV =
        (glx::PFNGLVERTEXATTRIB4UBVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttrib4ubvNV");
    glx::glVertexAttribs1dvNV =
        (glx::PFNGLVERTEXATTRIBS1DVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs1dvNV");
    glx::glVertexAttribs1fvNV =
        (glx::PFNGLVERTEXATTRIBS1FVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs1fvNV");
    glx::glVertexAttribs1svNV =
        (glx::PFNGLVERTEXATTRIBS1SVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs1svNV");
    glx::glVertexAttribs2dvNV =
        (glx::PFNGLVERTEXATTRIBS2DVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs2dvNV");
    glx::glVertexAttribs2fvNV =
        (glx::PFNGLVERTEXATTRIBS2FVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs2fvNV");
    glx::glVertexAttribs2svNV =
        (glx::PFNGLVERTEXATTRIBS2SVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs2svNV");
    glx::glVertexAttribs3dvNV =
        (glx::PFNGLVERTEXATTRIBS3DVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs3dvNV");
    glx::glVertexAttribs3fvNV =
        (glx::PFNGLVERTEXATTRIBS3FVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs3fvNV");
    glx::glVertexAttribs3svNV =
        (glx::PFNGLVERTEXATTRIBS3SVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs3svNV");
    glx::glVertexAttribs4dvNV =
        (glx::PFNGLVERTEXATTRIBS4DVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs4dvNV");
    glx::glVertexAttribs4fvNV =
        (glx::PFNGLVERTEXATTRIBS4FVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs4fvNV");
    glx::glVertexAttribs4svNV =
        (glx::PFNGLVERTEXATTRIBS4SVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs4svNV");
    glx::glVertexAttribs4ubvNV =
        (glx::PFNGLVERTEXATTRIBS4UBVNVPROC)
        GET_GL_PROC_ADDRESS("glVertexAttribs4ubvNV");
#endif
}


static void InitExt_EXT_paletted_texture()
{
#if defined(GET_GL_PROC_ADDRESS)
    glx::glColorTableEXT = (glx::PFNGLCOLORTABLEEXTPROC) GET_GL_PROC_ADDRESS("glColorTableEXT");
#endif
}


static void InitExt_EXT_blend_minmax()
{
#if defined(GET_GL_PROC_ADDRESS)
    glx::glBlendEquationEXT = (glx::PFNGLBLENDEQUATIONEXTPROC) GET_GL_PROC_ADDRESS("glBlendEquationEXT");
#endif
}


static void InitExt_EXT_swap_control()
{
#ifdef _WIN32
    glx::wglSwapIntervalEXT = (glx::PFNWGLSWAPINTERVALEXTPROC) GET_GL_PROC_ADDRESS("wglSwapIntervalEXT");
    glx::wglGetSwapIntervalEXT = (glx::PFNWGLGETSWAPINTERVALEXTPROC) GET_GL_PROC_ADDRESS("wglGetSwapIntervalEXT");
#endif // _WIN32
}


static void InitExt_GLX_SGI_video_sync()
{
    glx::glXGetVideoSyncSGI = (glx::PFNGLXGETVIDEOSYNCSGIPROC) GET_GL_PROC_ADDRESS("glXGetVideoSyncSGI");
    glx::glXWaitVideoSyncSGI = (glx::PFNGLXWAITVIDEOSYNCSGIPROC) GET_GL_PROC_ADDRESS("glXWaitVideoSyncSGI");
    glx::glXGetRefreshRateSGI = (glx::PFNGLXGETREFRESHRATESGIPROC) GET_GL_PROC_ADDRESS("glXGetRefreshRateSGI");
}


static void InitExt_NV_fragment_program()
{
#if defined(GET_GL_PROC_ADDRESS)
    glx::glProgramNamedParameter4fNV = (glx::PFNGLPROGRAMNAMEDPARAMETER4FNVPROC) GET_GL_PROC_ADDRESS("glProgramNamedParameter4fNV");
    glx::glProgramNamedParameter4dNV = (glx::PFNGLPROGRAMNAMEDPARAMETER4DNVPROC) GET_GL_PROC_ADDRESS("glProgramNamedParameter4dNV");
    glx::glProgramNamedParameter4fvNV = (glx::PFNGLPROGRAMNAMEDPARAMETER4FVNVPROC) GET_GL_PROC_ADDRESS("glProgramNamedParameter4fvNV");
    glx::glProgramNamedParameter4dvNV = (glx::PFNGLPROGRAMNAMEDPARAMETER4DVNVPROC) GET_GL_PROC_ADDRESS("glProgramNamedParameter4dvNV");
    glx::glGetProgramNamedParameterfvNV = (glx::PFNGLGETPROGRAMNAMEDPARAMETERFVNVPROC) GET_GL_PROC_ADDRESS("glGetProgramNamedParameterfvNV");
    glx::glGetProgramNamedParameterdvNV = (glx::PFNGLGETPROGRAMNAMEDPARAMETERDVNVPROC) GET_GL_PROC_ADDRESS("glGetProgramNamedParameterdvNV");
    glx::glProgramLocalParameter4fNV = (glx::PFNGLPROGRAMLOCALPARAMETER4FNVPROC) GET_GL_PROC_ADDRESS("glProgramLocalParameter4fNV");
    glx::glProgramLocalParameter4dNV = (glx::PFNGLPROGRAMLOCALPARAMETER4DNVPROC) GET_GL_PROC_ADDRESS("glProgramLocalParameter4dNV");
    glx::glProgramLocalParameter4fvNV = (glx::PFNGLPROGRAMLOCALPARAMETER4FVNVPROC) GET_GL_PROC_ADDRESS("glProgramLocalParameter4fvNV");
    glx::glProgramLocalParameter4dvNV = (glx::PFNGLPROGRAMLOCALPARAMETER4DVNVPROC) GET_GL_PROC_ADDRESS("glProgramLocalParameter4dvNV");
    glx::glGetProgramLocalParameterfvNV = (glx::PFNGLGETPROGRAMLOCALPARAMETERFVNVPROC) GET_GL_PROC_ADDRESS("glGetProgramLocalParameterfvNV");
    glx::glGetProgramLocalParameterdvNV = (glx::PFNGLGETPROGRAMLOCALPARAMETERDVNVPROC) GET_GL_PROC_ADDRESS("glGetProgramLocalParameterdvNV");
#endif // GET_GL_PROC_ADDRESS
}


static void InitExt_ARB_vertex_program()
{
#if defined(GET_GL_PROC_ADDRESS)
    glx::glBindProgramARB = (glx::PFNGLBINDPROGRAMARBPROC) GET_GL_PROC_ADDRESS("glBindProgramARB");
    glx::glDeleteProgramsARB = (glx::PFNGLDELETEPROGRAMSARBPROC) GET_GL_PROC_ADDRESS("glDeleteProgramsARB");
    glx::glGenProgramsARB = (glx::PFNGLGENPROGRAMSARBPROC) GET_GL_PROC_ADDRESS("glGenProgramsARB");
    glx::glIsProgramARB = (glx::PFNGLISPROGRAMARBPROC) GET_GL_PROC_ADDRESS("glIsProgramARB");
    glx::glVertexAttrib1sARB = (glx::PFNGLVERTEXATTRIB1SARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib1sARB");
    glx::glVertexAttrib1fARB = (glx::PFNGLVERTEXATTRIB1FARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib1fARB");
    glx::glVertexAttrib1dARB = (glx::PFNGLVERTEXATTRIB1DARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib1dARB");
    glx::glVertexAttrib2sARB = (glx::PFNGLVERTEXATTRIB2SARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib2sARB");
    glx::glVertexAttrib2fARB = (glx::PFNGLVERTEXATTRIB2FARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib2fARB");
    glx::glVertexAttrib2dARB = (glx::PFNGLVERTEXATTRIB2DARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib2dARB");
    glx::glVertexAttrib3sARB = (glx::PFNGLVERTEXATTRIB3SARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib3sARB");
    glx::glVertexAttrib3fARB = (glx::PFNGLVERTEXATTRIB3FARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib3fARB");
    glx::glVertexAttrib3dARB = (glx::PFNGLVERTEXATTRIB3DARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib3dARB");
    glx::glVertexAttrib4sARB = (glx::PFNGLVERTEXATTRIB4SARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib4sARB");
    glx::glVertexAttrib4fARB = (glx::PFNGLVERTEXATTRIB4FARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib4fARB");
    glx::glVertexAttrib4dARB = (glx::PFNGLVERTEXATTRIB4DARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib4dARB");
    glx::glVertexAttrib4NubARB = (glx::PFNGLVERTEXATTRIB4NUBARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib4NubARB");
    glx::glVertexAttrib1svARB = (glx::PFNGLVERTEXATTRIB1SVARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib1svARB");
    glx::glVertexAttrib1fvARB = (glx::PFNGLVERTEXATTRIB1FVARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib1fvARB");
    glx::glVertexAttrib1dvARB = (glx::PFNGLVERTEXATTRIB1DVARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib1dvARB");
    glx::glVertexAttrib2svARB = (glx::PFNGLVERTEXATTRIB2SVARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib2svARB");
    glx::glVertexAttrib2fvARB = (glx::PFNGLVERTEXATTRIB2FVARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib2fvARB");
    glx::glVertexAttrib2dvARB = (glx::PFNGLVERTEXATTRIB2DVARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib2dvARB");
    glx::glVertexAttrib3svARB = (glx::PFNGLVERTEXATTRIB3SVARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib3svARB");
    glx::glVertexAttrib3fvARB = (glx::PFNGLVERTEXATTRIB3FVARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib3fvARB");
    glx::glVertexAttrib3dvARB = (glx::PFNGLVERTEXATTRIB3DVARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib3dvARB");
    glx::glVertexAttrib4bvARB = (glx::PFNGLVERTEXATTRIB4BVARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib4bvARB");
    glx::glVertexAttrib4svARB = (glx::PFNGLVERTEXATTRIB4SVARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib4svARB");
    glx::glVertexAttrib4ivARB = (glx::PFNGLVERTEXATTRIB4IVARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib4ivARB");
    glx::glVertexAttrib4ubvARB = (glx::PFNGLVERTEXATTRIB4UBVARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib4ubvARB");
    glx::glVertexAttrib4usvARB = (glx::PFNGLVERTEXATTRIB4USVARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib4usvARB");
    glx::glVertexAttrib4uivARB = (glx::PFNGLVERTEXATTRIB4UIVARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib4uivARB");
    glx::glVertexAttrib4fvARB = (glx::PFNGLVERTEXATTRIB4FVARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib4fvARB");
    glx::glVertexAttrib4dvARB = (glx::PFNGLVERTEXATTRIB4DVARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib4dvARB");
    glx::glVertexAttrib4NbvARB = (glx::PFNGLVERTEXATTRIB4NBVARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib4NbvARB");
    glx::glVertexAttrib4NsvARB = (glx::PFNGLVERTEXATTRIB4NSVARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib4NsvARB");
    glx::glVertexAttrib4NivARB = (glx::PFNGLVERTEXATTRIB4NIVARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib4NivARB");
    glx::glVertexAttrib4NubvARB = (glx::PFNGLVERTEXATTRIB4NUBVARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib4NubvARB");
    glx::glVertexAttrib4NusvARB = (glx::PFNGLVERTEXATTRIB4NUSVARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib4NusvARB");
    glx::glVertexAttrib4NuivARB = (glx::PFNGLVERTEXATTRIB4NUIVARBPROC) GET_GL_PROC_ADDRESS("glVertexAttrib4NuivARB");
    glx::glVertexAttribPointerARB = (glx::PFNGLVERTEXATTRIBPOINTERARBPROC) GET_GL_PROC_ADDRESS("glVertexAttribPointerARB");
    glx::glEnableVertexAttribArrayARB = (glx::PFNGLENABLEVERTEXATTRIBARRAYARBPROC) GET_GL_PROC_ADDRESS("glEnableVertexAttribArrayARB");
    glx::glDisableVertexAttribArrayARB = (glx::PFNGLDISABLEVERTEXATTRIBARRAYARBPROC) GET_GL_PROC_ADDRESS("glDisableVertexAttribArrayARB");
    glx::glGetVertexAttribdvARB = (glx::PFNGLGETVERTEXATTRIBDVARBPROC) GET_GL_PROC_ADDRESS("glGetVertexAttribdvARB");
    glx::glGetVertexAttribfvARB = (glx::PFNGLGETVERTEXATTRIBFVARBPROC) GET_GL_PROC_ADDRESS("glGetVertexAttribfvARB");
    glx::glGetVertexAttribivARB = (glx::PFNGLGETVERTEXATTRIBIVARBPROC) GET_GL_PROC_ADDRESS("glGetVertexAttribivARB");
    glx::glGetVertexAttribPointervARB = (glx::PFNGLGETVERTEXATTRIBPOINTERVARBPROC) GET_GL_PROC_ADDRESS("glGetVertexAttribPointervARB");
    glx::glProgramEnvParameter4dARB = (glx::PFNGLPROGRAMENVPARAMETER4DARBPROC) GET_GL_PROC_ADDRESS("glProgramEnvParameter4dARB");
    glx::glProgramEnvParameter4dvARB = (glx::PFNGLPROGRAMENVPARAMETER4DVARBPROC) GET_GL_PROC_ADDRESS("glProgramEnvParameter4dvARB");
    glx::glProgramEnvParameter4fARB = (glx::PFNGLPROGRAMENVPARAMETER4FARBPROC) GET_GL_PROC_ADDRESS("glProgramEnvParameter4fARB");
    glx::glProgramEnvParameter4fvARB = (glx::PFNGLPROGRAMENVPARAMETER4FVARBPROC) GET_GL_PROC_ADDRESS("glProgramEnvParameter4fvARB");
    glx::glProgramLocalParameter4dARB = (glx::PFNGLPROGRAMLOCALPARAMETER4DARBPROC) GET_GL_PROC_ADDRESS("glProgramLocalParameter4dARB");
    glx::glProgramLocalParameter4dvARB = (glx::PFNGLPROGRAMLOCALPARAMETER4DVARBPROC) GET_GL_PROC_ADDRESS("glProgramLocalParameter4dvARB");
    glx::glProgramLocalParameter4fARB = (glx::PFNGLPROGRAMLOCALPARAMETER4FARBPROC) GET_GL_PROC_ADDRESS("glProgramLocalParameter4fARB");
    glx::glProgramLocalParameter4fvARB = (glx::PFNGLPROGRAMLOCALPARAMETER4FVARBPROC) GET_GL_PROC_ADDRESS("glProgramLocalParameter4fvARB");
    glx::glGetProgramEnvParameterdvARB = (glx::PFNGLGETPROGRAMENVPARAMETERDVARBPROC) GET_GL_PROC_ADDRESS("glGetProgramEnvParameterdvARB");
    glx::glGetProgramEnvParameterfvARB = (glx::PFNGLGETPROGRAMENVPARAMETERFVARBPROC) GET_GL_PROC_ADDRESS("glGetProgramEnvParameterfvARB");
    glx::glGetProgramLocalParameterdvARB = (glx::PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC) GET_GL_PROC_ADDRESS("glGetProgramLocalParameterdvARB");
    glx::glGetProgramLocalParameterfvARB = (glx::PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC) GET_GL_PROC_ADDRESS("glGetProgramLocalParameterfvARB");
    glx::glProgramStringARB = (glx::PFNGLPROGRAMSTRINGARBPROC) GET_GL_PROC_ADDRESS("glProgramStringARB");
    glx::glGetProgramStringARB = (glx::PFNGLGETPROGRAMSTRINGARBPROC) GET_GL_PROC_ADDRESS("glGetProgramStringARB");
    glx::glGetProgramivARB = (glx::PFNGLGETPROGRAMIVARBPROC) GET_GL_PROC_ADDRESS("glGetProgramivARB");
#endif // GET_GL_PROC_ADDRESS
}


static void InitExt_ARB_vertex_buffer_object()
{
#if defined(GET_GL_PROC_ADDRESS)
    glx::glBindBufferARB = (glx::PFNGLBINDBUFFERARBPROC) GET_GL_PROC_ADDRESS("glBindBufferARB");
    glx::glDeleteBuffersARB = (glx::PFNGLDELETEBUFFERSARBPROC) GET_GL_PROC_ADDRESS("glDeleteBuffersARB");
    glx::glGenBuffersARB = (glx::PFNGLGENBUFFERSARBPROC) GET_GL_PROC_ADDRESS("glGenBuffersARB");
    glx::glIsBufferARB = (glx::PFNGLISBUFFERARBPROC) GET_GL_PROC_ADDRESS("glIsBufferARB");
    glx::glBufferDataARB = (glx::PFNGLBUFFERDATAARBPROC) GET_GL_PROC_ADDRESS("glBufferDataARB");
    glx::glBufferSubDataARB = (glx::PFNGLBUFFERSUBDATAARBPROC) GET_GL_PROC_ADDRESS("glBufferSubDataARB");
    glx::glGetBufferSubDataARB = (glx::PFNGLGETBUFFERSUBDATAARBPROC) GET_GL_PROC_ADDRESS("glGetBufferSubDataARB");
    glx::glMapBufferARB = (glx::PFNGLMAPBUFFERARBPROC) GET_GL_PROC_ADDRESS("glMapBufferARB");
    glx::glUnmapBufferARB = (glx::PFNGLUNMAPBUFFERARBPROC) GET_GL_PROC_ADDRESS("glUnmapBufferARB");
    glx::glGetBufferParameterivARB = (glx::PFNGLGETBUFFERPARAMETERIVARBPROC) GET_GL_PROC_ADDRESS("glGetBufferParameterivARB");
    glx::glGetBufferPointervARB = (glx::PFNGLGETBUFFERPOINTERVARBPROC) GET_GL_PROC_ADDRESS("glGetBufferPointervARB");
#endif
}


static void InitExt_ARB_vertex_shader()
{
#if defined(GET_GL_PROC_ADDRESS)
    glx::glBindAttribLocationARB = (glx::PFNGLBINDATTRIBLOCATIONARBPROC) GET_GL_PROC_ADDRESS("glBindAttribLocationARB");
    glx::glGetActiveAttribARB = (glx::PFNGLGETACTIVEATTRIBARBPROC) GET_GL_PROC_ADDRESS("glGetActiveAttribARB");
    glx::glGetAttribLocationARB = (glx::PFNGLGETATTRIBLOCATIONARBPROC) GET_GL_PROC_ADDRESS("glGetAttribLocationARB");
#endif
}


static void InitExt_ARB_shader_objects()
{
#if defined(GET_GL_PROC_ADDRESS)
    glx::glDeleteObjectARB = (glx::PFNGLDELETEOBJECTARBPROC) GET_GL_PROC_ADDRESS("glDeleteObjectARB");
    glx::glGetHandleARB = (glx::PFNGLGETHANDLEARBPROC) GET_GL_PROC_ADDRESS("glGetHandleARB");
    glx::glDetachObjectARB = (glx::PFNGLDETACHOBJECTARBPROC) GET_GL_PROC_ADDRESS("glDetachObjectARB");
    glx::glCreateShaderObjectARB = (glx::PFNGLCREATESHADEROBJECTARBPROC) GET_GL_PROC_ADDRESS("glCreateShaderObjectARB");
    glx::glShaderSourceARB = (glx::PFNGLSHADERSOURCEARBPROC) GET_GL_PROC_ADDRESS("glShaderSourceARB");
    glx::glCompileShaderARB = (glx::PFNGLCOMPILESHADERARBPROC) GET_GL_PROC_ADDRESS("glCompileShaderARB");
    glx::glCreateProgramObjectARB = (glx::PFNGLCREATEPROGRAMOBJECTARBPROC) GET_GL_PROC_ADDRESS("glCreateProgramObjectARB");
    glx::glAttachObjectARB = (glx::PFNGLATTACHOBJECTARBPROC) GET_GL_PROC_ADDRESS("glAttachObjectARB");
    glx::glLinkProgramARB = (glx::PFNGLLINKPROGRAMARBPROC) GET_GL_PROC_ADDRESS("glLinkProgramARB");
    glx::glUseProgramObjectARB = (glx::PFNGLUSEPROGRAMOBJECTARBPROC) GET_GL_PROC_ADDRESS("glUseProgramObjectARB");
    glx::glValidateProgramARB = (glx::PFNGLVALIDATEPROGRAMARBPROC) GET_GL_PROC_ADDRESS("glValidateProgramARB");
    glx::glUniform1fARB = (glx::PFNGLUNIFORM1FARBPROC) GET_GL_PROC_ADDRESS("glUniform1fARB");
    glx::glUniform2fARB = (glx::PFNGLUNIFORM2FARBPROC) GET_GL_PROC_ADDRESS("glUniform2fARB");
    glx::glUniform3fARB = (glx::PFNGLUNIFORM3FARBPROC) GET_GL_PROC_ADDRESS("glUniform3fARB");
    glx::glUniform4fARB = (glx::PFNGLUNIFORM4FARBPROC) GET_GL_PROC_ADDRESS("glUniform4fARB");
    glx::glUniform1iARB = (glx::PFNGLUNIFORM1IARBPROC) GET_GL_PROC_ADDRESS("glUniform1iARB");
    glx::glUniform2iARB = (glx::PFNGLUNIFORM2IARBPROC) GET_GL_PROC_ADDRESS("glUniform2iARB");
    glx::glUniform3iARB = (glx::PFNGLUNIFORM3IARBPROC) GET_GL_PROC_ADDRESS("glUniform3iARB");
    glx::glUniform4iARB = (glx::PFNGLUNIFORM4IARBPROC) GET_GL_PROC_ADDRESS("glUniform4iARB");
    glx::glUniform1fvARB = (glx::PFNGLUNIFORM1FVARBPROC) GET_GL_PROC_ADDRESS("glUniform1fvARB");
    glx::glUniform2fvARB = (glx::PFNGLUNIFORM2FVARBPROC) GET_GL_PROC_ADDRESS("glUniform2fvARB");
    glx::glUniform3fvARB = (glx::PFNGLUNIFORM3FVARBPROC) GET_GL_PROC_ADDRESS("glUniform3fvARB");
    glx::glUniform4fvARB = (glx::PFNGLUNIFORM4FVARBPROC) GET_GL_PROC_ADDRESS("glUniform4fvARB");
    glx::glUniform1ivARB = (glx::PFNGLUNIFORM1IVARBPROC) GET_GL_PROC_ADDRESS("glUniform1ivARB");
    glx::glUniform2ivARB = (glx::PFNGLUNIFORM2IVARBPROC) GET_GL_PROC_ADDRESS("glUniform2ivARB");
    glx::glUniform3ivARB = (glx::PFNGLUNIFORM3IVARBPROC) GET_GL_PROC_ADDRESS("glUniform3ivARB");
    glx::glUniform4ivARB = (glx::PFNGLUNIFORM4IVARBPROC) GET_GL_PROC_ADDRESS("glUniform4ivARB");
    glx::glUniformMatrix2fvARB = (glx::PFNGLUNIFORMMATRIX2FVARBPROC) GET_GL_PROC_ADDRESS("glUniformMatrix2fvARB");
    glx::glUniformMatrix3fvARB = (glx::PFNGLUNIFORMMATRIX3FVARBPROC) GET_GL_PROC_ADDRESS("glUniformMatrix3fvARB");
    glx::glUniformMatrix4fvARB = (glx::PFNGLUNIFORMMATRIX4FVARBPROC) GET_GL_PROC_ADDRESS("glUniformMatrix4fvARB");
    glx::glGetObjectParameterfvARB = (glx::PFNGLGETOBJECTPARAMETERFVARBPROC) GET_GL_PROC_ADDRESS("glGetObjectParameterfvARB");
    glx::glGetObjectParameterivARB = (glx::PFNGLGETOBJECTPARAMETERIVARBPROC) GET_GL_PROC_ADDRESS("glGetObjectParameterivARB");
    glx::glGetInfoLogARB = (glx::PFNGLGETINFOLOGARBPROC) GET_GL_PROC_ADDRESS("glGetInfoLogARB");
    glx::glGetAttachedObjectsARB = (glx::PFNGLGETATTACHEDOBJECTSARBPROC) GET_GL_PROC_ADDRESS("glGetAttachedObjectsARB");
    glx::glGetUniformLocationARB = (glx::PFNGLGETUNIFORMLOCATIONARBPROC) GET_GL_PROC_ADDRESS("glGetUniformLocationARB");
    glx::glGetActiveUniformARB = (glx::PFNGLGETACTIVEUNIFORMARBPROC) GET_GL_PROC_ADDRESS("glGetActiveUniformARB");
    glx::glGetUniformfvARB = (glx::PFNGLGETUNIFORMFVARBPROC) GET_GL_PROC_ADDRESS("glGetUniformfvARB");
    glx::glGetUniformivARB = (glx::PFNGLGETUNIFORMIVARBPROC) GET_GL_PROC_ADDRESS("glGetUniformivARB");
    glx::glGetShaderSourceARB = (glx::PFNGLGETSHADERSOURCEARBPROC) GET_GL_PROC_ADDRESS("glGetShaderSourceARB");
#endif
}


static void InitExt_ARB_color_buffer_float()
{
#if defined(GET_GL_PROC_ADDRESS)
    glx::glClampColorARB = (glx::PFNGLCLAMPCOLORARBPROC) GET_GL_PROC_ADDRESS("glClampColorARB");
#endif
}

static void InitExt_EXT_framebuffer_object()
{
#if defined(GET_GL_PROC_ADDRESS)
    glx::glIsRenderbufferEXT = (glx::PFNGLISRENDERBUFFEREXTPROC) GET_GL_PROC_ADDRESS("glIsRenderbufferEXT");
    glx::glBindRenderbufferEXT = (glx::PFNGLBINDRENDERBUFFEREXTPROC) GET_GL_PROC_ADDRESS("glBindRenderbufferEXT");
    glx::glDeleteRenderbuffersEXT = (glx::PFNGLDELETERENDERBUFFERSEXTPROC) GET_GL_PROC_ADDRESS("glDeleteRenderbuffersEXT");
    glx::glGenRenderbuffersEXT = (glx::PFNGLGENRENDERBUFFERSEXTPROC) GET_GL_PROC_ADDRESS("glGenRenderbuffersEXT");
    glx::glRenderbufferStorageEXT = (glx::PFNGLRENDERBUFFERSTORAGEEXTPROC) GET_GL_PROC_ADDRESS("glRenderbufferStorageEXT");
    glx::glGetRenderbufferParameterivEXT = (glx::PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC) GET_GL_PROC_ADDRESS("glGetRenderbufferParameterivEXT");
    glx::glIsFramebufferEXT = (glx::PFNGLISFRAMEBUFFEREXTPROC) GET_GL_PROC_ADDRESS("glIsFramebufferEXT");
    glx::glBindFramebufferEXT = (glx::PFNGLBINDFRAMEBUFFEREXTPROC) GET_GL_PROC_ADDRESS("glBindFramebufferEXT");
    glx::glDeleteFramebuffersEXT = (glx::PFNGLDELETEFRAMEBUFFERSEXTPROC) GET_GL_PROC_ADDRESS("glDeleteFramebuffersEXT");
    glx::glGenFramebuffersEXT = (glx::PFNGLGENFRAMEBUFFERSEXTPROC) GET_GL_PROC_ADDRESS("glGenFramebuffersEXT");
    glx::glCheckFramebufferStatusEXT = (glx::PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC) GET_GL_PROC_ADDRESS("glCheckFramebufferStatusEXT");
    glx::glFramebufferTexture1DEXT  = (glx::PFNGLFRAMEBUFFERTEXTURE1DEXTPROC) GET_GL_PROC_ADDRESS("glFramebufferTexture1DEXT");
    glx::glFramebufferTexture2DEXT  = (glx::PFNGLFRAMEBUFFERTEXTURE2DEXTPROC) GET_GL_PROC_ADDRESS("glFramebufferTexture2DEXT");
    glx::glFramebufferTexture3DEXT  = (glx::PFNGLFRAMEBUFFERTEXTURE3DEXTPROC) GET_GL_PROC_ADDRESS("glFramebufferTexture3DEXT");
    glx::glFramebufferRenderbufferEXT = (glx::PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC) GET_GL_PROC_ADDRESS("glFramebufferRenderbufferEXT");
    glx::glGetFramebufferAttachmentParameterivEXT = (glx::PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC) GET_GL_PROC_ADDRESS("glGetFramebufferAttachmentParameterivEXT");
    glx::glGenerateMipmapEXT = (glx::PFNGLGENERATEMIPMAPEXTPROC) GET_GL_PROC_ADDRESS("glGenerateMipmapEXT");   
#endif
}

void InitExtension(const char* ext)
{
    if (!strcmp(ext, "GL_NV_fragment_program"))
        InitExt_NV_fragment_program();
    else if (!strcmp(ext, "GL_ARB_vertex_program"))
        InitExt_ARB_vertex_program();
    else if (!strcmp(ext, "GL_ARB_multitexture"))
        InitExt_ARB_multitexture();
    else if (!strcmp(ext, "GL_ARB_vertex_buffer_object"))
        InitExt_ARB_vertex_buffer_object();
    else if (!strcmp(ext, "GL_NV_register_combiners"))
        InitExt_NV_register_combiners();
    else if (!strcmp(ext, "GL_NV_register_combiners2"))
        InitExt_NV_register_combiners2();
    else if (!strcmp(ext, "GL_NV_vertex_program"))
        InitExt_NV_vertex_program();
    else if (!strcmp(ext, "GL_ARB_texture_compression"))
        InitExt_ARB_texture_compression();
    else if (!strcmp(ext, "GL_EXT_blend_minmax"))
        InitExt_EXT_blend_minmax();
    else if (!strcmp(ext, "GL_EXT_paletted_texture"))
        InitExt_EXT_paletted_texture();
    else if (!strcmp(ext, "GL_ARB_shader_objects"))
        InitExt_ARB_shader_objects();
    else if (!strcmp(ext, "GL_ARB_vertex_shader"))
        InitExt_ARB_vertex_shader();
    else if (!strcmp(ext, "WGL_EXT_swap_control"))
        InitExt_EXT_swap_control();
    else if (!strcmp(ext, "GLX_SGI_video_sync"))
        InitExt_GLX_SGI_video_sync();
    else if (!strcmp(ext, "GLX_ARB_color_buffer_float"))
        InitExt_ARB_color_buffer_float();
    else if (!strcmp(ext, "GL_EXT_framebuffer_object"))
        InitExt_EXT_framebuffer_object();
}


bool ExtensionSupported(const char *ext)
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
