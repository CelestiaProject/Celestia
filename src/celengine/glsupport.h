#pragma once

#ifdef GL_ES
#include <epoxy/gl.h>
/*
#include <GLES2/gl2.h>
#define GL_GLEXT_PROTOTYPES 1
#include <GLES2/gl2ext.h>
*/
#else
#ifdef _WIN32
#include <windows.h>
#endif
#include <epoxy/gl.h>
#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
#endif

#ifdef GL_ES
#ifdef glDepthRange
#undef glDepthRange
#endif
#define glDepthRange glDepthRangef
#endif

namespace celestia
{
namespace gl
{

constexpr const int GL_2_1 = 21;

extern bool ARB_shader_texture_lod;
extern bool EXT_texture_compression_s3tc;
extern bool EXT_texture_filter_anisotropic;
#ifdef GL_ES
extern bool OES_vertex_array_object;
#else
extern bool ARB_vertex_array_object;
extern bool EXT_framebuffer_object;
#endif
extern GLint maxPointSize;
extern GLfloat maxLineWidth;

bool init() noexcept;
bool checkVersion(int) noexcept;
} // gl
} // celestia
