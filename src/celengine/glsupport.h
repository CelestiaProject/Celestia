#pragma once

#include <string>
#include <celutil/array_view.h>

#ifdef _WIN32
#include <windows.h>
#endif
#include <epoxy/gl.h>

#ifdef GL_ES
#ifdef glDepthRange
#undef glDepthRange
#endif
#define glDepthRange glDepthRangef
#endif

namespace celestia::gl
{

constexpr const int GL_2_1 = 21;
constexpr const int GLES_2 = 20;

extern bool ARB_shader_texture_lod;
extern bool EXT_texture_compression_s3tc;
extern bool EXT_texture_filter_anisotropic;
extern bool MESA_pack_invert;
#ifdef GL_ES
extern bool OES_vertex_array_object;
extern bool OES_texture_border_clamp;
#else
extern bool ARB_vertex_array_object;
extern bool EXT_framebuffer_object;
#endif
extern GLint maxPointSize;
extern GLint maxTextureSize;
extern GLfloat maxLineWidth;
extern GLint maxTextureAnisotropy;

bool init(util::array_view<std::string> = {}) noexcept;
bool checkVersion(int) noexcept;

} // end namespace celestia::gl
