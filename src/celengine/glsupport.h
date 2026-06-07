#pragma once

#include <string>
#include <celutil/array_view.h>

#ifdef _WIN32
#ifdef IMPORT_GLSUPPORT
#define CELAPI __declspec(dllimport)
#else
#define CELAPI __declspec(dllexport)
#endif
#else
#define CELAPI
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
    GL_3     = 30,
    GL_3_0   = 30,
    GL_3_1   = 31,
    GL_3_2   = 32,
    GL_3_3   = 33,
    GL_4     = 40,
    GL_4_0   = 40,
    GL_4_1   = 41,
    GL_4_2   = 42,
    GL_4_3   = 43,
    GL_4_4   = 44,
    GL_4_5   = 45,
    GL_4_6   = 46,
    GLES_3   = 30,
    GLES_3_0 = 30,
    GLES_3_1 = 31,
    GLES_3_2 = 32,
};

#ifndef GL_ES
extern CELAPI bool ARB_invalidate_subdata; //NOSONAR
#endif
extern CELAPI bool ARB_texture_compression_bptc; //NOSONAR
extern CELAPI bool EXT_texture_compression_s3tc; //NOSONAR
extern CELAPI bool EXT_texture_compression_s3tc_srgb; //NOSONAR
extern CELAPI bool EXT_texture_filter_anisotropic; //NOSONAR
extern CELAPI bool EXT_texture_sRGB_R8; //NOSONAR
extern CELAPI bool MESA_pack_invert; //NOSONAR
#ifdef GL_ES
extern CELAPI bool OES_texture_border_clamp; //NOSONAR
extern CELAPI bool OES_geometry_shader; //NOSONAR
#endif
extern CELAPI GLint maxPointSize; //NOSONAR
extern CELAPI GLint maxTextureSize; //NOSONAR
extern CELAPI GLfloat maxLineWidth; //NOSONAR
extern CELAPI GLint maxTextureAnisotropy; //NOSONAR
extern CELAPI bool sRGBRendering; //NOSONAR

bool init(util::array_view<std::string> = {}) noexcept;
bool checkVersion(int) noexcept;
bool hasGeomShader() noexcept;
void enableGeomShaders() noexcept;
void disableGeomShaders() noexcept;

} // end namespace celestia::gl
