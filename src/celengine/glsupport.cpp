#include "glsupport.h"

namespace celestia
{
namespace gl
{

#ifdef GL_ES
bool OES_vertex_array_object        = false;
#else
bool ARB_vertex_array_object        = false;
bool EXT_framebuffer_object         = false;
#endif
bool ARB_shader_texture_lod         = false;
bool EXT_texture_compression_s3tc   = false;
bool EXT_texture_filter_anisotropic = false;

namespace
{
    inline bool has_extension(const char* name) noexcept
    {
        return epoxy_has_gl_extension(name);
    }
}

bool init() noexcept
{
#ifdef GL_ES
    OES_vertex_array_object        = has_extension("GL_OES_vertex_array_object");
#else
    ARB_vertex_array_object        = has_extension("GL_ARB_vertex_array_object");
    EXT_framebuffer_object         = has_extension("GL_EXT_framebuffer_object");
#endif
    ARB_shader_texture_lod         = has_extension("GL_ARB_shader_texture_lod");
    EXT_texture_compression_s3tc   = has_extension("GL_EXT_texture_compression_s3tc");
    EXT_texture_filter_anisotropic = has_extension("GL_EXT_texture_filter_anisotropic");

    return true;
}

bool checkVersion(int v) noexcept
{
    return epoxy_gl_version() >= v;
}
} // gl
} // celestia
