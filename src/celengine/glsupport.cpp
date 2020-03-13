#include "glsupport.h"

namespace celestia
{
namespace gl
{

bool ARB_shader_texture_lod         = false;
bool ARB_vertex_array_object        = false;
bool EXT_framebuffer_object         = false;
bool EXT_texture_compression_s3tc   = false;
bool EXT_texture_filter_anisotropic = false;

namespace
{
    inline bool has_extension(const char* name) noexcept
    {
#ifdef USE_GLEW
        return glewIsSupported(name);
#else
        return epoxy_has_gl_extension(name);
#endif
    }
}

bool init() noexcept
{
#ifdef USE_GLEW
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK)
        return false;
#endif
    ARB_shader_texture_lod         = has_extension("GL_ARB_shader_texture_lod");
    ARB_vertex_array_object        = has_extension("GL_ARB_vertex_array_object");
    EXT_framebuffer_object         = has_extension("GL_EXT_framebuffer_object");
    EXT_texture_compression_s3tc   = has_extension("GL_EXT_texture_compression_s3tc");
    EXT_texture_filter_anisotropic = has_extension("GL_EXT_texture_filter_anisotropic");

    return true;
}

bool checkVersion(int v) noexcept
{
#ifdef USE_GLEW
    switch (v)
    {
    case GL_2_1:
        return GLEW_VERSION_2_1 != GL_FALSE;
    default:
        return false;
    }
#else
    return epoxy_gl_version() >= v;
#endif
}
} // gl
} // celestia
