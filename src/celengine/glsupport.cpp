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
CELAPI bool OES_geometry_shader               = false; //NOSONAR
#else
CELAPI bool ARB_invalidate_subdata             = false; //NOSONAR
#endif
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

bool EnableGeomShaders = true;

inline bool has_extension(const char *name) noexcept
{
    return epoxy_has_gl_extension(name);
}

bool check_extension(util::array_view<std::string> list, const char *name) noexcept
{
    return std::find(list.begin(), list.end(), std::string(name)) == list.end()
           && has_extension(name);
}

void enable_workarounds()
{
    bool isMesa = false;
    bool isAMD = false;
    bool isNavi = false;

    const char* s;
    s = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    // "4.6 (Compatibility Profile) Mesa 22.3.6"
    // "OpenGL ES 3.2 Mesa 22.3.6"
    if (s != nullptr)
        isMesa = std::strstr(s, "Mesa") != nullptr;

    s = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    // "AMD" for radeonsi
    // "Mesa/X.org" for llvmpipe
    // "Collabora Ltd" for zink
    if (s != nullptr)
        isAMD = std::strcmp(s, "AMD") == 0;

    s = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    // "AMD Radeon RX 6600 (navi23, LLVM 15.0.6, DRM 3.52, 6.4.0-0.deb12.2-amd64)"" for radeonsi
    // "llvmpipe (LLVM 15.0.6, 256 bits)""
    // "zink (llvmpipe (LLVM 15.0.6, 256 bits))""
    // "zink (AMD Radeon RX 6600 (RADV NAVI23))""
    if (s != nullptr)
        isNavi = std::strstr(s, "navi") != nullptr;

    // https://gitlab.freedesktop.org/mesa/mesa/-/issues/9971
    if (isMesa && isAMD && isNavi)
        EnableGeomShaders = false;
}

} // namespace

bool init(util::array_view<std::string> ignore) noexcept
{
#ifdef GL_ES
    OES_texture_border_clamp           = check_extension(ignore, "GL_OES_texture_border_clamp") || check_extension(ignore, "GL_EXT_texture_border_clamp");
    OES_geometry_shader                = check_extension(ignore, "GL_OES_geometry_shader") || check_extension(ignore, "GL_EXT_geometry_shader");
    // BPTC on GLES is exposed via GL_EXT_texture_compression_bptc; the
    // compressed-format tokens (0x8E8C / 0x8E8D) are identical to the desktop
    // GL_ARB_texture_compression_bptc extension, so the same flag drives both
    // upload paths.
    ARB_texture_compression_bptc   = check_extension(ignore, "GL_EXT_texture_compression_bptc");
#else
    ARB_invalidate_subdata         = check_extension(ignore, "GL_ARB_invalidate_subdata");
    ARB_texture_compression_bptc   = check_extension(ignore, "GL_ARB_texture_compression_bptc");
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

    enable_workarounds();

    return true;
}

bool checkVersion(int v) noexcept
{
    static int version = 0;
    if (version == 0)
        version = epoxy_gl_version(); // this function always queries GL
    return version >= v;
}

bool hasGeomShader() noexcept
{
#ifdef GL_ES
    return EnableGeomShaders && checkVersion(celestia::gl::GLES_3_2);
#else
    return EnableGeomShaders && checkVersion(celestia::gl::GL_3_2);
#endif
}

void enableGeomShaders() noexcept
{
    EnableGeomShaders = true;
}

void disableGeomShaders() noexcept
{
    EnableGeomShaders = false;
}

} // end namespace celestia::gl
