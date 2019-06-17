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


static const string GetInfoLog(GLuint obj);


ostream* g_shaderLogFile = nullptr;


GLShader::GLShader(GLuint _id) :
    id(_id)
{
}


GLuint
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
    const auto** sourceStrings = new const char*[source.size()];
    for (unsigned int i = 0; i < source.size(); i++)
        sourceStrings[i] = source[i].c_str();

    // Copy shader source to OpenGL
    glShaderSource(id, source.size(), sourceStrings, nullptr);
    delete[] sourceStrings;

    // Actually compile the shader
    glCompileShader(id);

    GLint compileSuccess;
    glGetShaderiv(id, GL_COMPILE_STATUS, &compileSuccess);
    if (compileSuccess != GL_TRUE)
        return ShaderStatus_CompileError;

    return ShaderStatus_OK;
}


GLShader::~GLShader()
{
    glDeleteShader(id);
}



//************* GLxxxProperty **********

FloatShaderParameter::FloatShaderParameter() :
    slot(-1)
{
}

FloatShaderParameter::FloatShaderParameter(GLuint obj, const char* name)
{
    slot = glGetUniformLocation(obj, name);
}

FloatShaderParameter&
FloatShaderParameter::operator=(float f)
{
    if (slot != -1)
        glUniform1f(slot, f);
    return *this;
}


Vec3ShaderParameter::Vec3ShaderParameter() :
    slot(-1)
{
}

Vec3ShaderParameter::Vec3ShaderParameter(GLuint obj, const char* name)
{
    slot = glGetUniformLocation(obj, name);
}

Vec3ShaderParameter&
Vec3ShaderParameter::operator=(const Eigen::Vector3f& v)
{
    if (slot != -1)
        glUniform3fv(slot, 1, v.data());
    return *this;
}

Vec4ShaderParameter::Vec4ShaderParameter() :
    slot(-1)
{
}

Vec4ShaderParameter::Vec4ShaderParameter(GLuint obj, const char* name)
{
    slot = glGetUniformLocation(obj, name);
}

Vec4ShaderParameter&
Vec4ShaderParameter::operator=(const Eigen::Vector4f& v)
{
    if (slot != -1)
        glUniform4fv(slot, 1, v.data());
    return *this;
}


IntegerShaderParameter::IntegerShaderParameter() :
    slot(-1)
{
}

IntegerShaderParameter::IntegerShaderParameter(GLuint obj, const char* name)
{
    slot = glGetUniformLocation(obj, name);
}

IntegerShaderParameter&
IntegerShaderParameter::operator=(int i)
{
    if (slot != -1)
        glUniform1i(slot, i);
    return *this;
}

//************* GLProgram **************

GLProgram::GLProgram(GLuint _id) :
    id(_id)
{
}


GLProgram::~GLProgram()
{
    glDeleteProgram(id);
}


void
GLProgram::use() const
{
    glUseProgram(id);
}


void
GLProgram::attach(const GLShader& shader)
{
    glAttachShader(id, shader.getID());
}


GLShaderStatus
GLProgram::link()
{
    glLinkProgram(id);

    GLint linkSuccess;
    glGetProgramiv(id, GL_LINK_STATUS, &linkSuccess);
    if (linkSuccess != GL_TRUE)
    {
        if (g_shaderLogFile != nullptr)
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
    GLuint vsid = glCreateShader(GL_VERTEX_SHADER);

    auto* shader = new GLVertexShader(vsid);

    GLShaderStatus status = shader->compile(source);
    if (status != ShaderStatus_OK)
    {
        if (g_shaderLogFile != nullptr)
        {
            *g_shaderLogFile << "Error compiling vertex shader:\n";
            *g_shaderLogFile << GetInfoLog(shader->getID());
        }
        delete shader;
        return status;
    }

    *vs = shader;

    return ShaderStatus_OK;
}


GLShaderStatus
GLShaderLoader::CreateFragmentShader(const vector<string>& source,
                                     GLFragmentShader** fs)
{
    GLuint fsid = glCreateShader(GL_FRAGMENT_SHADER);

    auto* shader = new GLFragmentShader(fsid);

    GLShaderStatus status = shader->compile(source);
    if (status != ShaderStatus_OK)
    {
        if (g_shaderLogFile != nullptr)
        {
            *g_shaderLogFile << "Error compiling fragment shader:\n";
            *g_shaderLogFile << GetInfoLog(shader->getID());
        }
        delete shader;
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
    GLuint progid = glCreateProgram();

    auto* prog = new GLProgram(progid);

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
    GLVertexShader* vs = nullptr;
    GLShaderStatus status = CreateVertexShader(vsSource, &vs);
    if (status != ShaderStatus_OK)
        return status;

    GLFragmentShader* fs = nullptr;
    status = CreateFragmentShader(fsSource, &fs);
    if (status != ShaderStatus_OK)
    {
        delete vs;
        return status;
    }

    GLProgram* prog = nullptr;
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
GetInfoLog(GLuint obj)
{
    GLint logLength = 0;
    GLsizei charsWritten = 0;

    glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength <= 0)
        return string();

    auto* log = new char[logLength];

    glGetShaderInfoLog(obj, logLength, &charsWritten, log);

    string s(log, charsWritten);
    delete[] log;

    return s;
}
