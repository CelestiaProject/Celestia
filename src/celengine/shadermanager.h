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
    Vec3ShaderProperty direction;
    Vec3ShaderProperty diffuse;
    Vec3ShaderProperty specular;
    Vec3ShaderProperty halfVector;
};

class CelestiaGLProgram
{
 public:
    CelestiaGLProgram(GLProgram& _program, const ShaderProperties&);
    ~CelestiaGLProgram();

    void use() const { program->use(); }
    
 public:
    CelestiaGLProgramLight lights[MaxShaderLights];
    Vec3ShaderProperty eyePosition;
    FloatShaderProperty shininess;
    
 private:
    void initProperties(const ShaderProperties&);
    FloatShaderProperty floatProperty(const std::string&);
    Vec3ShaderProperty vec3Property(const std::string&);
    Vec4ShaderProperty vec4Property(const std::string&);

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
