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
    bool usesShadows() const;
    bool usesFragmentLighting() const;
    unsigned int getShadowCountForLight(unsigned int) const;
    void setShadowCountForLight(unsigned int, unsigned int);
    bool hasShadowsForLight(unsigned int) const;

 enum
 {
     DiffuseTexture         = 0x01,
     SpecularTexture        = 0x02,
     NormalTexture          = 0x04,
     NightTexture           = 0x08,
     SpecularInDiffuseAlpha = 0x10,
     RingShadowTexture      = 0x20,
 };

 enum
 {
     DiffuseModel     = 0,
     SpecularModel    = 1,
     RingIllumModel   = 2,
 };

 public:
    unsigned short nLights;
    unsigned short texUsage;
    unsigned short lightModel;

    // Two bits per light, up to eight lights + three shadows per light
    unsigned short shadowCounts;
};


static const unsigned int MaxShaderLights = 4;
static const unsigned int MaxShaderShadows = 3;
struct CelestiaGLProgramLight
{
    Vec3ShaderParameter direction;
    Vec3ShaderParameter diffuse;
    Vec3ShaderParameter specular;
    Vec3ShaderParameter halfVector;
};

struct CelestiaGLProgramShadow
{
    Vec4ShaderParameter texGenS;
    Vec4ShaderParameter texGenT;
    FloatShaderParameter scale;
    FloatShaderParameter bias;
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
    Vec3ShaderParameter fragLightSpecColor[MaxShaderLights];
    Vec3ShaderParameter eyePosition;
    FloatShaderParameter shininess;
    Vec3ShaderParameter ambientColor;

    FloatShaderParameter ringWidth;
    FloatShaderParameter ringRadius;

    CelestiaGLProgramShadow shadows[MaxShaderLights][MaxShaderShadows];
    
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
    GLVertexShader* buildRingsVertexShader(const ShaderProperties&);
    GLFragmentShader* buildRingsFragmentShader(const ShaderProperties&);

    std::map<ShaderProperties, CelestiaGLProgram*> shaders;
};

extern ShaderManager& GetShaderManager();

#endif // _CELENGINE_SHADERMANAGER_H_
