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


ShaderManager g_ShaderManager;


static const char* errorVertexShaderSource = 
    "void main(void) {\n"
    "   gl_Position = ftransform();\n"
    "}\n";
static const char* errorFragmentShaderSource =
    "void main(void) {\n"
    "   gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
    "}\n";

static const char* defaultVertexShaderSource = 
    "varying vec4 diff;\n"
    "varying vec2 tex0;\n"
    "void main(void) {\n"
    "   diff = max(0.0, dot(gl_Normal, gl_LightSource[0].position));\n"
    "   tex0 = gl_MultiTexCoord0;\n"
    "   gl_Position = ftransform();\n"
    "}\n";
static const char* defaultFragmentShaderSource =
    "uniform sampler2D diffTex;\n"
    "varying vec4 diff;\n"
    "varying vec2 tex0;\n"
    "void main(void) {\n"
    "   gl_FragColor = diff * texture2D(diffTex, tex0.st);\n"
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
    if ((texUsage & RingShadowTexture) != 0 || shadowCounts != 0)
        return true;
    else
        return false;
}


bool
ShaderProperties::usesFragmentLighting() const
{
    if ((texUsage & NormalTexture) != 0)
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
    if (n < MaxShaderShadows && light < MaxShaderLights)
    {
        shadowCounts &= ~(3 << light * 2);
        shadowCounts |= n << light * 2;
    }
}


bool
ShaderProperties::hasShadowsForLight(unsigned int light) const
{
    assert(light < MaxShaderLights);
    return ((getShadowCountForLight(light) != 0) ||
            (texUsage & RingShadowTexture));
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


ShaderManager::ShaderManager() :
    logFile(NULL)
{
    logFile = new ofstream("shaders.log");
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
    sprintf(buf, "lights[%d].%s", i, property);
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

    if (props.lightModel == ShaderProperties::SpecularModel)
    {
        shininess            = floatParam("shininess");
    }

    if (props.lightModel == ShaderProperties::RingIllumModel)
    {
        // TODO: Eye position also required for specular lighting with
        // local viewer.
        eyePosition          = vec3Param("eyePosition");
    }

    ambientColor = vec3Param("ambientColor");

    if (props.texUsage & ShaderProperties::RingShadowTexture)
    {
        ringWidth            = floatParam("ringWidth");
        ringRadius           = floatParam("ringRadius");
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
}


static string
DeclareLights(const ShaderProperties& props)
{
    if (props.nLights == 0)
        return string("");

    char lightSourceBuf[128];
    sprintf(lightSourceBuf,
            "struct {\n"
            "   vec3 direction;\n"
            "   vec3 diffuse;\n"
            "   vec3 specular;\n"
            "   vec3 halfVector;\n"
            "} uniform lights[%d];\n",
            props.nLights);

    return string(lightSourceBuf);
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
DirectionalLight(unsigned int i, const ShaderProperties& props)
{
    string source;

    source += "nDotVP = max(0.0, dot(gl_Normal, " +
        LightProperty(i, "direction") + "));\n";
    if (props.lightModel == ShaderProperties::SpecularModel)
    {
        source += "nDotHV = max(0.0, dot(gl_Normal, " +
            LightProperty(i, "halfVector") + "));\n";
    }

    if (props.usesFragmentLighting())
    {
        // Diffuse is computed in the fragment shader when fragment lighting
        // is enabled.
    }
    else if (props.usesShadows())
    {
        // When there are shadows, we need to track the diffuse contributions
        // separately for each light.
        source += SeparateDiffuse(i) + " = nDotVP;\n";
    }
    else
    {
        // Sum the diffuse contribution from all lights
        source += "diff += vec4(" + LightProperty(i, "diffuse") + " * nDotVP, 1);\n";
    }

    if (props.lightModel == ShaderProperties::SpecularModel)
    {
        if (props.usesShadows())
        {
            source += SeparateSpecular(i) +
                " = pow(nDotHV, shininess);\n";
        }
        else
        {
            source += "spec += vec4(" + LightProperty(i, "specular") +
                " * (pow(nDotHV, shininess) * nDotVP), 1.0);\n";
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

    if (props.usesFragmentLighting())
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
            IndexedParameter("ringShadowTexCoord", light) + ", 0.0)).a);\n";
    }

    return source;
}


static string
Shadow(unsigned int light, unsigned int shadow)
{
    string source;

    source += "shadowCenter = " +
        IndexedParameter("shadowTexCoord", light, shadow) +
        " - vec2(0.5, 0.5);\n";
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



GLVertexShader*
ShaderManager::buildVertexShader(const ShaderProperties& props)
{
    string source;

    source += DeclareLights(props);
    if (props.lightModel == ShaderProperties::SpecularModel)
        source += "uniform float shininess;\n";

    if (!props.usesFragmentLighting())
    {
        if (!props.usesShadows())
        {
            source += "uniform vec3 ambientColor;\n";
            source += "varying vec4 diff;\n";
        }
        else 
        {
            source += "varying vec4 diffFactors;\n";
        }
    }

    if (props.lightModel == ShaderProperties::SpecularModel)
    {
        if (!props.usesShadows())
            source += "varying vec4 spec;\n";
        else
            source += "varying vec4 specFactors;\n";
    }

    if (props.texUsage & ShaderProperties::DiffuseTexture)
        source += "varying vec2 diffTexCoord;\n";
    if (props.texUsage & ShaderProperties::NormalTexture)
    {
        source += "varying vec2 normTexCoord;\n";
        for (unsigned int i = 0; i < props.nLights; i++)
            source += "varying vec3 " + LightDir(i) + ";\n";
    }
    if (props.texUsage & ShaderProperties::SpecularTexture)
        source += "varying vec2 specTexCoord;\n";
    if (props.texUsage & ShaderProperties::NightTexture)
    {
        source += "varying vec2 nightTexCoord;\n";
        source += "varying float totalLight;\n";
    }

    if (props.texUsage & ShaderProperties::RingShadowTexture)
    {
        source += "uniform float ringWidth;\n";
        source += "uniform float ringRadius;\n";
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            source += "varying float " + 
                IndexedParameter("ringShadowTexCoord", i) + ";\n";
        }
    }

    if (props.shadowCounts != 0)
    {
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            for (unsigned int j = 0; j < props.getShadowCountForLight(i); j++)
            {
                source += "varying vec2 " +
                    IndexedParameter("shadowTexCoord", i, j) + ";\n";
                source += "uniform vec4 " +
                    IndexedParameter("shadowTexGenS", i, j) + ";\n";
                source += "uniform vec4 " +
                    IndexedParameter("shadowTexGenT", i, j) + ";\n";
            }
        }
    }

    if (props.texUsage & ShaderProperties::NormalTexture)
        source += "attribute vec3 tangent;\n";

    source += "\nvoid main(void)\n{\n";
    source += "float nDotVP;\n";
    if (props.lightModel == ShaderProperties::SpecularModel)
    {
        source += "float nDotHV;\n";
    }

    if (!props.usesShadows() && !props.usesFragmentLighting())
    {
        source += "diff = vec4(ambientColor, 1);\n";
    }

    for (unsigned int i = 0; i < props.nLights; i++)
    {
        source += DirectionalLight(i, props);
    }

    if (props.texUsage & ShaderProperties::NightTexture)
    {
        // Output the blend factor for night lights textures
        source += "totalLight = 1 - totalLight;\n";
        source += "totalLight = totalLight * totalLight * totalLight * totalLight;\n";
    }

    if (props.texUsage & ShaderProperties::NormalTexture)
    {
        source += "vec3 bitangent = cross(gl_Normal, tangent);\n";
        for (unsigned int j = 0; j < props.nLights; j++)
        {
            source += LightDir(j) + ".x = dot(tangent, " + LightProperty(j, "direction") + ");\n";
            source += LightDir(j) + ".y = dot(-bitangent, " + LightProperty(j, "direction") + ");\n";
            source += LightDir(j) + ".z = dot(gl_Normal, " + LightProperty(j, "direction") + ");\n";
        }
    }

    unsigned int nTexCoords = 0;
    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        source += "diffTexCoord = " + TexCoord2D(nTexCoords) + ";\n";
        nTexCoords++;
    }

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

    if (props.texUsage & ShaderProperties::RingShadowTexture)
    {
        source += "vec3 ringShadowProj;\n";
        for (unsigned int j = 0; j < props.nLights; j++)
        {
            source += "ringShadowProj = gl_Vertex.xyz + " +
                LightProperty(j, "direction") +
                " * max(0, gl_Vertex.y / -" +
                LightProperty(j, "direction") + ".y);\n";
            source += IndexedParameter("ringShadowTexCoord", j) +
                " = length(ringShadowProj) * ringWidth - ringRadius;\n";
        }
    }

    if (props.shadowCounts != 0)
    {
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            for (unsigned int j = 0; j < props.getShadowCountForLight(i); j++)
            {
                source += IndexedParameter("shadowTexCoord", i, j) +
                    ".s = dot(gl_Vertex, " +
                    IndexedParameter("shadowTexGenS", i, j) + ");\n";
                source += IndexedParameter("shadowTexCoord", i, j) +
                    ".t = dot(gl_Vertex, " +
                    IndexedParameter("shadowTexGenT", i, j) + ");\n";
            }
        }
    }

    source += "gl_Position = ftransform();\n";
    source += "}\n";

    *logFile << "Vertex shader source:\n";
    DumpShaderSource(*logFile, source);
    *logFile << '\n';

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

    if (props.usesFragmentLighting() || props.usesShadows())
    {
        source += "uniform vec3 ambientColor;\n";
        source += "vec4 diff = vec4(ambientColor, 1);\n";
        for (unsigned int i = 0; i < props.nLights; i++)
            source += "uniform vec3 " + FragLightProperty(i, "color") + ";\n";
    }
    else
    {
        source += "varying vec4 diff;\n";
    }

    if (props.usesShadows() && !props.usesFragmentLighting())
    {
        source += "varying vec4 diffFactors;\n";
    }

    if (props.lightModel == ShaderProperties::SpecularModel)
    {
        if (props.usesShadows())
        {
            source += "varying vec4 specFactors;\n";
            source += "vec4 spec;\n";
        }
        else
        {
            source += "varying vec4 spec;\n";
        }
    }

    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        source += "varying vec2 diffTexCoord;\n";
        source += "uniform sampler2D diffTex;\n";
    }

    if (props.texUsage & ShaderProperties::NormalTexture)
    {
        source += "varying vec2 normTexCoord;\n";
        for (unsigned int i = 0; i < props.nLights; i++)
            source += "varying vec3 " + LightDir(i) + ";\n";
        source += "uniform sampler2D normTex;\n";
    }

    if (props.texUsage & ShaderProperties::SpecularTexture)
    {
        source += "varying vec2 specTexCoord;\n";
        source += "uniform sampler2D specTex;\n";
    }

    if (props.texUsage & ShaderProperties::NightTexture)
    {
        source += "varying vec2 nightTexCoord;\n";
        source += "uniform sampler2D nightTex;\n";
        source += "varying float totalLight;\n";
    }

    if (props.texUsage & ShaderProperties::RingShadowTexture)
    {
        source += "uniform sampler2D ringTex;\n";
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            source += "varying float " + 
                IndexedParameter("ringShadowTexCoord", i) + ";\n";
        }
    }

    if (props.shadowCounts != 0)
    {
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            for (unsigned int j = 0; j < props.getShadowCountForLight(i); j++)
            {
                source += "varying vec2 " +
                    IndexedParameter("shadowTexCoord", i, j) + ";\n";
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
        if (props.shadowCounts != 0)
        {
            source += "vec2 shadowCenter;\n";
            source += "float shadowR;\n";
        }
    }

    if (props.texUsage & ShaderProperties::NormalTexture)
    {
        source += "vec3 n = texture2D(normTex, normTexCoord.st).xyz * 2 - vec3(1, 1, 1);\n";
        source += "float l;\n";
        for (unsigned i = 0; i < props.nLights; i++)
        {
            // Bump mapping with self shadowing
            source += "l = max(0.0, dot(" + LightDir(i) + ", n)) * clamp(" + LightDir(i) + ".z * 8.0, 0.0, 1.0);\n";

            string illum;
            if (props.hasShadowsForLight(i))
                illum = string("l * shadow");
            else
                illum = string("l");

            if (props.hasShadowsForLight(i))
                source += ShadowsForLightSource(props, i);

            source += "diff += vec4(" + illum + " * " +
                FragLightProperty(i, "color") + ", 0);\n";

            if (props.lightModel == ShaderProperties::SpecularModel &&
                props.usesShadows())
            {
                source += "spec += " + illum + " * " + SeparateSpecular(i) +
                    " * vec4(" + FragLightProperty(i, "color") + ", 0.0);\n";
            }
        }
    }
    else if (props.usesShadows())
    {
        // Sum the contributions from each light source
        for (unsigned i = 0; i < props.nLights; i++)
        {
            source += ShadowsForLightSource(props, i);
            source += "diff += shadow * vec4(" +
                FragLightProperty(i, "color") + ", 0.0);\n";
            if (props.lightModel == ShaderProperties::SpecularModel)
            {
                source += "spec += shadow * " + SeparateSpecular(i) +
                    " * vec4(" +
                    FragLightProperty(i, "color") + ", 0.0);\n";
            }
        }
    }

    if (props.texUsage & ShaderProperties::DiffuseTexture)
        source += "color = texture2D(diffTex, diffTexCoord.st);\n";
    else
        source += "color = vec4(1.0, 1.0, 1.0, 1.0);\n";

    if (props.lightModel == ShaderProperties::SpecularModel)
    {
        if (props.texUsage & ShaderProperties::SpecularInDiffuseAlpha)
            source += "gl_FragColor = color * diff + color.a * spec;\n";
        else if (props.texUsage & ShaderProperties::SpecularTexture)
            source += "gl_FragColor = color * diff + texture2D(specTex, specTexCoord.st) * spec;\n";
        else
            source += "gl_FragColor = color * diff + spec;\n";
    }
    else
    {
        source += "gl_FragColor = color * diff;\n";
    }

    if (props.texUsage & ShaderProperties::NightTexture)
    {
        source += "gl_FragColor += texture2D(nightTex, nightTexCoord.st) * totalLight;\n";
    }

    source += "}\n";

    *logFile << "Fragment shader source:\n";
    DumpShaderSource(*logFile, source);
    *logFile << '\n';
    
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
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            source += "uniform vec4 " +
                IndexedParameter("shadowTexGenS", i, 0) + ";\n";
            source += "uniform vec4 " +
                IndexedParameter("shadowTexGenT", i, 0) + ";\n";
            source += "varying vec3 " +
                IndexedParameter("shadowTexCoord", i, 0) + ";\n";
        }
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
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            source += IndexedParameter("shadowTexCoord", i, 0) +
                ".x = dot(gl_Vertex, " +
                IndexedParameter("shadowTexGenS", i, 0) + ");\n";
            source += IndexedParameter("shadowTexCoord", i, 0) +
                ".y = dot(gl_Vertex, " +
                IndexedParameter("shadowTexGenT", i, 0) + ");\n";
            source += IndexedParameter("shadowTexCoord", i, 0) +
                ".z = dot(gl_Vertex.xyz, " +
                LightProperty(i, "direction") + ");\n";
        }
    }

    source += "gl_Position = ftransform();\n";
    source += "}\n";

    *logFile << "Vertex shader source:\n";
    DumpShaderSource(*logFile, source);
    *logFile << '\n';

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
    source += "vec4 diff = vec4(ambientColor, 1);\n";
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
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            source += "varying vec3 " +
                IndexedParameter("shadowTexCoord", i, 0) + ";\n";
            source += "uniform float " +
                IndexedParameter("shadowScale", i, 0) + ";\n";
            source += "uniform float " +
                IndexedParameter("shadowBias", i, 0) + ";\n";
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
        source += "shadow = 1.0;\n";
        source += Shadow(i, 0);
        source += "shadow = min(1.0, shadow + step(0.0, " +
            IndexedParameter("shadowTexCoord", i, 0) + ".z));\n";
        source += "diff += (shadow * " + SeparateDiffuse(i) + ") * vec4(" +
            FragLightProperty(i, "color") + ", 0.0);\n";
    }

    if (props.texUsage & ShaderProperties::DiffuseTexture)
        source += "color = texture2D(diffTex, diffTexCoord.st);\n";
    else
        source += "color = vec4(1.0, 1.0, 1.0, 1.0);\n";

    source += "gl_FragColor = color * diff;\n";

    source += "}\n";

    *logFile << "Fragment shader source:\n";
    DumpShaderSource(*logFile, source);
    *logFile << '\n';
    
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
        *logFile << "Error creating shader.\n";
        GLShaderLoader::CreateProgram(errorVertexShaderSource,
                                      errorFragmentShaderSource,
                                      &prog);
    }

    if (prog == NULL)
        return NULL;
    else
        return new CelestiaGLProgram(*prog, props);
}
