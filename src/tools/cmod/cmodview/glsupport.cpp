#include "glsupport.h"

namespace celestia
{
namespace gl
{

bool ARB_shader_objects       = false;
bool ARB_shading_language_100 = false;
bool EXT_framebuffer_object   = false;

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
    ARB_shader_objects       = has_extension("GL_ARB_shader_objects");
    ARB_shading_language_100 = has_extension("GL_ARB_shading_language_100");
    EXT_framebuffer_object   = has_extension("GL_EXT_framebuffer_object");

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
