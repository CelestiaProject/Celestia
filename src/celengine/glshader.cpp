// glshader.cpp
//
// Copyright (C) 2001-2006, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include "glshader.h"
#include <GL/glew.h>

using namespace std;


static const string GetInfoLog(GLhandleARB obj);


ostream* g_shaderLogFile = NULL;


GLShader::GLShader(GLhandleARB _id) :
    id(_id)
{
}


GLhandleARB
GLShader::getID() const
{
    return id;
}


GLShaderStatus
GLShader::compile(const vector<string>& source)
{
    if (source.empty())
        return ShaderStatus_EmptyProgram;
    
    // Convert vector of shader source strings to an array for OpenGL
    const char** sourceStrings = new const char*[source.size()];
    for (unsigned int i = 0; i < source.size(); i++)
        sourceStrings[i] = source[i].c_str();

    // Copy shader source to OpenGL
    glShaderSourceARB(id, source.size(), sourceStrings, NULL);
    delete[] sourceStrings;

    // Actually compile the shader
    glCompileShaderARB(id);
    
    GLint compileSuccess;
    glGetObjectParameterivARB(id, GL_OBJECT_COMPILE_STATUS_ARB,
                                   &compileSuccess);
    if (compileSuccess == GL_FALSE)
        return ShaderStatus_CompileError;

    return ShaderStatus_OK;
}


GLShader::~GLShader()
{
    glDeleteObjectARB(id);
}



//************* GLxxxProperty **********

FloatShaderParameter::FloatShaderParameter() :
    slot(-1)
{
}

FloatShaderParameter::FloatShaderParameter(GLhandleARB obj, const char* name)
{
    slot = glGetUniformLocationARB(obj, name);
}

FloatShaderParameter&
FloatShaderParameter::operator=(float f)
{
    if (slot != -1)
        glUniform1fARB(slot, f);
    return *this;
}


Vec3ShaderParameter::Vec3ShaderParameter() :
    slot(-1)
{
}

Vec3ShaderParameter::Vec3ShaderParameter(GLhandleARB obj, const char* name)
{
    slot = glGetUniformLocationARB(obj, name);
}

Vec3ShaderParameter&
Vec3ShaderParameter::operator=(const Eigen::Vector3f& v)
{
    if (slot != -1)
        glUniform3fvARB(slot, 1, v.data());
    return *this;
}

Vec4ShaderParameter::Vec4ShaderParameter() :
    slot(-1)
{
}

Vec4ShaderParameter::Vec4ShaderParameter(GLhandleARB obj, const char* name)
{
    slot = glGetUniformLocationARB(obj, name);
}

Vec4ShaderParameter&
Vec4ShaderParameter::operator=(const Eigen::Vector4f& v)
{
    if (slot != -1)
        glUniform4fvARB(slot, 1, v.data());
    return *this;
}


//************* GLProgram **************

GLProgram::GLProgram(GLhandleARB _id) :
    id(_id)
{
}


GLProgram::~GLProgram()
{
    glDeleteObjectARB(id);
}


void
GLProgram::use() const
{
    glUseProgramObjectARB(id);
}


void
GLProgram::attach(const GLShader& shader)
{
    glAttachObjectARB(id, shader.getID());
}


GLShaderStatus
GLProgram::link()
{
    glLinkProgramARB(id);

    GLint linkSuccess;
    glGetObjectParameterivARB(id, GL_OBJECT_LINK_STATUS_ARB,
                                   &linkSuccess);
    if (linkSuccess == GL_FALSE)
    {
        if (g_shaderLogFile != NULL)
        {
            *g_shaderLogFile << "Error linking shader program:\n";
            *g_shaderLogFile << GetInfoLog(getID());
        }
        return ShaderStatus_LinkError;
    }

    return ShaderStatus_OK;
}


//************* GLShaderLoader ************

GLShaderStatus
GLShaderLoader::CreateVertexShader(const vector<string>& source,
                                   GLVertexShader** vs)
{
    GLhandleARB vsid = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);

    GLVertexShader* shader = new GLVertexShader(vsid);
    if (!shader)
        return ShaderStatus_OutOfMemory;

    GLShaderStatus status = shader->compile(source);
    if (status != ShaderStatus_OK)
    {
        if (g_shaderLogFile != NULL)
        {
            *g_shaderLogFile << "Error compiling vertex shader:\n";
            *g_shaderLogFile << GetInfoLog(shader->getID());
        }
        return status;
    }

    *vs = shader;

    return ShaderStatus_OK;
}


GLShaderStatus
GLShaderLoader::CreateFragmentShader(const vector<string>& source,
                                     GLFragmentShader** fs)
{
    GLhandleARB fsid = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);

    GLFragmentShader* shader = new GLFragmentShader(fsid);
    if (!shader)
        return ShaderStatus_OutOfMemory;

    GLShaderStatus status = shader->compile(source);
    if (status != ShaderStatus_OK)
    {
        if (g_shaderLogFile != NULL)
        {
            *g_shaderLogFile << "Error compiling fragment shader:\n";
            *g_shaderLogFile << GetInfoLog(shader->getID());
        }
        return status;
    }

    *fs = shader;

    return ShaderStatus_OK;
}


GLShaderStatus
GLShaderLoader::CreateVertexShader(const string& source,
                                   GLVertexShader** vs)
                                        
{
    vector<string> v;
    v.push_back(source);
    return CreateVertexShader(v, vs);
}


GLShaderStatus
GLShaderLoader::CreateFragmentShader(const string& source,
                                     GLFragmentShader** fs)
{
    vector<string> v;
    v.push_back(source);
    return CreateFragmentShader(v, fs);
}


GLShaderStatus
GLShaderLoader::CreateProgram(const GLVertexShader& vs,
                              const GLFragmentShader& fs,
                              GLProgram** progOut)
{
    GLhandleARB progid = glCreateProgramObjectARB();

    GLProgram* prog = new GLProgram(progid);
    if (!prog)
        return ShaderStatus_OutOfMemory;

    prog->attach(vs);
    prog->attach(fs);

    *progOut = prog;

    return ShaderStatus_OK;
}


GLShaderStatus
GLShaderLoader::CreateProgram(const vector<string>& vsSource,
                              const vector<string>& fsSource,
                              GLProgram** progOut)
{
    GLVertexShader* vs = NULL;
    GLShaderStatus status = CreateVertexShader(vsSource, &vs);
    if (status != ShaderStatus_OK)
        return status;

    GLFragmentShader* fs = NULL;
    status = CreateFragmentShader(fsSource, &fs);
    if (status != ShaderStatus_OK)
    {
        delete vs;
        return status;
    }

    GLProgram* prog = NULL;
    status = CreateProgram(*vs, *fs, &prog);
    if (status != ShaderStatus_OK)
    {
        delete vs;
        delete fs;
        return status;
    }

    *progOut = prog;

    // No need to keep these around--the program doesn't reference them
    delete vs;
    delete fs;

    return ShaderStatus_OK;
}


GLShaderStatus
GLShaderLoader::CreateProgram(const string& vsSource,
                              const string& fsSource,
                              GLProgram** progOut)
{
    vector<string> vsSourceVec;
    vsSourceVec.push_back(vsSource);
    vector<string> fsSourceVec;
    fsSourceVec.push_back(fsSource);

    return CreateProgram(vsSourceVec, fsSourceVec, progOut);
}


const string
GetInfoLog(GLhandleARB obj)
{
    GLint logLength = 0;
    GLsizei charsWritten = 0;

    glGetObjectParameterivARB(obj, GL_OBJECT_INFO_LOG_LENGTH_ARB,
                                   &logLength);
    if (logLength <= 0)
        return string();

    char* log = new char[logLength];
    if (log == NULL)
        return string();
    
    glGetInfoLogARB(obj, logLength, &charsWritten, log);
    
    string s(log, charsWritten);
    delete[] log;
    
    return s;
}
