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


static VertexProcessor* vpARB = nullptr;


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

    glGetIntegerv(GL_MAX_TEXTURE_UNITS,
                  (GLint*) &maxSimultaneousTextures);

#ifdef VPROC
    if (GLEW_ARB_vertex_program && glGenProgramsARB)
    {
        DPRINTF(1, "Renderer: ARB vertex programs supported.\n");
        if (vpARB == nullptr)
            vpARB = vp::initARB();
        vertexProc = vpARB;
    }
#endif
}


bool GLContext::setRenderPath(GLRenderPath path)
{
    if (!renderPathSupported(path))
        return false;

#ifdef VPROC
    switch (path)
    {
    case GLPath_GLSL:
        vertexPath = VPath_ARB;
        break;
    default:
        return false;
    }
#endif

    renderPath = path;

    return true;
}


bool GLContext::renderPathSupported(GLRenderPath path) const
{
    switch (path)
    {
    case GLPath_GLSL:
        return GLEW_VERSION_2_1 != GL_FALSE;

    default:
        return false;
    }
}


GLContext::GLRenderPath GLContext::nextRenderPath()
{
#if 0
    GLContext::GLRenderPath newPath = renderPath;

    do {
        newPath = (GLRenderPath) ((int) newPath + 1);;
        if (newPath > GLPath_GLSL)
            newPath = GLPath_Basic;
    } while (newPath != renderPath && !renderPathSupported(newPath));

    renderPath = newPath;

    return renderPath;
#endif
    return GLPath_GLSL;
}


bool GLContext::extensionSupported(const string& ext) const
{
    return (find(extensions.begin(), extensions.end(), ext) != extensions.end());
}


bool GLContext::bumpMappingSupported() const
{
    return true;
}


#ifdef VPROC
GLContext::VertexPath GLContext::getVertexPath() const
{
    return vertexPath;
}


VertexProcessor* GLContext::getVertexProcessor() const
{
    return vertexProc;
}
#endif
