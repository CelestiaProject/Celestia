#include "glsupport.h"

namespace celestia::gl
{

bool ARB_shader_objects       = false;
bool ARB_shading_language_100 = false;
bool EXT_framebuffer_object   = false;

namespace
{
    inline bool has_extension(const char* name) noexcept
    {
        return epoxy_has_gl_extension(name);
    }
}

bool init() noexcept
{
    ARB_shader_objects       = has_extension("GL_ARB_shader_objects");
    ARB_shading_language_100 = has_extension("GL_ARB_shading_language_100");
    EXT_framebuffer_object   = has_extension("GL_EXT_framebuffer_object");

    return true;
}

bool checkVersion(int v) noexcept
{
    return epoxy_gl_version() >= v;
}

} // end namespace celestia::gl
