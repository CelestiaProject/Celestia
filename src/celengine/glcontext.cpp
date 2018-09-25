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


static VertexProcessor* vpNV = nullptr;
static VertexProcessor* vpARB = nullptr;
static FragmentProcessor* fpNV = nullptr;


void GLContext::init(const vector<string>& ignoreExt)
{
    auto* extensionsString = (char*) glGetString(GL_EXTENSIONS);
    if (extensionsString != nullptr)
    {
        char* next = extensionsString;

        while (*next != '\0')
        {
            while (*next != '\0' && *next != ' ')
                next++;

            string ext(extensionsString, next - extensionsString);

            // scan the ignore list
            auto iter = std::find(ignoreExt.begin(), ignoreExt.end(), ext);
            if (iter == ignoreExt.end())
                extensions.push_back(ext);

            if (*next == '\0')
                break;
            next++;
            extensionsString = next;
        }
    }

    if (GLEW_ARB_multitexture && glActiveTextureARB != nullptr)
    {
        glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB,
                      (GLint*) &maxSimultaneousTextures);
    }

    if (GLEW_ARB_vertex_program && glGenProgramsARB)
    {
        DPRINTF(1, "Renderer: ARB vertex programs supported.\n");
        if (vpARB == nullptr)
            vpARB = vp::initARB();
        vertexProc = vpARB;
    }
    else if (GLEW_NV_vertex_program && glGenProgramsNV)
    {
        DPRINTF(1, "Renderer: nVidia vertex programs supported.\n");
        if (vpNV == nullptr)
            vpNV = vp::initNV();
        vertexProc = vpNV;
    }

    if (GLEW_NV_fragment_program && glGenProgramsNV)
    {
        DPRINTF(1, "Renderer: nVidia fragment programs supported.\n");
        if (fpNV == nullptr)
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
        vertexPath = VPath_Basic;
        break;
    case GLPath_DOT3_ARBVP:
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

    case GLPath_DOT3_ARBVP:
        return GLEW_ARB_texture_env_dot3 &&
               GLEW_ARB_vertex_program &&
               vertexProc != nullptr;

    case GLPath_ARBFP_ARBVP:
        return false;
        /*
        return GLEW_ARB_vertex_program &&
               GLEW_ARB_fragment_program &&
               vertexProc != nullptr;
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
    return vertexPath == VPath_Basic ? nullptr : vertexProc;
}


FragmentProcessor* GLContext::getFragmentProcessor() const
{
    if (renderPath == GLPath_NV30 /* || renderPath == GLPath_ARGFP_ARBVP */ )
        return fragmentProc;
    else
        return nullptr;
}
