// glcontext.h
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include "gl.h"
#include "glext.h"
#include "glcontext.h"

using namespace std;


GLContext::GLContext() :
    renderPath(GLPath_Basic),
    maxSimultaneousTextures(1)
{
}

GLContext::~GLContext()
{
}


void GLContext::init(const vector<string>& ignoreExt)
{
    char* extensionsString = (char*) glGetString(GL_EXTENSIONS);
    if (extensionsString != NULL)
    {
        char* next = extensionsString;
        
        while (*next != '\0')
        {
            while (*next != '\0' && *next != ' ')
                next++;

            string ext(extensionsString, next - extensionsString);

            // scan the ignore list
            bool shouldIgnore = false;
            for (vector<string>::const_iterator iter = ignoreExt.begin();
                 iter != ignoreExt.end(); iter++)
            {
                if (*iter == ext)
                {
                    shouldIgnore = true;
                    break;
                }
            }

            if (!shouldIgnore)
                extensions.insert(extensions.end(), ext);

            if (*next == '\0')
                break;
            next++;
            extensionsString = next;
        }
    }

    if (extensionSupported("GL_ARB_multitexture") &&
        glx::glActiveTextureARB != NULL)
    {
        glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB,
                      (GLint*) &maxSimultaneousTextures);
    }

    // Initialize all extensions used
    for (vector<string>::const_iterator iter = extensions.begin();
         iter != extensions.end(); iter++)
    {
        InitExtension(iter->c_str());
    }
}


bool GLContext::setRenderPath(GLRenderPath path)
{
    if (renderPathSupported(path))
    {
        renderPath = path;
        return true;
    }
    else
    {
        return false;
    }
}


bool GLContext::renderPathSupported(GLRenderPath path) const
{
    switch (path)
    {
    case GLPath_Basic:
        return true;

    case GLPath_Multitexture:
        return (maxSimultaneousTextures > 1 &&
                extensionSupported("GL_EXT_texture_env_combine"));

    case GLPath_NvCombiner:
        return extensionSupported("GL_NV_register_combiners");

    case GLPath_DOT3_ARBVP:
        return (extensionSupported("GL_ARB_texture_env_dot3") &&
                extensionSupported("GL_ARB_vertex_program"));

    case GLPath_NvCombiner_NvVP:
        return (extensionSupported("GL_NV_register_combiners") &&
                extensionSupported("GL_NV_vertex_program"));

    case GLPath_NvCombiner_ARBVP:
        return (extensionSupported("GL_NV_register_combiners") &&
                extensionSupported("GL_ARB_vertex_program"));

    case GLPath_ARBFP_ARBVP:
        return (extensionSupported("GL_ARB_vertex_program") &&
                extensionSupported("GL_ARB_fragment_program"));

    case GLPath_NV30:
        return (extensionSupported("GL_ARB_vertex_program") &&
                extensionSupported("GL_NV_fragment_program"));

    default:
        return false;
    }
}


bool GLContext::extensionSupported(const string& ext) const
{
    return (find(extensions.begin(), extensions.end(), ext) != extensions.end());
}


bool GLContext::bumpMappingSupported() const
{
    return renderPath > GLPath_Multitexture;
}
