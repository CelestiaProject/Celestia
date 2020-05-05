#pragma once

#ifdef _WIN32
#include <windows.h>
#endif
#include <epoxy/gl.h>
#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

namespace celestia
{
namespace gl
{

constexpr const int GL_2_1 = 21;

extern bool ARB_shader_objects;
extern bool ARB_shading_language_100;
extern bool EXT_framebuffer_object;

bool init() noexcept;
bool checkVersion(int) noexcept;
} // gl
} // celestia
