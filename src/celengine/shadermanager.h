// shadermanager.h
//
// Copyright (C) 2001-2004, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_SHADERMANAGER_H_
#define _CELENGINE_SHADERMANAGER_H_

#include <map>
#include <iostream>
#include <celengine/glshader.h>

class ShaderProperties
{
 public:
    ShaderProperties();

 enum
 {
     DiffuseTexture   = 0x1,
     SpecularTexture  = 0x2,
     NormalTexture    = 0x4,
     NightTexture     = 0x8
 };

 enum
 {
     DiffuseModel     = 0,
     SpecularModel    = 1,
 };

 public:
    unsigned short nLights;
    unsigned short texUsage;
    unsigned short lightModel;
};


class ShaderManager
{
 public:
    ShaderManager();
    ~ShaderManager();

    GLProgram* getShader(const ShaderProperties&);

 private:
    GLProgram* buildProgram(const ShaderProperties&);
    GLVertexShader* buildVertexShader(const ShaderProperties&);
    GLFragmentShader* buildFragmentShader(const ShaderProperties&);

    std::map<ShaderProperties, GLProgram*> shaders;

    std::ostream* logFile;
};

extern ShaderManager& GetShaderManager();

#endif // _CELENGINE_SHADERMANAGER_H_
