// glext.cpp

#include <string.h>
#include "gl.h"
#include "glext.h"


// ARB_texture_compression
PFNGLCOMPRESSEDTEXIMAGE3DARBPROC glCompressedTexImage3DARB;
PFNGLCOMPRESSEDTEXIMAGE2DARBPROC glCompressedTexImage2DARB;
PFNGLCOMPRESSEDTEXIMAGE1DARBPROC glCompressedTexImage1DARB;
PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC glCompressedTexSubImage3DARB;
PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC glCompressedTexSubImage2DARB;
PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC glCompressedTexSubImage1DARB;

// ARB_multitexture command function pointers
PFNGLMULTITEXCOORD2IARBPROC glMultiTexCoord2iARB;
PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB;
PFNGLMULTITEXCOORD3FARBPROC glMultiTexCoord3fARB;
PFNGLMULTITEXCOORD3FVARBPROC glMultiTexCoord3fvARB;
PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB;

// NV_register_combiners command function pointers
PFNGLCOMBINERPARAMETERFVNVPROC glCombinerParameterfvNV;
PFNGLCOMBINERPARAMETERIVNVPROC glCombinerParameterivNV;
PFNGLCOMBINERPARAMETERFNVPROC glCombinerParameterfNV;
PFNGLCOMBINERPARAMETERINVPROC glCombinerParameteriNV;
PFNGLCOMBINERINPUTNVPROC glCombinerInputNV;
PFNGLCOMBINEROUTPUTNVPROC glCombinerOutputNV;
PFNGLFINALCOMBINERINPUTNVPROC glFinalCombinerInputNV;
PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC glGetCombinerInputParameterfvNV;
PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC glGetCombinerInputParameterivNV;
PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC glGetCombinerOutputParameterfvNV;
PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC glGetCombinerOutputParameterivNV;
PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC glGetFinalCombinerInputParameterfvNV;
PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC glGetFinalCombinerInputParameterivNV;

// EXT_paletted_texture command function pointers
PFNGLCOLORTABLEEXTPROC glColorTableEXT;

// WGL_EXT_swap_control command function pointers
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapIntervalEXT;

// extern void Alert(const char *szFormat, ...);


void Alert(const char *szFormat, ...)
{
}

#if 0
// Check for required extensions and initialize them if present
bool InitGLExtensions(void)
{
#if 0
    if (!extensionSupported("GL_EXT_bgra")) {
        Alert("Required OpenGL extension GL_EXT_bgra not supported");
        return false;
    }
#endif

    if (!extensionSupported("GL_ARB_multitexture")) {
        Alert("Required OpenGL extension GL_ARB_multitexture not supported");
        return false;
    }

#if 0
    if (!extensionSupported("GL_EXT_texture_env_combine")) {
	Alert("Required OpenGL extension GL_EXT_texture_env_combine not supported");
	return false;
    }

    if (!extensionSupported("GL_NV_register_combiners")) {
        Alert("Required OpenGL extension GL_NV_register_combiners not supported");
        return false;
    }

    if (!extensionSupported("GL_EXT_texture_cube_map")) {
        Alert("Required OpenGL extension GL_EXT_texture_cube_map not supported");
        return false;
    }

    if (!extensionSupported("GL_EXT_separate_specular_color")) {
        Alert("Required OpenGL extension GL_EXT_separate_specular_color not supported");
        return false;
    }

    if (!extensionSupported("WGL_EXT_swap_control")) {
        Alert("Required OpenGL extension WGL_EXT_swap_control not supported");
        return false;
    }
#endif

    initMultiTexture();
#if 0
    initRegisterCombiners();
    initSwapControl();
#endif

    return true;
}
#endif

// ARB_multitexture
void InitExtMultiTexture()
{
    glMultiTexCoord2iARB =
        (PFNGLMULTITEXCOORD2IARBPROC) wglGetProcAddress("glMultiTexCoord2iARB");
    glMultiTexCoord2fARB =
        (PFNGLMULTITEXCOORD2FARBPROC) wglGetProcAddress("glMultiTexCoord2fARB");
    glMultiTexCoord3fARB =
        (PFNGLMULTITEXCOORD3FARBPROC) wglGetProcAddress("glMultiTexCoord3fARB");
    glMultiTexCoord3fvARB =
        (PFNGLMULTITEXCOORD3FVARBPROC) wglGetProcAddress("glMultiTexCoord3fvARB");
    glActiveTextureARB =
        (PFNGLACTIVETEXTUREARBPROC) wglGetProcAddress("glActiveTextureARB");
    glClientActiveTextureARB =
        (PFNGLCLIENTACTIVETEXTUREARBPROC) wglGetProcAddress("glClientActiveTextureARB");
}


// ARB_texture_compression
void InitExtTextureCompression()
{
    glCompressedTexImage3DARB =
        (PFNGLCOMPRESSEDTEXIMAGE3DARBPROC)
        wglGetProcAddress("glCompressedTexImage3DARB");
    glCompressedTexImage2DARB =
        (PFNGLCOMPRESSEDTEXIMAGE2DARBPROC)
        wglGetProcAddress("glCompressedTexImage2DARB");
    glCompressedTexImage1DARB =
        (PFNGLCOMPRESSEDTEXIMAGE1DARBPROC)
        wglGetProcAddress("glCompressedTexImage1DARB");
    glCompressedTexSubImage3DARB =
        (PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC)
        wglGetProcAddress("glCompressedTexSubImage3DARB");
    glCompressedTexSubImage2DARB =
        (PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC)
        wglGetProcAddress("glCompressedTexSubImage2DARB");
    glCompressedTexSubImage1DARB =
        (PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC)
        wglGetProcAddress("glCompressedTexSubImage1DARB");
}


// NV_register_combiners
void InitExtRegisterCombiners()
{
  /* Retrieve all NV_register_combiners routines. */
  glCombinerParameterfvNV =
    (PFNGLCOMBINERPARAMETERFVNVPROC)
    wglGetProcAddress("glCombinerParameterfvNV");
  glCombinerParameterivNV =
    (PFNGLCOMBINERPARAMETERIVNVPROC)
    wglGetProcAddress("glCombinerParameterivNV");
  glCombinerParameterfNV =
    (PFNGLCOMBINERPARAMETERFNVPROC)
    wglGetProcAddress("glCombinerParameterfNV");
  glCombinerParameteriNV =
    (PFNGLCOMBINERPARAMETERINVPROC)
    wglGetProcAddress("glCombinerParameteriNV");
  glCombinerInputNV =
    (PFNGLCOMBINERINPUTNVPROC)
    wglGetProcAddress("glCombinerInputNV");
  glCombinerOutputNV =
    (PFNGLCOMBINEROUTPUTNVPROC)
    wglGetProcAddress("glCombinerOutputNV");
  glFinalCombinerInputNV =
    (PFNGLFINALCOMBINERINPUTNVPROC)
    wglGetProcAddress("glFinalCombinerInputNV");
  glGetCombinerInputParameterfvNV =
    (PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC)
    wglGetProcAddress("glGetCombinerInputParameterfvNV");
  glGetCombinerInputParameterivNV =
    (PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC)
    wglGetProcAddress("glGetCombinerInputParameterivNV");
  glGetCombinerOutputParameterfvNV =
    (PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC)
    wglGetProcAddress("glGetCombinerOutputParameterfvNV");
  glGetCombinerOutputParameterivNV =
    (PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC)
    wglGetProcAddress("glGetCombinerOutputParameterivNV");
  glGetFinalCombinerInputParameterfvNV =
    (PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC)
    wglGetProcAddress("glGetFinalCombinerInputParameterfvNV");
  glGetFinalCombinerInputParameterivNV =
    (PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC)
    wglGetProcAddress("glGetFinalCombinerInputParameterivNV");
}


void InitExtPalettedTexture()
{
    // glColorTableEXT = (void *) wglGetProcAddress("glColorTableEXT");
}


void InitExtSwapControl()
{
    wglSwapIntervalEXT = 
        (PFNWGLSWAPINTERVALEXTPROC) wglGetProcAddress("wglSwapIntervalEXT");
    wglGetSwapIntervalEXT =
        (PFNWGLGETSWAPINTERVALEXTPROC) wglGetProcAddress("wglGetSwapIntervalEXT");
}


bool ExtensionSupported(char *ext)
{
    char *extensions = (char *) glGetString(GL_EXTENSIONS);

    if (extensions == NULL)
        return false;

    int len = strlen(ext);
    for (;;) {
        if (strncmp(extensions, ext, len) == 0)
            return true;
        extensions = strchr(extensions, ' ');
        if  (extensions != NULL)
            extensions++;
        else
            break;
    }

    return false;
}
