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
     NightTexture     = 0x8,
     SpecularInDiffuseAlpha = 0x10,
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


static const int MaxShaderLights = 4;
struct CelestiaGLProgramLight
{
    Vec3ShaderParameter direction;
    Vec3ShaderParameter diffuse;
    Vec3ShaderParameter specular;
    Vec3ShaderParameter halfVector;
};

class CelestiaGLProgram
{
 public:
    CelestiaGLProgram(GLProgram& _program, const ShaderProperties&);
    ~CelestiaGLProgram();

    void use() const { program->use(); }
    
 public:
    CelestiaGLProgramLight lights[MaxShaderLights];
    Vec3ShaderParameter fragLightColor[MaxShaderLights];
    Vec3ShaderParameter eyePosition;
    FloatShaderParameter shininess;
    Vec3ShaderParameter ambientColor;
    
 private:
    void initParameters(const ShaderProperties&);
    void initSamplers(const ShaderProperties&);

    FloatShaderParameter floatParam(const std::string&);
    Vec3ShaderParameter vec3Param(const std::string&);
    Vec4ShaderParameter vec4Param(const std::string&);

    GLProgram* program;
};


class ShaderManager
{
 public:
    ShaderManager();
    ~ShaderManager();

    CelestiaGLProgram* getShader(const ShaderProperties&);

 private:
    CelestiaGLProgram* buildProgram(const ShaderProperties&);
    GLVertexShader* buildVertexShader(const ShaderProperties&);
    GLFragmentShader* buildFragmentShader(const ShaderProperties&);

    std::map<ShaderProperties, CelestiaGLProgram*> shaders;

    std::ostream* logFile;
};

extern ShaderManager& GetShaderManager();

#endif // _CELENGINE_SHADERMANAGER_H_
