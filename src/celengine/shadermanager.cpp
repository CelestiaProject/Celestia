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
    initProperties(props);
};


CelestiaGLProgram::~CelestiaGLProgram()
{
    delete program;
}


FloatShaderProperty
CelestiaGLProgram::floatProperty(const string& propertyName)
{
    return FloatShaderProperty(program->getID(), propertyName.c_str());
}


Vec3ShaderProperty
CelestiaGLProgram::vec3Property(const string& propertyName)
{
    return Vec3ShaderProperty(program->getID(), propertyName.c_str());
}


Vec4ShaderProperty
CelestiaGLProgram::vec4Property(const string& propertyName)
{
    return Vec4ShaderProperty(program->getID(), propertyName.c_str());
}


static string
LightProperty(unsigned int i, char* property)
{
    char buf[64];
    sprintf(buf, "lights[%d].%s", i, property);
    return string(buf);
}


void
CelestiaGLProgram::initProperties(const ShaderProperties& props)
{
    for (unsigned int i = 0; i < props.nLights; i++)
    {
        lights[i].direction  = vec3Property(LightProperty(i, "direction"));
        lights[i].diffuse    = vec3Property(LightProperty(i, "diffuse"));
        lights[i].specular   = vec3Property(LightProperty(i, "specular"));
        lights[i].halfVector = vec3Property(LightProperty(i, "halfVector"));
    }

    if (props.lightModel == ShaderProperties::SpecularModel)
    {
        shininess            = floatProperty("shininess");
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
DirectionalLight(unsigned int i)
{
    string source;

    source += "nDotVP = max(0.0, dot(gl_Normal, vec3(" +
        LightProperty(i, "direction") + ")));\n";
    
    source += "diff += vec4(" + LightProperty(i, "diffuse") + " * nDotVP, 1);\n";

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

    source += "varying vec4 diff;\n";
    if (props.lightModel == ShaderProperties::SpecularModel)
        source += "varying vec4 spec;\n";
    if (props.texUsage & ShaderProperties::DiffuseTexture)
        source += "varying vec2 diffTexCoord;\n";

    source += "\nvoid main(void)\n{\n";
    source += "float nDotVP;\n";
    if (props.lightModel == ShaderProperties::SpecularModel)
    {
        source += "float nDotHV;\n";
    }

    source += "diff = 0;\n";
    for (unsigned int i = 0; i < props.nLights; i++)
    {
        if (props.lightModel == ShaderProperties::SpecularModel)
            source += DirectionalLightSpecular(i);
        else
            source += DirectionalLight(i);
    }
    
    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        source += "diffTexCoord = vec2(gl_MultiTexCoord0);\n";
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

    source += "varying vec4 diff;\n";
    if (props.lightModel == ShaderProperties::SpecularModel)
        source += "varying vec4 spec;\n";

    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        source += "varying vec2 diffTexCoord;\n";
        source += "uniform sampler2D diffTex;\n";
    }

    source += "\nvoid main(void)\n{\n";
    source += "vec4 color;\n";

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
