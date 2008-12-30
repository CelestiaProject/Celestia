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
#include <celengine/lightenv.h>
#include <celengine/atmosphere.h>

#define ADVANCED_CLOUD_SHADOWS 0


class ShaderProperties
{
 public:
    ShaderProperties();
    bool usesShadows() const;
    bool usesFragmentLighting() const;
    bool usesTangentSpaceLighting() const;
    unsigned int getShadowCountForLight(unsigned int) const;
    void setShadowCountForLight(unsigned int, unsigned int);
    bool hasShadowsForLight(unsigned int) const;
    bool hasSharedTextureCoords() const;
    bool hasSpecular() const;
    bool hasScattering() const;
    bool isViewDependent() const;

 enum
 {
     DiffuseTexture          =   0x01,
     SpecularTexture         =   0x02,
     NormalTexture           =   0x04,
     NightTexture            =   0x08,
     SpecularInDiffuseAlpha  =   0x10,
     RingShadowTexture       =   0x20,
     OverlayTexture          =   0x40,
     CloudShadowTexture      =   0x80,
     CompressedNormalTexture =  0x100,
     EmissiveTexture         =  0x200,
     VertexOpacities         =  0x800,
     VertexColors            = 0x1000,
     Scattering              = 0x2000,
     PointSprite             = 0x4000,
     SharedTextureCoords     = 0x8000,
 };

 enum
 {
     DiffuseModel          = 0,
     SpecularModel         = 1,
     RingIllumModel        = 2,
     PerPixelSpecularModel = 3,
     OrenNayarModel        = 4,
     AtmosphereModel       = 5,
     LunarLambertModel     = 6,
     ParticleDiffuseModel  = 7,
     EmissiveModel         = 8,
     ParticleModel         = 9,
 };
 
 enum
 {
     VolumetricScatteringEffect      = 0x0001,
     VolumetricAbsorptionEffect      = 0x0002,
     VolumetricEmissionEffect        = 0x0004,
 };
 
 public:
    unsigned short nLights;
    unsigned short texUsage;    
    unsigned short lightModel;

    // Two bits per light, up to eight lights + three shadows per light
    unsigned short shadowCounts;
    
    // Effects that may be applied with any light model
    unsigned short effects;
};


static const unsigned int MaxShaderLights = 4;
static const unsigned int MaxShaderShadows = 3;
struct CelestiaGLProgramLight
{
    Vec3ShaderParameter direction;
    Vec3ShaderParameter diffuse;
    Vec3ShaderParameter specular;
    Vec3ShaderParameter halfVector;
    FloatShaderParameter brightness; // max of diffuse r, g, b
};

struct CelestiaGLProgramShadow
{
    Vec4ShaderParameter texGenS;
    Vec4ShaderParameter texGenT;
    FloatShaderParameter falloff;
    FloatShaderParameter maxDepth;
};

class CelestiaGLProgram
{
 public:
    CelestiaGLProgram(GLProgram& _program, const ShaderProperties&);
    ~CelestiaGLProgram();

    void use() const { program->use(); }
    
    void setLightParameters(const LightingState& ls,
                            Color materialDiffuse,
                            Color materialSpecular,
                            Color materialEmissive
#ifdef USE_HDR
                           ,float nightLightScale = 1.0f
#endif
                            );
    void setEclipseShadowParameters(const LightingState& ls,
                                    float planetRadius,
                                    const Mat4f& planetMat);
    void setAtmosphereParameters(const Atmosphere& atmosphere,
                                 float atmPlanetRadius,
                                 float objRadius);
    
 public:
    CelestiaGLProgramLight lights[MaxShaderLights];
    Vec3ShaderParameter fragLightColor[MaxShaderLights];
    Vec3ShaderParameter fragLightSpecColor[MaxShaderLights];
    FloatShaderParameter fragLightBrightness[MaxShaderLights];
    Vec3ShaderParameter eyePosition;
    FloatShaderParameter shininess;
    Vec3ShaderParameter ambientColor;
    FloatShaderParameter opacity;
#ifdef USE_HDR
    FloatShaderParameter nightLightScale;
#endif

    FloatShaderParameter ringWidth;
    FloatShaderParameter ringRadius;
    
    // Mix of Lambertian and "lunar" (Lommel-Seeliger) photometric models.
    // 0 = pure Lambertian, 1 = L-S
    FloatShaderParameter lunarLambert;

    // Diffuse texture coordinate offset
    FloatShaderParameter textureOffset;

    // Cloud shadow parameters
    // Height of cloud layer above planet, in units of object radius
    FloatShaderParameter cloudHeight;
    FloatShaderParameter shadowTextureOffset;
    
    // Parameters for atmospheric scattering; all distances are normalized for
    // a unit sphere.
    FloatShaderParameter mieCoeff;
    FloatShaderParameter mieScaleHeight;
    // Value of k for Schlick approximation to Henyey-Greenstein phase function
    // A value of 0 is isotropic, negative values a primarily backscattering,
    // positive values are forward scattering.
    FloatShaderParameter miePhaseAsymmetry;
    
    // Rayleigh scattering terms. There are three scattering coefficients: red,
    // green, and blue light. To simulate Rayleigh scattering, the coefficients 
    // should be in ratios that fit 1/wavelength^4, but other values may be used
    // to simulate different types of wavelength dependent scattering.
    Vec3ShaderParameter rayleighCoeff;
    FloatShaderParameter rayleighScaleHeight;
    
    // Precomputed sum and inverse sum of Rayleigh and Mie scattering coefficients
    Vec3ShaderParameter scatterCoeffSum;
    Vec3ShaderParameter invScatterCoeffSum;
    // Precomputed sum of absorption and scattering coefficients--identical to
    // scatterCoeffSum when there is no absorption.
    Vec3ShaderParameter extinctionCoeff;
    
    // Radius of sphere for atmosphere--should be significantly larger than
    // scale height. Three components:
    //    x = radius
    //    y = radius^2
    //    z = 1/radius
    Vec3ShaderParameter atmosphereRadius;

    // Scale factor for point sprites
    FloatShaderParameter pointScale;

    CelestiaGLProgramShadow shadows[MaxShaderLights][MaxShaderShadows];
    
 private:
    void initParameters();
    void initSamplers();

    FloatShaderParameter floatParam(const std::string&);
    Vec3ShaderParameter vec3Param(const std::string&);
    Vec4ShaderParameter vec4Param(const std::string&);

    GLProgram* program;
    const ShaderProperties props;
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
    
    GLVertexShader* buildAtmosphereVertexShader(const ShaderProperties&);
    GLFragmentShader* buildAtmosphereFragmentShader(const ShaderProperties&);

    GLVertexShader* buildEmissiveVertexShader(const ShaderProperties&);
    GLFragmentShader* buildEmissiveFragmentShader(const ShaderProperties&);

    GLVertexShader* buildParticleVertexShader(const ShaderProperties&);
    GLFragmentShader* buildParticleFragmentShader(const ShaderProperties&);

    std::map<ShaderProperties, CelestiaGLProgram*> shaders;
};

extern ShaderManager& GetShaderManager();

#endif // _CELENGINE_SHADERMANAGER_H_
