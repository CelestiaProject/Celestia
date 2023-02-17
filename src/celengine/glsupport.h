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
enum Version
{
    GL_2_1   = 21,
    GL_3     = 30,
    GL_3_0   = 30,
    GL_3_1   = 31,
    GL_3_2   = 32,
    GL_3_3   = 33,
    GLES_2   = 20,
    GLES_2_0 = 20,
    GLES_3   = 30,
    GLES_3_0 = 30,
    GLES_3_1 = 31,
    GLES_3_2 = 32,
};

extern bool ARB_shader_texture_lod;
extern bool EXT_texture_compression_s3tc;
extern bool EXT_texture_filter_anisotropic;
extern bool MESA_pack_invert;
#ifdef GL_ES
extern bool OES_vertex_array_object;
extern bool OES_texture_border_clamp;
extern bool OES_geometry_shader;
#else
extern bool ARB_vertex_array_object;
extern bool ARB_framebuffer_object;
#endif
extern GLint maxPointSize;
extern GLint maxTextureSize;
extern GLfloat maxLineWidth;
extern GLint maxTextureAnisotropy;

bool init(util::array_view<std::string> = {}) noexcept;
bool checkVersion(int) noexcept;
bool hasGeomShader() noexcept;
void enableGeomShaders() noexcept;
void disableGeomShaders() noexcept;

} // end namespace celestia::gl
