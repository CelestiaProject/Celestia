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
#include <cstdio>

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
    lightModel(DiffuseModel)
{
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
    }

    if (props.lightModel == ShaderProperties::SpecularModel)
    {
        shininess            = floatParam("shininess");
    }

    ambientColor = vec3Param("ambientColor");
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

    if (props.texUsage & ShaderProperties::SpecularTexture)
    {
        int slot = glx::glGetUniformLocationARB(program->getID(), "specTex");
        if (slot != -1)
            glx::glUniform1iARB(slot, nSamplers++);
    }

    if (props.texUsage & ShaderProperties::NormalTexture)
    {
        int slot = glx::glGetUniformLocationARB(program->getID(), "normTex");
        if (slot != -1)
            glx::glUniform1iARB(slot, nSamplers++);
    }

    if (props.texUsage & ShaderProperties::NightTexture)
    {
        int slot = glx::glGetUniformLocationARB(program->getID(), "nightTex");
        if (slot != -1)
            glx::glUniform1iARB(slot, nSamplers++);
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
    
    source += "diff += vec4(" + LightProperty(i, "diffuse") + " * nDotVP, 1);\n";
    if (props.lightModel == ShaderProperties::SpecularModel)
    {
        source += "spec += vec4(" + LightProperty(i, "specular") +
            " * (pow(nDotHV, shininess) * nDotVP), 1);\n";
    }

    if (props.texUsage & ShaderProperties::NightTexture)
    {
        source += "totalLight += nDotVP;\n";
    }


    return source;
}


static string
DirectionalLightSpecular(unsigned int i)
{
    string source;

    source += "nDotVP = max(0.0, dot(gl_Normal, vec3(" +
        LightProperty(i, "direction") + ")));\n";
    source += "nDotHV = max(0.0, dot(gl_Normal, vec3(" +
        LightProperty(i, "halfVector") + ")));\n";

    source += "diff += vec4(" + LightProperty(i, "diffuse") + " * nDotVP, 1);\n";
    source += "spec += vec4(" + LightProperty(i, "specular") +
        " * (pow(nDotHV, shininess) * nDotVP), 1);\n";

    return source;
}


GLVertexShader*
ShaderManager::buildVertexShader(const ShaderProperties& props)
{
    string source;

    source += DeclareLights(props);
    if (props.lightModel == ShaderProperties::SpecularModel)
        source += "uniform float shininess;\n";
    source += "uniform vec3 ambientColor;\n";

    source += "varying vec4 diff;\n";
    if (props.lightModel == ShaderProperties::SpecularModel)
        source += "varying vec4 spec;\n";

    if (props.texUsage & ShaderProperties::DiffuseTexture)
        source += "varying vec2 diffTexCoord;\n";
    if (props.texUsage & ShaderProperties::SpecularTexture)
        source += "varying vec2 specTexCoord;\n";
    if (props.texUsage & ShaderProperties::NormalTexture)
    {
        source += "varying vec2 normTexCoord;\n";
        for (unsigned int i = 0; i < props.nLights; i++)
            source += "varying vec3 " + LightDir(i) + ";\n";
    }
    if (props.texUsage & ShaderProperties::NightTexture)
    {
        source += "varying vec2 nightTexCoord;\n";
        source += "varying float totalLight;\n";
    }

    if (props.texUsage & ShaderProperties::NormalTexture)
        source += "attribute vec3 tangent;\n";

    source += "\nvoid main(void)\n{\n";
    source += "float nDotVP;\n";
    if (props.lightModel == ShaderProperties::SpecularModel)
    {
        source += "float nDotHV;\n";
    }

    source += "diff = vec4(ambientColor, 1);\n";
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

    source += "gl_Position = ftransform();\n";
    source += "}\n";

    *logFile << "Vertex shader source:\n";
    *logFile << source << '\n';

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
    bool perFragmentLighting = ((props.texUsage & ShaderProperties::NormalTexture) != 0);

    if (perFragmentLighting)
    {
        source += "vec4 diff = vec4(0, 0, 0, 1);\n";
        for (unsigned int i = 0; i < props.nLights; i++)
            source += "uniform vec3 " + FragLightProperty(i, "color") + ";\n";
    }
    else
    {
        source += "varying vec4 diff;\n";
    }

    if (props.lightModel == ShaderProperties::SpecularModel)
        source += "varying vec4 spec;\n";

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

    source += "\nvoid main(void)\n{\n";
    source += "vec4 color;\n";

    if (props.texUsage & ShaderProperties::NormalTexture)
    {
        source += "vec3 n = texture2D(normTex, normTexCoord.st).xyz * 2 - vec3(1, 1, 1);\n";
        source += "float l;\n";
        for (unsigned i = 0; i < props.nLights; i++)
        {
            // Bump mapping with self shadowing
            source += "l = max(0, dot(" + LightDir(i) + ", n)) * clamp(" + LightDir(i) + ".z * 8, 0, 1);\n";
            source += "diff += vec4(l * " + FragLightProperty(i, "color") + ", 0);\n";
        }
    }

    if (props.texUsage & ShaderProperties::DiffuseTexture)
        source += "color = texture2D(diffTex, diffTexCoord.st);\n";
    else
        source += "color = diff;\n";

    if (props.lightModel == ShaderProperties::SpecularModel)
    {
        if (props.texUsage & ShaderProperties::SpecularInDiffuseAlpha)
            source += "gl_FragColor = color * diff + color.a * spec;\n";
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

    //source += "gl_FragColor = vec4(lightcolor0, 1);\n";

    source += "}\n";

    *logFile << "Fragment shader source:\n";
    *logFile << source << '\n';
    
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

    GLVertexShader* vs = buildVertexShader(props);
    GLFragmentShader* fs = buildFragmentShader(props);
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
