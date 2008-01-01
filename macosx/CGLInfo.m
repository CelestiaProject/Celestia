//
//  CGLInfo.m
//  celestia
//
//  Created by Bob Ippolito on Wed Jun 26 2002.
//  2005-05 Modified substantially by Da Woon Jung
//

#import "CGLInfo.h"
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>
#import <OpenGL/glext.h>

#ifndef GL_SHADING_LANGUAGE_VERSION_ARB
#define GL_SHADING_LANGUAGE_VERSION_ARB   0x8B8C
#endif

static char *gExtensions;

#define ENDL    [result appendString:@"\n"]

// Function taken from glext.cpp, so we don't have to compile
// this whole file as Obj-C++
static BOOL ExtensionSupported(const char *ext)
{
    if (gExtensions == nil)
        return false;

    char *extensions = gExtensions;
    int len = strlen(ext);
    for (;;) {
        if (strncmp(extensions, ext, len) == 0)
            return YES;
        extensions = strchr(extensions, ' ');
        if  (extensions != nil)
            ++extensions;
        else
            break;
    }

    return NO;
}

static NSString *queryGLInteger(GLenum e, NSString *desc)
{
    GLint r = 0;
    NSString *result;

    glGetIntegerv(e, &r);
//    if (glGetError())
//        result = [NSString stringWithFormat:NSLocalizedString(@"%@: --not available--",""), desc];
//    else
    result = [NSString stringWithFormat: NSLocalizedString(@"%@: %d",""), desc, r];

    return result;
}

static NSString *queryGLExtension(const char *extName)
{
    return [NSString stringWithFormat: NSLocalizedString(@"%s:  %@",""), extName, (ExtensionSupported(extName) ? NSLocalizedString(@"Supported","") : @"-")];
}


@implementation CGLInfo

+(NSString *)info
{
    NSMutableString *result = [NSMutableString string];
    gExtensions = (char *) glGetString(GL_EXTENSIONS);

    [result appendString:
        [NSString stringWithFormat:NSLocalizedString(@"Vendor: %@",""),
            [NSString stringWithUTF8String:(const char *)glGetString (GL_VENDOR)]]]; ENDL;
    [result appendString:
        [NSString stringWithFormat:NSLocalizedString(@"Renderer: %@",""), [NSString stringWithUTF8String:(const char *)glGetString (GL_RENDERER)]]]; ENDL;
    [result appendString:
        [NSString stringWithFormat:NSLocalizedString(@"Version: %@",""), [NSString stringWithUTF8String:(const char *)glGetString (GL_VERSION)]]]; ENDL;

    if (ExtensionSupported("GL_ARB_shading_language_100"))
    {
        NSString *glslVersion = [NSString stringWithUTF8String:
            (const char *)glGetString (0x8B8C)];
        if (glslVersion)
        {
            [result appendString:
                [NSString stringWithFormat: @"%@%@",
                    NSLocalizedStringFromTable(@"GLSL version: ",@"po",""),
                    glslVersion]
            ]; ENDL;
        }
    }

    ENDL;

    [result appendString:
        (ExtensionSupported("GL_ARB_multitexture") ?
         queryGLInteger(GL_MAX_TEXTURE_UNITS_ARB, NSLocalizedString(@"Max simultaneous textures","")) :
         [NSString stringWithFormat: [NSString stringWithFormat: @"%@: 1", NSLocalizedString(@"Max simultaneous textures","")]])]; ENDL;
    [result appendString: queryGLInteger(GL_MAX_TEXTURE_SIZE, NSLocalizedString(@"Max texture size",""))]; ENDL;

    if (ExtensionSupported("GL_ARB_texture_cube_map"))
    {
        GLint maxCubeMapSize = 0;
        glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &maxCubeMapSize);
        [result appendString:
            [NSString stringWithFormat: @"%@%d",
                NSLocalizedStringFromTable(@"Max cube map size: ",@"po",""),
                maxCubeMapSize]
        ]; ENDL;
    }

    GLfloat pointSizeRange[2];
    glGetFloatv(GL_POINT_SIZE_RANGE, pointSizeRange);
    [result appendString:
        [NSString stringWithFormat: @"%@%f - %f",
            NSLocalizedStringFromTable(@"Point size range: ",@"po",""),
            pointSizeRange[0], pointSizeRange[1]]
    ]; ENDL;

    ENDL;

    [result appendString: NSLocalizedString(@"Extensions:","")]; ENDL;
    [result appendString: queryGLExtension("GL_ARB_vertex_buffer_object")]; ENDL;
    [result appendString: queryGLExtension("GL_ARB_texture_compression")]; ENDL;
    [result appendString: queryGLExtension("GL_EXT_texture_compression_s3tc")]; ENDL;
    [result appendString: queryGLExtension("GL_ARB_texture_border_clamp")]; ENDL;
    [result appendString: queryGLExtension("GL_EXT_texture_edge_clamp")]; ENDL;
    [result appendString: queryGLExtension("GL_ARB_texture_cube_map")]; ENDL;
    [result appendString: queryGLExtension("GL_SGIS_generate_mipmap")]; ENDL;
    [result appendString: queryGLExtension("GL_EXT_rescale_normal")]; ENDL;
    [result appendString: queryGLExtension("GL_ARB_point_sprite")]; ENDL;

    ENDL;

    [result appendString: queryGLExtension("GL_ARB_vertex_program")]; ENDL;
    [result appendString: queryGLExtension("GL_NV_vertex_program")]; ENDL;
    [result appendString: queryGLExtension("GL_NV_fragment_program")]; ENDL;
    [result appendString: queryGLExtension("GL_NV_register_combiners")]; ENDL;
    [result appendString: queryGLExtension("GL_ARB_texture_env_dot3")]; ENDL;
    [result appendString: queryGLExtension("GL_ARB_texture_env_combine")]; ENDL;
    [result appendString: queryGLExtension("GL_EXT_texture_env_combine")]; ENDL;

    [result appendString: queryGLExtension("GL_ARB_shading_language_100")]; ENDL;
    [result appendString: queryGLExtension("GL_ARB_shader_objects")]; ENDL;
    [result appendString: queryGLExtension("GL_ARB_vertex_shader")]; ENDL;
    [result appendString: queryGLExtension("GL_ARB_fragment_shader")]; ENDL;

    return result;
}
@end
