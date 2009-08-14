// glcontext.h
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <celutil/debug.h>
#include "glcontext.h"
#include <GL/glew.h>

using namespace std;


static VertexProcessor* vpNV = NULL;
static VertexProcessor* vpARB = NULL;
static FragmentProcessor* fpNV = NULL;


GLContext::GLContext() :
    renderPath(GLPath_Basic),
    vertexPath(VPath_Basic),
    vertexProc(NULL),
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

    if (GLEW_ARB_multitexture && glActiveTextureARB != NULL)
    {
        glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB,
                      (GLint*) &maxSimultaneousTextures);
    }

    if (GLEW_ARB_vertex_program && glGenProgramsARB)
    {
        DPRINTF(1, "Renderer: ARB vertex programs supported.\n");
        if (vpARB == NULL)
            vpARB = vp::initARB();
        vertexProc = vpARB;
    }
    else if (GLEW_NV_vertex_program && glGenProgramsNV)
    {
        DPRINTF(1, "Renderer: nVidia vertex programs supported.\n");
        if (vpNV == NULL)
            vpNV = vp::initNV();
        vertexProc = vpNV;
    }

    if (GLEW_NV_fragment_program && glGenProgramsNV)
    {
        DPRINTF(1, "Renderer: nVidia fragment programs supported.\n");
        if (fpNV == NULL)
            fpNV = fp::initNV();
        fragmentProc = fpNV;
    }
}


bool GLContext::setRenderPath(GLRenderPath path)
{
    if (!renderPathSupported(path))
        return false;

    switch (path)
    {
    case GLPath_Basic:
    case GLPath_Multitexture:
    case GLPath_NvCombiner:
        vertexPath = VPath_Basic;
        break;
    case GLPath_NvCombiner_NvVP:
        vertexPath = VPath_NV;
        break;
    case GLPath_DOT3_ARBVP:
    case GLPath_NvCombiner_ARBVP:
    case GLPath_ARBFP_ARBVP:
    case GLPath_NV30:
    case GLPath_GLSL:
        vertexPath = VPath_ARB;
        break;
    default:
        return false;
    }

    renderPath = path;

    return true;
}


bool GLContext::renderPathSupported(GLRenderPath path) const
{
    switch (path)
    {
    case GLPath_Basic:
        return true;

    case GLPath_Multitexture:
        return (maxSimultaneousTextures > 1 &&
               (GLEW_EXT_texture_env_combine || GLEW_ARB_texture_env_combine));

    case GLPath_NvCombiner:
        return false;
        /*
        // No longer supported; all recent NVIDIA drivers also support
        // the vertex_program extension, so the combiners-only path
        // isn't necessary.
        return GLEW_NV_register_combiners;
        */

    case GLPath_DOT3_ARBVP:
        return GLEW_ARB_texture_env_dot3 &&
               GLEW_ARB_vertex_program &&
               vertexProc != NULL;

    case GLPath_NvCombiner_NvVP:
        // If ARB_vertex_program is supported, don't report support for
        // this render path.
        return GLEW_NV_register_combiners &&
               GLEW_NV_vertex_program &&
               !GLEW_ARB_vertex_program &&
               vertexProc != NULL;

    case GLPath_NvCombiner_ARBVP:
        return GLEW_NV_register_combiners &&
               GLEW_ARB_vertex_program &&
               vertexProc != NULL;

    case GLPath_ARBFP_ARBVP:
        return false;
        /*
        return GLEW_ARB_vertex_program &&
               GLEW_ARB_fragment_program &&
               vertexProc != NULL;
        */

    case GLPath_GLSL:
        return GLEW_ARB_shader_objects &&
               GLEW_ARB_shading_language_100 &&
               GLEW_ARB_vertex_shader &&
               GLEW_ARB_fragment_shader;

    default:
        return false;
    }
}


GLContext::GLRenderPath GLContext::nextRenderPath()
{
    GLContext::GLRenderPath newPath = renderPath;

    do {
        newPath = (GLRenderPath) ((int) newPath + 1);;
        if (newPath > GLPath_GLSL)
            newPath = GLPath_Basic;
    } while (newPath != renderPath && !renderPathSupported(newPath));

    renderPath = newPath;

    return renderPath;
}


bool GLContext::extensionSupported(const string& ext) const
{
    return (find(extensions.begin(), extensions.end(), ext) != extensions.end());
}


bool GLContext::bumpMappingSupported() const
{
    return renderPath > GLPath_Multitexture;
}


GLContext::VertexPath GLContext::getVertexPath() const
{
    return vertexPath;
}


VertexProcessor* GLContext::getVertexProcessor() const
{
    return vertexPath == VPath_Basic ? NULL : vertexProc;
}


FragmentProcessor* GLContext::getFragmentProcessor() const
{
    if (renderPath == GLPath_NV30 /* || renderPath == GLPath_ARGFP_ARBVP */ )
        return fragmentProc;
    else
        return NULL;
}
