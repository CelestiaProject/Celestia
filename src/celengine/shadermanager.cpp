// shadermanager.cpp
//
// Copyright (C) 2001-2004, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "gl.h"
#include "glext.h"
#include "shadermanager.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdio>
#include <cassert>

using namespace std;

// GLSL on Mac OS X appears to have a bug that precludes us from using structs
// #define USE_GLSL_STRUCTS

ShaderManager g_ShaderManager;


static const char* errorVertexShaderSource = 
    "void main(void) {\n"
    "   gl_Position = ftransform();\n"
    "}\n";
static const char* errorFragmentShaderSource =
    "void main(void) {\n"
    "   gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
    "}\n";


ShaderManager&
GetShaderManager()
{
    return g_ShaderManager;
}


ShaderProperties::ShaderProperties() :
    nLights(0),
    texUsage(0),
    lightModel(DiffuseModel),
    shadowCounts(0)
{
}


bool
ShaderProperties::usesShadows() const
{
    return  (texUsage & RingShadowTexture) != 0 ||
             (texUsage & CloudShadowTexture) != 0 ||
             shadowCounts != 0;
}


bool
ShaderProperties::usesFragmentLighting() const
{
    if ((texUsage & NormalTexture) != 0 || lightModel == PerPixelSpecularModel)
        return true;
    else
        return false;
}


unsigned int
ShaderProperties::getShadowCountForLight(unsigned int i) const
{
    return (shadowCounts >> i * 2) & 3;
}


void
ShaderProperties::setShadowCountForLight(unsigned int light, unsigned int n)
{
    assert(n < MaxShaderShadows);
    assert(light < MaxShaderLights);
    if (n <= MaxShaderShadows && light < MaxShaderLights)
    {
        shadowCounts &= ~(3 << light * 2);
        shadowCounts |= n << light * 2;
    }
}


bool
ShaderProperties::hasShadowsForLight(unsigned int light) const
{
    assert(light < MaxShaderLights);
    return getShadowCountForLight(light) != 0 ||
            (texUsage & (RingShadowTexture | CloudShadowTexture)) != 0;
}


// Returns true if diffuse, specular, bump, and night textures all use the
// same texture coordinate set.
bool
ShaderProperties::hasSharedTextureCoords() const
{
    return (texUsage & SharedTextureCoords) != 0;
}


bool
ShaderProperties::hasSpecular() const
{
    switch (lightModel)
    {
    case SpecularModel:
    case PerPixelSpecularModel:
        return true;
    default:
        return false;    
    }
}


bool
ShaderProperties::isViewDependent() const
{
    switch (lightModel)
    {
    case SpecularModel:
    case PerPixelSpecularModel:
    case RingIllumModel:
        return true;
    default:
        return false;    
    }
}


bool
ShaderProperties::usesTangentSpaceLighting() const
{
    return (texUsage & NormalTexture) != 0;
}


bool operator<(const ShaderProperties& p0, const ShaderProperties& p1)
{
    if (p0.texUsage < p1.texUsage)
        return true;
    else if (p1.texUsage < p0.texUsage)
        return false;

    if (p0.nLights < p1.nLights)
        return true;
    else if (p1.nLights < p0.nLights)
        return false;

    if (p0.shadowCounts < p1.shadowCounts)
        return true;
    else if (p1.shadowCounts < p0.shadowCounts)
        return false;

    return (p0.lightModel < p1.lightModel);
}


ShaderManager::ShaderManager()
{
    if (g_shaderLogFile == NULL)
#ifdef _WIN32
        g_shaderLogFile = new ofstream("shaders.log");
#else
        g_shaderLogFile = new ofstream("/tmp/celestia-shaders.log");
#endif
}


ShaderManager::~ShaderManager()
{
}


CelestiaGLProgram*
ShaderManager::getShader(const ShaderProperties& props)
{
    map<ShaderProperties, CelestiaGLProgram*>::iterator iter = shaders.find(props);
    if (iter != shaders.end())
    {
        // Shader already exists
        return iter->second;
    }
    else
    {
        // Create a new shader and add it to the table of created shaders
        CelestiaGLProgram* prog = buildProgram(props);
        shaders[props] = prog;

        return prog;
    }
}


CelestiaGLProgram::CelestiaGLProgram(GLProgram& _program,
                                     const ShaderProperties& props) :
    program(&_program)
{
    initParameters(props);
    initSamplers(props);
};


CelestiaGLProgram::~CelestiaGLProgram()
{
    delete program;
}


FloatShaderParameter
CelestiaGLProgram::floatParam(const string& paramName)
{
    return FloatShaderParameter(program->getID(), paramName.c_str());
}


Vec3ShaderParameter
CelestiaGLProgram::vec3Param(const string& paramName)
{
    return Vec3ShaderParameter(program->getID(), paramName.c_str());
}


Vec4ShaderParameter
CelestiaGLProgram::vec4Param(const string& paramName)
{
    return Vec4ShaderParameter(program->getID(), paramName.c_str());
}


static string
LightProperty(unsigned int i, char* property)
{
    char buf[64];

#ifndef USE_GLSL_STRUCTS
    sprintf(buf, "light%d_%s", i, property);
#else
    sprintf(buf, "lights[%d].%s", i, property);
#endif
    return string(buf);
}


static string
FragLightProperty(unsigned int i, char* property)
{
    char buf[64];
    sprintf(buf, "light%s%d", property, i);
    return string(buf);
}


static string
IndexedParameter(const char* name, unsigned int index)
{
    char buf[64];
    sprintf(buf, "%s%d", name, index);
    return string(buf);
}


static string
IndexedParameter(const char* name, unsigned int index0, unsigned int index1)
{
    char buf[64];
    sprintf(buf, "%s%d_%d", name, index0, index1);
    return string(buf);
}


static string
RingShadowTexCoord(unsigned int index)
{
    char buf[64];
    sprintf(buf, "ringShadowTexCoord.%c", "xyzw"[index]);
    return string(buf);
}


static string
CloudShadowTexCoord(unsigned int index)
{
    char buf[64];
    sprintf(buf, "cloudShadowTexCoord%d", index);
    return string(buf);
}


void
CelestiaGLProgram::initParameters(const ShaderProperties& props)
{
    for (unsigned int i = 0; i < props.nLights; i++)
    {
        lights[i].direction  = vec3Param(LightProperty(i, "direction"));
        lights[i].diffuse    = vec3Param(LightProperty(i, "diffuse"));
        lights[i].specular   = vec3Param(LightProperty(i, "specular"));
        lights[i].halfVector = vec3Param(LightProperty(i, "halfVector"));

        fragLightColor[i] = vec3Param(FragLightProperty(i, "color"));
        fragLightSpecColor[i] = vec3Param(FragLightProperty(i, "specColor"));

        for (unsigned int j = 0; j < props.getShadowCountForLight(i); j++)
        {
            shadows[i][j].texGenS =
                vec4Param(IndexedParameter("shadowTexGenS", i, j));
            shadows[i][j].texGenT =
                vec4Param(IndexedParameter("shadowTexGenT", i, j));
            shadows[i][j].scale =
                floatParam(IndexedParameter("shadowScale", i, j));
            shadows[i][j].bias =
                floatParam(IndexedParameter("shadowBias", i, j));
        }
    }

    if (props.hasSpecular())
    {
        shininess            = floatParam("shininess");
    }

    if (props.isViewDependent())        
    {
        eyePosition          = vec3Param("eyePosition");
    }

    ambientColor = vec3Param("ambientColor");

    if (props.texUsage & ShaderProperties::RingShadowTexture)
    {
        ringWidth            = floatParam("ringWidth");
        ringRadius           = floatParam("ringRadius");
    }

    textureOffset = floatParam("textureOffset");
    
    if (props.texUsage & ShaderProperties::CloudShadowTexture)
    {
        cloudHeight         = floatParam("cloudHeight");
        shadowTextureOffset = floatParam("cloudShadowTexOffset");
    }
    
    if ((props.texUsage & ShaderProperties::NightTexture) != 0)
    {
        nightTexMin          = floatParam("nightTexMin");
    }
}


void
CelestiaGLProgram::initSamplers(const ShaderProperties& props)
{
    program->use();

    unsigned int nSamplers = 0;
    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        int slot = glx::glGetUniformLocationARB(program->getID(), "diffTex");
        if (slot != -1)
            glx::glUniform1iARB(slot, nSamplers++);
    }

    if (props.texUsage & ShaderProperties::NormalTexture)
    {
        int slot = glx::glGetUniformLocationARB(program->getID(), "normTex");
        if (slot != -1)
            glx::glUniform1iARB(slot, nSamplers++);
    }

    if (props.texUsage & ShaderProperties::SpecularTexture)
    {
        int slot = glx::glGetUniformLocationARB(program->getID(), "specTex");
        if (slot != -1)
            glx::glUniform1iARB(slot, nSamplers++);
    }

    if (props.texUsage & ShaderProperties::NightTexture)
    {
        int slot = glx::glGetUniformLocationARB(program->getID(), "nightTex");
        if (slot != -1)
            glx::glUniform1iARB(slot, nSamplers++);
    }

    if (props.texUsage & ShaderProperties::RingShadowTexture)
    {
        int slot = glx::glGetUniformLocationARB(program->getID(), "ringTex");
        if (slot != -1)
            glx::glUniform1iARB(slot, nSamplers++);
    }
    
    if (props.texUsage & ShaderProperties::OverlayTexture)
    {
        int slot = glx::glGetUniformLocationARB(program->getID(), "overlayTex");
        if (slot != -1)
            glx::glUniform1iARB(slot, nSamplers++);
    }
    
    if (props.texUsage & ShaderProperties::CloudShadowTexture)
    {
        int slot = glx::glGetUniformLocationARB(program->getID(), "cloudShadowTex");
        if (slot != -1)
            glx::glUniform1iARB(slot, nSamplers++);
    }
}


static void
DumpShaderSource(ostream& out, const std::string& source)
{
    bool newline = true;
    unsigned int lineNumber = 0;

    for (unsigned int i = 0; i < source.length(); i++)
    {
        if (newline)
        {
            lineNumber++;
            out << setw(3) << lineNumber << ": ";
            newline = false;
        }

        out << source[i];
        if (source[i] == '\n')
            newline = true;
    }

    out.flush();
}


static string
DeclareLights(const ShaderProperties& props)
{
    if (props.nLights == 0)
        return string("");

    char lightSourceBuf[128];

#ifndef USE_GLSL_STRUCTS
    string lightSourceDecl;
    
    for (unsigned int i = 0; i < props.nLights; i++)
    {
        sprintf(lightSourceBuf,
                "uniform vec3 light%d_direction;\n"
                "uniform vec3 light%d_diffuse;\n"
                "uniform vec3 light%d_specular;\n"
                "uniform vec3 light%d_halfVector;\n",
                i, i, i, i);
        lightSourceDecl += string(lightSourceBuf);
    }
    
    return lightSourceDecl;
#else    
    sprintf(lightSourceBuf,
            "uniform struct {\n"
            "   vec3 direction;\n"
            "   vec3 diffuse;\n"
            "   vec3 specular;\n"
            "   vec3 halfVector;\n"
            "} lights[%d];\n",
            props.nLights);

    return string(lightSourceBuf);
#endif
}


static string
SeparateDiffuse(unsigned int i)
{
    // Used for packing multiple diffuse factors into the diffuse color.
    // It's probably better to use separate float interpolants.  I'll switch
    // to this once I verify that shader compilers are smart enough to pack
    // multiple scalars into a single vector interpolant.
    char buf[32];
    sprintf(buf, "diffFactors.%c", "xyzw"[i & 3]);
    return string(buf);
}


static string
SeparateSpecular(unsigned int i)
{
    // Used for packing multiple specular factors into the specular color.
    char buf[32];
    sprintf(buf, "specFactors.%c", "xyzw"[i & 3]);
    return string(buf);
}


// Used by rings shader
static string
ShadowDepth(unsigned int i)
{
    char buf[32];
    sprintf(buf, "shadowDepths.%c", "xyzw"[i & 3]);
    return string(buf);
}


static string
TexCoord2D(unsigned int i)
{
    char buf[64];
    sprintf(buf, "gl_MultiTexCoord%d.st", i);
    return string(buf);
}


static string
LightDir(unsigned int i)
{
    char buf[32];
    sprintf(buf, "lightDir%d", i);
    return string(buf);
}


static string
LightHalfVector(unsigned int i)
{
    char buf[32];
    sprintf(buf, "lightHalfVec%d", i);
    return string(buf);
}


static string
TangentSpaceTransform(const string& dst, const string& src)
{
    string source;
    
    source += dst + ".x = dot(tangent, " + src + ");\n";
    source += dst + ".y = dot(-bitangent, " + src + ");\n";
    source += dst + ".z = dot(gl_Normal, " + src + ");\n";
    
    return source;
}


static string
DirectionalLight(unsigned int i, const ShaderProperties& props)
{
    string source;

    source += "nDotVP = max(0.0, dot(gl_Normal, " +
        LightProperty(i, "direction") + "));\n";
        
    if (props.lightModel == ShaderProperties::SpecularModel)
    {
        source += "hv = normalize(" + LightProperty(i, "direction") + " + normalize(eyePosition - gl_Vertex.xyz));\n";
        source += "nDotHV = max(0.0, dot(gl_Normal, hv));\n";
  
        // The calculation below uses the infinite viewer approximation. It's slightly faster,
        // but results in less accurate rendering.
        // source += "nDotHV = max(0.0, dot(gl_Normal, " + LightProperty(i, "halfVector") + "));\n";
    }

    if (props.usesTangentSpaceLighting())
    {
        source += TangentSpaceTransform(LightDir(i), LightProperty(i, "direction"));
        // Diffuse color is computed in the fragment shader
    }
    else if (props.lightModel == ShaderProperties::PerPixelSpecularModel)
    {
        source += SeparateDiffuse(i) + " = nDotVP;\n";
        // Specular is computed in the fragment shader; half vectors are required
        // for the calculation
        source += LightHalfVector(i) + " = " + LightProperty(i, "direction") + " + eyeDir;\n";
    }
    else if (props.usesShadows())
    {
        // When there are shadows, we need to track the diffuse contributions
        // separately for each light.
        source += SeparateDiffuse(i) + " = nDotVP;\n";
        if (props.hasSpecular())
        {
            source += SeparateSpecular(i) + " = pow(nDotHV, shininess);\n";
        }
    }
    else
    {
        source += "diff.rgb += " + LightProperty(i, "diffuse") + " * nDotVP;\n";
        if (props.hasSpecular())
        {
            source += "spec.rgb += " + LightProperty(i, "specular") +
                " * (pow(nDotHV, shininess) * nDotVP);\n";
        }
    }

    if (props.texUsage & ShaderProperties::NightTexture)
    {
        source += "totalLight += nDotVP;\n";
    }


    return source;
}


static string
BeginLightSourceShadows(const ShaderProperties& props, unsigned int light)
{
    string source;

    if (props.usesTangentSpaceLighting())
    {
        if (props.hasShadowsForLight(light))
            source += "shadow = 1.0;\n";
    }
    else
    {
        source += "shadow = " + SeparateDiffuse(light) + ";\n";
    }

    if (props.texUsage & ShaderProperties::RingShadowTexture)
    {
        source += "shadow *= (1.0 - texture2D(ringTex, vec2(" +
            RingShadowTexCoord(light) + ", 0.0)).a);\n";
    }
    
    if (props.texUsage & ShaderProperties::CloudShadowTexture)
    {
        source += "shadow *= (1.0 - texture2D(cloudShadowTex, " +
            CloudShadowTexCoord(light) + ").a * 0.75);\n";
    }

    return source;
}


static string
Shadow(unsigned int light, unsigned int shadow)
{
    string source;

    source += "shadowCenter.s = dot(vec4(position_obj, 1.0), " +
        IndexedParameter("shadowTexGenS", light, shadow) + ") - 0.5;\n";
    source += "shadowCenter.t = dot(vec4(position_obj, 1.0), " +
        IndexedParameter("shadowTexGenT", light, shadow) + ") - 0.5;\n";   
    source += "shadowR = clamp(dot(shadowCenter, shadowCenter) * " +
        IndexedParameter("shadowScale", light, shadow) + " + " +
        IndexedParameter("shadowBias", light, shadow) + ", 0.0, 1.0);\n";
    source += "shadow *= sqrt(shadowR);\n";

    return source;
}


static string
ShadowsForLightSource(const ShaderProperties& props, unsigned int light)
{
    string source = BeginLightSourceShadows(props, light);

    for (unsigned int i = 0; i < props.getShadowCountForLight(light); i++)
        source += Shadow(light, i);

    return source;
}


string
TextureSamplerDeclarations(const ShaderProperties& props)
{
    string source;

    // Declare texture samplers
    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        source += "uniform sampler2D diffTex;\n";
    }

    if (props.texUsage & ShaderProperties::NormalTexture)
    {
        source += "uniform sampler2D normTex;\n";
    }

    if (props.texUsage & ShaderProperties::SpecularTexture)
    {
        source += "uniform sampler2D specTex;\n";
    }

    if (props.texUsage & ShaderProperties::NightTexture)
    {
        source += "uniform sampler2D nightTex;\n";
    }

    if (props.texUsage & ShaderProperties::OverlayTexture)
    {
        source += "uniform sampler2D overlayTex;\n";
    }
    
    return source;
}


string
TextureCoordDeclarations(const ShaderProperties& props)
{
    string source;
    
    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        source += "varying vec2 diffTexCoord;\n";
    }

    if (!props.hasSharedTextureCoords())
    {
        if (props.texUsage & ShaderProperties::NormalTexture)
            source += "varying vec2 normTexCoord;\n";
        if (props.texUsage & ShaderProperties::SpecularTexture)
            source += "varying vec2 specTexCoord;\n";
        if (props.texUsage & ShaderProperties::NightTexture)
            source += "varying vec2 nightTexCoord;\n";
    }
    
    if (props.texUsage & ShaderProperties::OverlayTexture)
    {
        source += "varying vec2 overlayTexCoord;\n";
    }

    return source;    
}


GLVertexShader*
ShaderManager::buildVertexShader(const ShaderProperties& props)
{
    string source;

    source += DeclareLights(props);
    if (props.lightModel == ShaderProperties::SpecularModel)
        source += "uniform float shininess;\n";

    source += "uniform vec3 eyePosition;\n";

    source += TextureCoordDeclarations(props);
    source += "uniform float textureOffset;\n";

    if (props.usesTangentSpaceLighting())
    {
        source += "attribute vec3 tangent;\n";
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            source += "varying vec3 " + LightDir(i) + ";\n";
        }

        if (props.hasSpecular())
        {
            if (props.lightModel == ShaderProperties::PerPixelSpecularModel)
            {
                source += "vec3 eyeDirObj = normalize(eyePosition - gl_Vertex.xyz);\n";
                source += "varying vec3 eyeDir;\n";
            }
            else
            {
                source += "vec3 eyeDir = normalize(eyePosition - gl_Vertex.xyz);\n";
            }
        }
    }
    else if (props.lightModel == ShaderProperties::PerPixelSpecularModel)
    {
        source += "varying vec4 diffFactors;\n";
        source += "varying vec3 normal;\n";
        source += "vec3 eyeDir = normalize(eyePosition - gl_Vertex.xyz);\n";
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            source += "varying vec3 " + LightHalfVector(i) + ";\n";
        }
    }
    else if (props.usesShadows())
    {
        source += "varying vec4 diffFactors;\n";
        if (props.lightModel == ShaderProperties::SpecularModel)
        {
            source += "varying vec4 specFactors;\n";
            source += "vec3 eyeDir = normalize(eyePosition - gl_Vertex.xyz);\n";
        }
    }
    else
    {
        source += "uniform vec3 ambientColor;\n";
        source += "varying vec4 diff;\n";
        if (props.lightModel == ShaderProperties::SpecularModel)
        {
            source += "varying vec4 spec;\n";
            source += "vec3 eyeDir = normalize(eyePosition - gl_Vertex.xyz);\n";
        }
    }
        
    // Miscellaneous lighting values
    if (props.texUsage & ShaderProperties::NightTexture)
    {
        source += "varying float totalLight;\n";
        source += "uniform float nightTexMin;\n";
    }    

    // Shadow parameters
    if (props.shadowCounts != 0)
    {
        source += "varying vec3 position_obj;\n";
    }

    if (props.texUsage & ShaderProperties::RingShadowTexture)
    {
        source += "uniform float ringWidth;\n";
        source += "uniform float ringRadius;\n";
        source += "varying vec4 ringShadowTexCoord;\n";
    }
    
    if (props.texUsage & ShaderProperties::CloudShadowTexture)
    {
        source += "uniform float cloudShadowTexOffset;\n";
        source += "uniform float cloudHeight;\n";
        for (unsigned int i = 0; i < props.nLights; i++)
            source += "varying vec2 " + CloudShadowTexCoord(i) + ";\n";
    }
    
    // Begin main() function
    source += "\nvoid main(void)\n{\n";
    source += "float nDotVP;\n";
    if (props.lightModel == ShaderProperties::SpecularModel)
    {
        source += "float nDotHV;\n";
        source += "vec3 hv;\n";
    }

    if (props.texUsage & ShaderProperties::NightTexture)
    {
        source += "totalLight = 0.0;\n";
	}	

    if (props.usesTangentSpaceLighting())
    {
        source += "vec3 bitangent = cross(gl_Normal, tangent);\n";
        if (props.lightModel == ShaderProperties::PerPixelSpecularModel)
        {
            source += TangentSpaceTransform("eyeDir", "eyeDirObj");
        }
    }
    else if (props.lightModel == ShaderProperties::PerPixelSpecularModel)
    {
        source += "normal = gl_Normal;\n";
    }
    else if (props.usesShadows())
    {
    }
    else
    {
        source += "diff = vec4(ambientColor, 1.0);\n";
        if (props.hasSpecular())
            source += "spec = vec4(0.0, 0.0, 0.0, 0.0);\n";
    }

    for (unsigned int i = 0; i < props.nLights; i++)
    {
        source += DirectionalLight(i, props);
    }

    if (props.texUsage & ShaderProperties::NightTexture)
    {
        // Output the blend factor for night lights textures
        source += "totalLight = 1.0 - totalLight;\n";
        source += "totalLight = totalLight * totalLight * totalLight * totalLight;\n";
        source += "totalLight = max(totalLight, nightTexMin);\n";
    }

    unsigned int nTexCoords = 0;
    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        source += "diffTexCoord = " + TexCoord2D(nTexCoords) + ";\n";
        source += "diffTexCoord.x += textureOffset;\n";
        nTexCoords++;
    }

    if (!props.hasSharedTextureCoords())
    {
        if (props.texUsage & ShaderProperties::NormalTexture)
        {
            source += "normTexCoord = " + TexCoord2D(nTexCoords) + ";\n";
            nTexCoords++;
        }

        if (props.texUsage & ShaderProperties::SpecularTexture)
        {
            source += "specTexCoord = " + TexCoord2D(nTexCoords) + ";\n";
            nTexCoords++;
        }

        if (props.texUsage & ShaderProperties::NightTexture)
        {
            source += "nightTexCoord = " + TexCoord2D(nTexCoords) + ";\n";
            nTexCoords++;
        }
    }

    if (props.texUsage & ShaderProperties::RingShadowTexture)
    {
        source += "vec3 ringShadowProj;\n";
        for (unsigned int j = 0; j < props.nLights; j++)
        {
            source += "ringShadowProj = gl_Vertex.xyz + " +
                LightProperty(j, "direction") +
                " * max(0.0, gl_Vertex.y / -" +
                LightProperty(j, "direction") + ".y);\n";
            source += RingShadowTexCoord(j) +
                " = (length(ringShadowProj) - ringRadius) * ringWidth;\n";
        }
    }
    
    if (props.texUsage & ShaderProperties::CloudShadowTexture)
    {
        for (unsigned int j = 0; j < props.nLights; j++)
        {
            source += "{\n";
            
            // A cheap way to calculate cloud shadow texture coordinates that doesn't correctly account
            // for sun angle.
            //source += "    " + CloudShadowTexCoord(j) + " = vec2(diffTexCoord.x + cloudShadowTexOffset, diffTexCoord.y);\n";

            // Compute the intersection of the sun direction and the cloud layer (currently assumed to be a sphere)
            source += "    float s = 1.0 / (cloudHeight * cloudHeight);\n";
            source += "    float invPi = 1.0f / 3.1415927;\n";
            source += "    vec3 coeff;\n";
            source += "    coeff.x = dot(" + LightProperty(j, "direction") + ", " + LightProperty(j, "direction") + ") * s;\n";
            source += "    coeff.y = dot(" + LightProperty(j, "direction") + ", gl_Vertex.xyz) * s;\n";
            source += "    coeff.z = dot(gl_Vertex.xyz, gl_Vertex.xyz) * s - 1.0;\n";
            source += "    float disc = sqrt(coeff.y * coeff.y - coeff.x * coeff.z);\n";
            source += "    vec3 cloudSpherePos = normalize(gl_Vertex.xyz + ((-coeff.y + disc) / coeff.x) * " + LightProperty(j, "direction") + ");\n";
            
            // Find the texture coordinates at this point on the sphere by converting from rectangular to spherical; this is an
            // expensive calculation to perform per vertex.
            source += "    " + CloudShadowTexCoord(j) + " = vec2(cloudShadowTexOffset + fract(atan(cloudSpherePos.x, cloudSpherePos.z) * (invPi * 0.5) + 0.75), 0.5 - asin(cloudSpherePos.y) * invPi);\n";
            source += "}\n";
        }
    }

    if (props.texUsage & ShaderProperties::OverlayTexture)
    {
        source += "overlayTexCoord = " + TexCoord2D(nTexCoords) + ";\n";
        nTexCoords++;
    }

    if (props.shadowCounts != 0)
    {
        source += "position_obj = gl_Vertex.xyz;\n";
    }

    source += "gl_Position = ftransform();\n";
    source += "}\n";

    if (g_shaderLogFile != NULL)
    {
        *g_shaderLogFile << "Vertex shader source:\n";
        DumpShaderSource(*g_shaderLogFile, source);
        *g_shaderLogFile << '\n';
    }

    GLVertexShader* vs = NULL;
    GLShaderStatus status = GLShaderLoader::CreateVertexShader(source, &vs);
    if (status != ShaderStatus_OK)
        return NULL;
    else
        return vs;
}


GLFragmentShader*
ShaderManager::buildFragmentShader(const ShaderProperties& props)
{
    string source;

    string diffTexCoord("diffTexCoord");
    string specTexCoord("specTexCoord");
    string nightTexCoord("nightTexCoord");
    string normTexCoord("normTexCoord");
    if (props.hasSharedTextureCoords())
    {
        specTexCoord  = diffTexCoord;
        nightTexCoord = diffTexCoord;
        normTexCoord  = diffTexCoord;
    }

    source += TextureSamplerDeclarations(props);
    source += TextureCoordDeclarations(props);        
    
    // Declare lighting parameters
    if (props.usesTangentSpaceLighting())
    {
        source += "uniform vec3 ambientColor;\n";
        source += "vec4 diff = vec4(ambientColor, 1.0);\n";
        if (props.lightModel == ShaderProperties::PerPixelSpecularModel)
        {
            source += "varying vec3 eyeDir;\n";
            source += "vec4 spec = vec4(0.0);\n";
            source += "uniform float shininess;\n";
        }
        else if (props.lightModel == ShaderProperties::SpecularModel)
        {
            source += "varying vec4 specFactors;\n";
            source += "vec4 spec = vec4(0.0);\n";
        }
            
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            source += "varying vec3 " + LightDir(i) + ";\n";
            source += "uniform vec3 " + FragLightProperty(i, "color") + ";\n";
            if (props.hasSpecular())
            {
                source += "uniform vec3 " + FragLightProperty(i, "specColor") + ";\n";
            }
        }
    }
    else if (props.lightModel == ShaderProperties::PerPixelSpecularModel)
    {
        source += "uniform vec3 ambientColor;\n";
        source += "varying vec4 diffFactors;\n";
        source += "vec4 diff = vec4(ambientColor, 1.0);\n";
        source += "varying vec3 normal;\n";        
        source += "vec4 spec = vec4(0.0);\n";
        source += "uniform float shininess;\n";
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            source += "varying vec3 " + LightHalfVector(i) + ";\n";
            source += "uniform vec3 " + FragLightProperty(i, "color") + ";\n";
            source += "uniform vec3 " + FragLightProperty(i, "specColor") + ";\n";
        }
    }
    else if (props.usesShadows())
    {
        source += "uniform vec3 ambientColor;\n";
        source += "vec4 diff = vec4(ambientColor, 1.0);\n";
        source += "varying vec4 diffFactors;\n";
        if (props.lightModel == ShaderProperties::SpecularModel)
        {
            source += "varying vec4 specFactors;\n";
            source += "vec4 spec = vec4(0.0);\n";
        }
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            source += "uniform vec3 " + FragLightProperty(i, "color") + ";\n";
            if (props.lightModel == ShaderProperties::SpecularModel)
                source += "uniform vec3 " + FragLightProperty(i, "specColor") + ";\n";
        }
    }
    else
    {
        source += "varying vec4 diff;\n";
        if (props.lightModel == ShaderProperties::SpecularModel)
        {
            source += "varying vec4 spec;\n";
        }
    }

    // Miscellaneous lighting values
    if (props.texUsage & ShaderProperties::NightTexture)
    {
        source += "varying float totalLight;\n";    
    }
    
    // Declare shadow parameters
    if (props.shadowCounts != 0)
    {
        source += "varying vec3 position_obj;\n";
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            for (unsigned int j = 0; j < props.getShadowCountForLight(i); j++)
            {
                source += "uniform vec4 " +
                    IndexedParameter("shadowTexGenS", i, j) + ";\n";
                source += "uniform vec4 " +
                    IndexedParameter("shadowTexGenT", i, j) + ";\n";
                source += "uniform float " +
                    IndexedParameter("shadowScale", i, j) + ";\n";
                source += "uniform float " +
                    IndexedParameter("shadowBias", i, j) + ";\n";
            }
        }
    }
    
    if (props.texUsage & ShaderProperties::RingShadowTexture)
    {
        source += "uniform sampler2D ringTex;\n";
        source += "varying vec4 ringShadowTexCoord;\n";
    }

    if (props.texUsage & ShaderProperties::CloudShadowTexture)
    {
        source += "uniform sampler2D cloudShadowTex;\n";
        for (unsigned int i = 0; i < props.nLights; i++)
            source += "varying vec2 " + CloudShadowTexCoord(i) + ";\n";
    }
    
    source += "\nvoid main(void)\n{\n";
    source += "vec4 color;\n";

    if (props.usesShadows())
    {
        // Temporaries required for shadows
        source += "float shadow;\n";
        if (props.shadowCounts != 0)
        {
            source += "vec2 shadowCenter;\n";
            source += "float shadowR;\n";
        }
    }
    
    // Sum the illumination from each light source, computing a total diffuse and specular
    // contributions from all sources.
    if (props.usesTangentSpaceLighting())
    {
        // Get the normal in tangent space. Ordinarily it comes from the normal texture, but if one
        // isn't provided, we'll simulate a smooth surface by using a constant (in tangent space)
        // normal of [ 0 0 1 ]
        // TODO: normalizing the filtered normal texture value noticeably improves the appearance; add
        // an option for this.
        if (props.texUsage & ShaderProperties::NormalTexture)
        {
            //source += "vec3 n = normalize(texture2D(normTex, " + normTexCoord + ".st).xyz * 2.0 - vec3(1.0, 1.0, 1.0));\n";
            source += "vec3 n = texture2D(normTex, " + normTexCoord + ".st).xyz * 2.0 - vec3(1.0, 1.0, 1.0);\n";
        }
        else
        {
            source += "vec3 n = vec3(0.0, 0.0, 1.0);\n";
        }
        
        source += "float l;\n";
        if (props.lightModel == ShaderProperties::PerPixelSpecularModel)
        {
            source += "vec3 eyeDirN = normalize(eyeDir);\n";
            source += "vec3 hv;\n";
            source += "float nDotHV;\n";
        }
        
        for (unsigned i = 0; i < props.nLights; i++)
        {
            // Bump mapping with self shadowing
            // TODO: normalize the light direction (optionally--not as important for finely tesselated
            // geometry like planet spheres.)
            // source += LightDir(i) + " = normalize(" + LightDir(i) + ");\n";
            source += "l = max(0.0, dot(" + LightDir(i) + ", n)) * clamp(" + LightDir(i) + ".z * 8.0, 0.0, 1.0);\n";

            string illum;
            if (props.hasShadowsForLight(i))
                illum = string("l * shadow");
            else
                illum = string("l");

            if (props.hasShadowsForLight(i))
                source += ShadowsForLightSource(props, i);

            source += "diff.rgb += " + illum + " * " +
                FragLightProperty(i, "color") + ";\n";

            if (props.lightModel == ShaderProperties::SpecularModel && props.usesShadows())
            {
                source += "spec.rgb += " + illum + " * " + SeparateSpecular(i) +
                    " * " + FragLightProperty(i, "specColor") +
                    ";\n";
            }
            else if (props.lightModel == ShaderProperties::PerPixelSpecularModel)
            {
                source += "hv = normalize(eyeDir + " + LightDir(i) + ");\n";
                source += "nDotHV = max(0.0, dot(n, hv));\n";
                source += "spec.rgb += " + illum + " * pow(nDotHV, shininess) * " + FragLightProperty(i, "specColor") + ";\n";
            }
        }        
    }
    else if (props.lightModel == ShaderProperties::PerPixelSpecularModel)
    {
        source += "float nDotHV;\n";
        source += "vec3 n = normalize(normal);\n";
        
        // Sum the contributions from each light source
        for (unsigned i = 0; i < props.nLights; i++)
        {
            string illum;
            
            if (props.hasShadowsForLight(i))
                illum = string("shadow");
            else
                illum = SeparateDiffuse(i);

            if (props.hasShadowsForLight(i))
                source += ShadowsForLightSource(props, i);

            source += "diff.rgb += " + illum + " * " + FragLightProperty(i, "color") + ";\n";            
            source += "nDotHV = max(0.0, dot(n, normalize(" + LightHalfVector(i) + ")));\n";
            source += "spec.rgb += " + illum + " * pow(nDotHV, shininess) * " + FragLightProperty(i, "specColor") + ";\n";
        }
    }
    else if (props.usesShadows())
    {
        // Sum the contributions from each light source
        for (unsigned i = 0; i < props.nLights; i++)
        {
            source += ShadowsForLightSource(props, i);
            source += "diff.rgb += shadow * " +
                FragLightProperty(i, "color") + ";\n";
            if (props.lightModel == ShaderProperties::SpecularModel)
            {
                source += "spec.rgb += shadow * " + SeparateSpecular(i) +
                    " * " +
                    FragLightProperty(i, "specColor") + ";\n";
            }
        }
    }
    
    if (props.texUsage & ShaderProperties::DiffuseTexture)
        source += "color = texture2D(diffTex, " + diffTexCoord + ".st);\n";
    else
        source += "color = vec4(1.0, 1.0, 1.0, 1.0);\n";
        
    // Mix in the overlay color with the base color
    if (props.texUsage & ShaderProperties::OverlayTexture)
    {
        source += "vec4 overlayColor = texture2D(overlayTex, overlayTexCoord.st);\n";
        source += "color.rgb = mix(color.rgb, overlayColor.rgb, overlayColor.a);\n";
    }

    if (props.hasSpecular())
    {
        // Add in the specular color
        if (props.texUsage & ShaderProperties::SpecularInDiffuseAlpha)
            source += "gl_FragColor = color * diff + float(color.a) * spec;\n";
        else if (props.texUsage & ShaderProperties::SpecularTexture)
            source += "gl_FragColor = color * diff + texture2D(specTex, " + specTexCoord + ".st) * spec;\n";
        else
            source += "gl_FragColor = color * diff + spec;\n";
    }
    else
    {
        source += "gl_FragColor = color * diff;\n";
    }
    
    // Add in the emissive color
    // TODO: support a constant emissive color, not just an emissive texture
    if (props.texUsage & ShaderProperties::NightTexture)
    {
        source += "gl_FragColor += texture2D(nightTex, " + nightTexCoord + ".st) * totalLight;\n";
    }

    source += "}\n";

    if (g_shaderLogFile != NULL)
    {
        *g_shaderLogFile << "Fragment shader source:\n";
        DumpShaderSource(*g_shaderLogFile, source);
        *g_shaderLogFile << '\n';
    }
    
    GLFragmentShader* fs = NULL;
    GLShaderStatus status = GLShaderLoader::CreateFragmentShader(source, &fs);
    if (status != ShaderStatus_OK)
        return NULL;
    else
        return fs;
}


GLVertexShader*
ShaderManager::buildRingsVertexShader(const ShaderProperties& props)
{
    string source;

    source += DeclareLights(props);
    source += "uniform vec3 eyePosition;\n";

    source += "varying vec4 diffFactors;\n";

    if (props.texUsage & ShaderProperties::DiffuseTexture)
        source += "varying vec2 diffTexCoord;\n";

    if (props.shadowCounts != 0)
    {
        source += "varying vec3 position_obj;\n";
        source += "varying vec4 shadowDepths;\n";
    }

    source += "\nvoid main(void)\n{\n";
    source += "float nDotVP;\n";

    // Get the normalized direction from the eye to the vertex
    source += "vec3 eyeDir = normalize(eyePosition - gl_Vertex.xyz);\n";

    for (unsigned int i = 0; i < props.nLights; i++)
    {
        source += SeparateDiffuse(i) + " = (dot(" +
            LightProperty(i, "direction") + ", eyeDir) + 1.0) * 0.5;\n";
    }

    if (props.texUsage & ShaderProperties::DiffuseTexture)
        source += "diffTexCoord = " + TexCoord2D(0) + ";\n";

    if (props.shadowCounts != 0)
    {
        source += "position_obj = gl_Vertex.xyz;\n";
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            source += ShadowDepth(i) + " = dot(gl_Vertex.xyz, " +
                       LightProperty(i, "direction") + ");\n";
        }
    }

    source += "gl_Position = ftransform();\n";
    source += "}\n";

    if (g_shaderLogFile != NULL)
    {
        *g_shaderLogFile << "Vertex shader source:\n";
        DumpShaderSource(*g_shaderLogFile, source);
        *g_shaderLogFile << '\n';
    }

    GLVertexShader* vs = NULL;
    GLShaderStatus status = GLShaderLoader::CreateVertexShader(source, &vs);
    if (status != ShaderStatus_OK)
        return NULL;
    else
        return vs;
}


GLFragmentShader*
ShaderManager::buildRingsFragmentShader(const ShaderProperties& props)
{
    string source;

    source += "uniform vec3 ambientColor;\n";
    source += "vec4 diff = vec4(ambientColor, 1.0);\n";
    for (unsigned int i = 0; i < props.nLights; i++)
        source += "uniform vec3 " + FragLightProperty(i, "color") + ";\n";

    source += "varying vec4 diffFactors;\n";

    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        source += "varying vec2 diffTexCoord;\n";
        source += "uniform sampler2D diffTex;\n";
    }

    
    if (props.shadowCounts != 0)
    {
        source += "varying vec3 position_obj;\n";
        source += "varying vec4 shadowDepths;\n";
        
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            for (unsigned int j = 0; j < props.getShadowCountForLight(i); j++)
            {
                source += "uniform vec4 " +
                    IndexedParameter("shadowTexGenS", i, j) + ";\n";
                source += "uniform vec4 " +
                    IndexedParameter("shadowTexGenT", i, j) + ";\n";
                source += "uniform float " +
                    IndexedParameter("shadowScale", i, j) + ";\n";
                source += "uniform float " +
                    IndexedParameter("shadowBias", i, j) + ";\n";
            }
        }
    }

    source += "\nvoid main(void)\n{\n";
    source += "vec4 color;\n";

    if (props.usesShadows())
    {
        // Temporaries required for shadows
        source += "float shadow;\n";
        source += "vec2 shadowCenter;\n";
        source += "float shadowR;\n";
    }

    // Sum the contributions from each light source
    for (unsigned i = 0; i < props.nLights; i++)
    {
        if (props.usesShadows())
        {
            source += "shadow = 1.0;\n";
            source += Shadow(i, 0);
            source += "shadow = min(1.0, shadow + step(0.0, " + ShadowDepth(i) + "));\n";
            source += "diff.rgb += (shadow * " + SeparateDiffuse(i) + ") * " +
                FragLightProperty(i, "color") + ";\n";
        }
        else
        {
            source += "diff.rgb += " + SeparateDiffuse(i) + " * " +
                FragLightProperty(i, "color") + ";\n";
        }
    }

    if (props.texUsage & ShaderProperties::DiffuseTexture)
        source += "color = texture2D(diffTex, diffTexCoord.st);\n";
    else
        source += "color = vec4(1.0, 1.0, 1.0, 1.0);\n";

    source += "gl_FragColor = color * diff;\n";

    source += "}\n";

    if (g_shaderLogFile != NULL)
    {
        *g_shaderLogFile << "Fragment shader source:\n";
        DumpShaderSource(*g_shaderLogFile, source);
        *g_shaderLogFile << '\n';
    }
    
    GLFragmentShader* fs = NULL;
    GLShaderStatus status = GLShaderLoader::CreateFragmentShader(source, &fs);
    if (status != ShaderStatus_OK)
        return NULL;
    else
        return fs;
}


CelestiaGLProgram*
ShaderManager::buildProgram(const ShaderProperties& props)
{
    GLProgram* prog = NULL;
    GLShaderStatus status;

    GLVertexShader* vs = NULL;
    GLFragmentShader* fs = NULL;

    if (props.lightModel == ShaderProperties::RingIllumModel)
    {
        vs = buildRingsVertexShader(props);
        fs = buildRingsFragmentShader(props);
    }
    else
    {
        vs = buildVertexShader(props);
        fs = buildFragmentShader(props);
    }

    if (vs != NULL && fs != NULL)
    {
        status = GLShaderLoader::CreateProgram(*vs, *fs, &prog);
        if (status == ShaderStatus_OK)
        {
            if (props.texUsage & ShaderProperties::NormalTexture)
            {
                // Tangents always in attribute 6 (should be a constant
                // someplace)
                glx::glBindAttribLocationARB(prog->getID(), 6, "tangent");
            }
            status = prog->link();
        }
    }
    else
    {
        status = ShaderStatus_CompileError;
        if (vs != NULL)
            delete vs;
        if (fs != NULL)
            delete fs;
    }

    if (status != ShaderStatus_OK)
    {
        // If the shader creation failed for some reason, substitute the
        // error shader.
        status = GLShaderLoader::CreateProgram(errorVertexShaderSource,
                                               errorFragmentShaderSource,
                                               &prog);
        if (status != ShaderStatus_OK)
        {
            if (g_shaderLogFile != NULL)
                *g_shaderLogFile << "Failed to create error shader!\n";
        }
        else
        {
            status = prog->link();
        }
    }

    if (prog == NULL)
        return NULL;
    else
        return new CelestiaGLProgram(*prog, props);
}
