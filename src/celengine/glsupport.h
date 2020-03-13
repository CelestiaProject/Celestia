#pragma once

#ifdef USE_GLEW
#include <GL/glew.h>
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

namespace celestia
{
namespace gl
{

constexpr const int GL_2_1 = 21;

extern bool ARB_shader_texture_lod;
extern bool ARB_vertex_array_object;
extern bool EXT_framebuffer_object;
extern bool EXT_texture_compression_s3tc;
extern bool EXT_texture_filter_anisotropic;

bool init() noexcept;
bool checkVersion(int) noexcept;
} // gl
} // celestia
