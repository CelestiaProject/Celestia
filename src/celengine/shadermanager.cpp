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
    else if (p0.texUsage < p1.texUsage)
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


GLProgram*
ShaderManager::getShader(const ShaderProperties& props)
{
    //map<ShaderProperties*, GLProgram*>::
    map<ShaderProperties, GLProgram*>::iterator iter = shaders.find(props);
    if (iter != shaders.end())
    {
        // Shader already exists
        return iter->second;
    }
    else
    {
        // Create a new shader and add it to the table of created shaders
        GLProgram* prog = buildProgram(props);
        shaders[props] = prog;

        return prog;
    }
}



static string
LightProperty(unsigned int i, char* property)
{
    char buf[64];
    sprintf(buf, "gl_LightSource[%d].%s", i, property);
    return string(buf);
}


static string
DirectionalLight(unsigned int i)
{
    string source;

    source += "nDotVP = max(0.0, dot(gl_Normal, vec3(" +
        LightProperty(i, "position") + ")));\n";
    
    source += "diff += " + LightProperty(i, "diffuse") + " * nDotVP;\n";

    return source;
}


static string
DirectionalLightSpecular(unsigned int i)
{
    string source;

    source += "nDotVP = max(0.0, dot(gl_Normal, vec3(" +
        LightProperty(i, "position") + ")));\n";
    source += "nDotHV = max(0.0, dot(gl_Normal, vec3(" +
        LightProperty(i, "halfVector") + ")));\n";

    source += "diff += " + LightProperty(i, "diffuse") + " * nDotVP;\n";
    source += "spec += " + LightProperty(i, "diffuse") +
        " * (pow(nDotHV, gl_FrontMaterial.shininess) * nDotVP);\n";

    return source;
}


#if 0
static char* dirLightSource = 
    "void DirectionalLightSpecular(in vec3 lightDir,\n"
    "                              in vec3 halfVec,\n"
    "                              in vec3 normal,\n"
    "                              in float specExp,\n"
    "                              inout vec4 diffuse,\n"
    "                              inout vec4 specular)\n"
    "{\n";
    "    float nDotVP = max(0.0, dot(normal, lightDir));\n"
    "    float nDotHV = max(0.0, dot(normal, halfVec));\n"
    "    float pf = pow(nDotHV, specExp"
#endif
    

GLVertexShader*
ShaderManager::buildVertexShader(const ShaderProperties& props)
{
    string source;

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
        source += "diffTexCoord = gl_MultiTexCoord0;\n";
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
        source += "gl_FragColor = color * diff + spec;\n";
    else
        source += "gl_FragColor = color * diff;\n";
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


GLProgram*
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

#if 0    
    status = GLShaderLoader::CreateProgram(defaultVertexShaderSource,
                                           defaultFragmentShaderSource,
                                           &prog);
#endif

    if (status != ShaderStatus_OK)
    {
        // If the shader creation failed for some reason, substitute the
        // error shader.
        *logFile << "Error creating shader.\n";
        GLShaderLoader::CreateProgram(errorVertexShaderSource,
                                      errorFragmentShaderSource,
                                      &prog);
    }

    return prog;
}
