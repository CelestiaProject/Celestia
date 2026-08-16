#include "glsupport.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <fmt/format.h>
#include <celutil/gettext.h>

namespace celestia::gl
{

#ifdef GL_ES
CELAPI bool OES_texture_border_clamp          = false; //NOSONAR
#else
CELAPI bool ARB_invalidate_subdata             = false; //NOSONAR
#endif
CELAPI bool dualSourceBlending                 = false; //NOSONAR
CELAPI bool ARB_texture_compression_bptc      = false; //NOSONAR
CELAPI bool EXT_texture_compression_s3tc      = false; //NOSONAR
CELAPI bool EXT_texture_compression_s3tc_srgb = false; //NOSONAR
CELAPI bool EXT_texture_filter_anisotropic    = false; //NOSONAR
CELAPI bool EXT_texture_sRGB_R8               = false; //NOSONAR
CELAPI bool MESA_pack_invert                  = false; //NOSONAR
CELAPI GLint maxPointSize                     = 0; //NOSONAR
CELAPI GLint maxTextureSize                   = 0; //NOSONAR
CELAPI GLfloat maxLineWidth                   = 0.0f; //NOSONAR
CELAPI GLint maxTextureAnisotropy             = 0; //NOSONAR
CELAPI bool sRGBRendering                     = false; //NOSONAR

namespace
{

inline bool has_extension(const char *name) noexcept
{
    return epoxy_has_gl_extension(name);
}

bool check_extension(util::array_view<std::string> list, const char *name) noexcept
{
    return std::find(list.begin(), list.end(), std::string(name)) == list.end()
           && has_extension(name);
}

} // namespace

bool init(util::array_view<std::string> ignore) noexcept
{
#ifdef GL_ES
    OES_texture_border_clamp           = check_extension(ignore, "GL_OES_texture_border_clamp") || check_extension(ignore, "GL_EXT_texture_border_clamp");
    dualSourceBlending                 = check_extension(ignore, "GL_EXT_blend_func_extended");
    if (dualSourceBlending)
    {
        GLint maxDualSourceDrawBuffers = 0;
        glGetIntegerv(GL_MAX_DUAL_SOURCE_DRAW_BUFFERS_EXT, &maxDualSourceDrawBuffers);
        dualSourceBlending = maxDualSourceDrawBuffers > 0;
    }
    // BPTC on GLES is exposed via GL_EXT_texture_compression_bptc; the
    // compressed-format tokens (0x8E8C / 0x8E8D) are identical to the desktop
    // GL_ARB_texture_compression_bptc extension, so the same flag drives both
    // upload paths.
    ARB_texture_compression_bptc   = check_extension(ignore, "GL_EXT_texture_compression_bptc");
#else
    ARB_invalidate_subdata         = check_extension(ignore, "GL_ARB_invalidate_subdata");
    ARB_texture_compression_bptc   = check_extension(ignore, "GL_ARB_texture_compression_bptc");
    dualSourceBlending             = true;
#endif
    EXT_texture_compression_s3tc   = check_extension(ignore, "GL_EXT_texture_compression_s3tc");
#ifdef GL_ES
    // On GLES, sRGB S3TC requires a separate extension.
    EXT_texture_compression_s3tc_srgb = EXT_texture_compression_s3tc
                                        && check_extension(ignore, "GL_EXT_texture_compression_s3tc_srgb");
#else
    EXT_texture_compression_s3tc_srgb = EXT_texture_compression_s3tc;
#endif
    EXT_texture_filter_anisotropic = check_extension(ignore, "GL_EXT_texture_filter_anisotropic") || check_extension(ignore, "GL_ARB_texture_filter_anisotropic");
    EXT_texture_sRGB_R8            = check_extension(ignore, "GL_EXT_texture_sRGB_R8");
    MESA_pack_invert               = check_extension(ignore, "GL_MESA_pack_invert");

    std::array<GLint, 2> pointSizeRange = { 0, 0 };
    std::array<GLfloat, 2> lineWidthRange = { 0.0f, 0.0f };
#ifdef GL_ES
    glGetIntegerv(GL_ALIASED_POINT_SIZE_RANGE, pointSizeRange.data());
    glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, lineWidthRange.data());
#else
    // GL 3.2 Core removed GL_SMOOTH_*_RANGE; GL_POINT_SIZE_RANGE / GL_LINE_WIDTH_RANGE
    // are the surviving (and only) range queries.
    glGetIntegerv(GL_POINT_SIZE_RANGE, pointSizeRange.data());
    glGetFloatv(GL_LINE_WIDTH_RANGE, lineWidthRange.data());
#endif
    maxPointSize = pointSizeRange[1];
    maxLineWidth = lineWidthRange[1];

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

    if (gl::EXT_texture_filter_anisotropic)
        glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxTextureAnisotropy);

    return true;
}

bool checkVersion(int v) noexcept
{
    static int version = 0;
    if (version == 0)
        version = epoxy_gl_version(); // this function always queries GL
    return version >= v;
}

} // end namespace celestia::gl
