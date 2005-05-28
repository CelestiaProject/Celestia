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
    GLuint r[2];
    NSString *result;

    glGetIntegerv(e, r);
    if (glGetError())
        result = [NSString stringWithFormat:NSLocalizedString(@"%@: --not available--",""), desc];
    else
        result = [NSString stringWithFormat:NSLocalizedString(@"%@: %d",""), desc, (int)r[0]];

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
            [NSString stringWithUTF8String:glGetString (GL_VENDOR)]]]; ENDL;
    [result appendString:
        [NSString stringWithFormat:NSLocalizedString(@"Renderer: %@",""), [NSString stringWithUTF8String:glGetString (GL_RENDERER)]]]; ENDL;
    [result appendString:
        [NSString stringWithFormat:NSLocalizedString(@"Version: %@",""), [NSString stringWithUTF8String:glGetString (GL_VERSION)]]]; ENDL;

    ENDL;

    [result appendString:
        (ExtensionSupported("GL_ARB_multitexture") ?
         queryGLInteger(GL_MAX_TEXTURE_UNITS_ARB, NSLocalizedString(@"Max simultaneous textures","")) :
         [NSString stringWithFormat: [NSString stringWithFormat: @"%@: 1", NSLocalizedString(@"Max simultaneous textures","")]])]; ENDL;
    [result appendString: queryGLInteger(GL_MAX_TEXTURE_SIZE, NSLocalizedString(@"Max texture size",""))]; ENDL;

    ENDL;

    [result appendString: NSLocalizedString(@"Extensions:","")]; ENDL;
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
