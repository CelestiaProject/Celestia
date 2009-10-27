// glcontext.h
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_GLCONTEXT_H_
#define _CELENGINE_GLCONTEXT_H_

#include <celengine/vertexprog.h>
#include <celengine/fragmentprog.h>
#include <vector>
#include <string>

class GLContext
{
 public:
    GLContext();
    virtual ~GLContext();

    enum GLRenderPath 
    {
        GLPath_Basic             = 0,
        GLPath_Multitexture      = 1,
        GLPath_NvCombiner        = 2,
        GLPath_DOT3_ARBVP        = 3,
        GLPath_NvCombiner_NvVP   = 4,
        GLPath_NvCombiner_ARBVP  = 5,
        GLPath_ARBFP_ARBVP       = 6,
        GLPath_NV30              = 7,
        GLPath_GLSL              = 8,
    };

    enum VertexPath
    {
        VPath_Basic              = 0,
        VPath_NV                 = 1,
        VPath_ARB                = 2,
    };

    void init(const std::vector<std::string>& ignoreExt);
    
    GLRenderPath getRenderPath() const { return renderPath; };
    bool setRenderPath(GLRenderPath);
    bool renderPathSupported(GLRenderPath) const;
    GLRenderPath nextRenderPath();

    bool extensionSupported(const std::string&) const;

    int getMaxTextures() const { return maxSimultaneousTextures; };
    bool hasMultitexture() const { return renderPath >= GLPath_Multitexture; };
    bool bumpMappingSupported() const;

    VertexPath getVertexPath() const;

    VertexProcessor* getVertexProcessor() const;
    FragmentProcessor* getFragmentProcessor() const;

 private:
    GLRenderPath renderPath;
    VertexPath vertexPath;
    VertexProcessor* vertexProc;
    FragmentProcessor* fragmentProc;

    int maxSimultaneousTextures;
    std::vector<std::string> extensions;
};

#endif // _CELENGINE_GLCONTEXT_H_

