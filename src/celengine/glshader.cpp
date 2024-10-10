// glshader.cpp
//
// Copyright (C) 2001-2006, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celutil/logger.h>
#include "glshader.h"

using celestia::util::GetLogger;


namespace
{
std::string
GetInfoLog(GLuint obj)
{
    bool isShader;
    if (glIsShader(obj))
    {
        isShader = true;
    }
    else if (glIsProgram(obj))
    {
        isShader = false;
    }
    else
    {
        GetLogger()->error("Unknown object passed to GetInfoLog()!\n");
        return std::string();
    }

    GLint logLength;
    if (isShader)
        glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &logLength);
    else
        glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &logLength);

    if (logLength <= 0)
        return std::string();

    std::string infoLog(static_cast<std::string::size_type>(logLength), '\0');

    GLsizei charsWritten;
    if (isShader)
        glGetShaderInfoLog(obj, logLength, &charsWritten, infoLog.data());
    else
        glGetProgramInfoLog(obj, logLength, &charsWritten, infoLog.data());

    if (charsWritten <= 0)
        return std::string();

    infoLog.resize(static_cast<std::string::size_type>(charsWritten));
    return infoLog;
}
} // end unnamed namespace

std::ostream* g_shaderLogFile = nullptr;


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
GLShader::compile(const std::vector<std::string>& source)
{
    if (source.empty())
        return GLShaderStatus::EmptyProgram;

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
        return GLShaderStatus::CompileError;

    return GLShaderStatus::OK;
}


GLShader::~GLShader()
{
    glDeleteShader(id);
}



//************* GLxxxProperty **********

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


Vec2ShaderParameter::Vec2ShaderParameter(GLuint obj, const char* name)
{
    slot = glGetUniformLocation(obj, name);
}

Vec2ShaderParameter&
Vec2ShaderParameter::operator=(const Eigen::Vector2f& v)
{
    if (slot != -1)
        glUniform2fv(slot, 1, v.data());
    return *this;
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


Mat3ShaderParameter::Mat3ShaderParameter(GLuint obj, const char* name)
{
    slot = glGetUniformLocation(obj, name);
}

Mat3ShaderParameter&
Mat3ShaderParameter::operator=(const Eigen::Matrix3f& v)
{
    if (slot != -1)
        glUniformMatrix3fv(slot, 1, GL_FALSE, v.data());
    return *this;
}


Mat4ShaderParameter::Mat4ShaderParameter(GLuint obj, const char* name)
{
    slot = glGetUniformLocation(obj, name);
}

Mat4ShaderParameter&
Mat4ShaderParameter::operator=(const Eigen::Matrix4f& v)
{
    if (slot != -1)
        glUniformMatrix4fv(slot, 1, GL_FALSE, v.data());
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
        return GLShaderStatus::LinkError;
    }

    return GLShaderStatus::OK;
}


//************* GLShaderLoader ************

GLShaderStatus
GLShaderLoader::CreateVertexShader(const std::vector<std::string>& source,
                                   GLVertexShader** vs)
{
    GLuint vsid = glCreateShader(GL_VERTEX_SHADER);

    auto* shader = new GLVertexShader(vsid);

    GLShaderStatus status = shader->compile(source);
    if (status != GLShaderStatus::OK)
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

    return GLShaderStatus::OK;
}


GLShaderStatus
GLShaderLoader::CreateFragmentShader(const std::vector<std::string>& source,
                                     GLFragmentShader** fs)
{
    GLuint fsid = glCreateShader(GL_FRAGMENT_SHADER);

    auto* shader = new GLFragmentShader(fsid);

    GLShaderStatus status = shader->compile(source);
    if (status != GLShaderStatus::OK)
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

    return GLShaderStatus::OK;
}


GLShaderStatus
GLShaderLoader::CreateGeometryShader(const std::vector<std::string>& source,
                                     GLGeometryShader** gs)
{
    GLuint vsid = glCreateShader(GL_GEOMETRY_SHADER);

    auto* shader = new GLGeometryShader(vsid);

    GLShaderStatus status = shader->compile(source);
    if (status != GLShaderStatus::OK)
    {
        if (g_shaderLogFile != nullptr)
        {
            *g_shaderLogFile << "Error compiling geometry shader:\n";
            *g_shaderLogFile << GetInfoLog(shader->getID());
        }
        delete shader;
        return status;
    }

    *gs = shader;

    return GLShaderStatus::OK;
}

GLShaderStatus
GLShaderLoader::CreateVertexShader(const std::string& source,
                                   GLVertexShader** vs)

{
    std::vector<std::string> v;
    v.push_back(source);
    return CreateVertexShader(v, vs);
}


GLShaderStatus
GLShaderLoader::CreateFragmentShader(const std::string& source,
                                     GLFragmentShader** fs)
{
    std::vector<std::string> v;
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

    return GLShaderStatus::OK;
}


GLShaderStatus
GLShaderLoader::CreateProgram(const std::vector<std::string>& vsSource,
                              const std::vector<std::string>& fsSource,
                              GLProgram** progOut)
{
    GLVertexShader* vs = nullptr;
    GLShaderStatus status = CreateVertexShader(vsSource, &vs);
    if (status != GLShaderStatus::OK)
        return status;

    GLFragmentShader* fs = nullptr;
    status = CreateFragmentShader(fsSource, &fs);
    if (status != GLShaderStatus::OK)
    {
        delete vs;
        return status;
    }

    GLProgram* prog = nullptr;
    status = CreateProgram(*vs, *fs, &prog);
    if (status != GLShaderStatus::OK)
    {
        delete vs;
        delete fs;
        return status;
    }

    *progOut = prog;

    // No need to keep these around--the program doesn't reference them
    delete vs;
    delete fs;

    return GLShaderStatus::OK;
}


GLShaderStatus
GLShaderLoader::CreateProgram(const GLVertexShader& vs,
                              const GLGeometryShader& gs,
                              const GLFragmentShader& fs,
                              GLProgram** progOut)
{
    GLuint progid = glCreateProgram();

    auto* prog = new GLProgram(progid);

    prog->attach(vs);
    prog->attach(gs);
    prog->attach(fs);

    *progOut = prog;

    return GLShaderStatus::OK;
}


GLShaderStatus
GLShaderLoader::CreateProgram(const std::vector<std::string>& vsSource,
                              const std::vector<std::string>& gsSource,
                              const std::vector<std::string>& fsSource,
                              GLProgram** progOut)
{
    GLVertexShader* vs = nullptr;
    GLShaderStatus status = CreateVertexShader(vsSource, &vs);
    if (status != GLShaderStatus::OK)
        return status;

    GLFragmentShader* fs = nullptr;
    status = CreateFragmentShader(fsSource, &fs);
    if (status != GLShaderStatus::OK)
    {
        delete vs;
        return status;
    }

    GLGeometryShader* gs = nullptr;
    status = CreateGeometryShader(gsSource, &gs);
    if (status != GLShaderStatus::OK)
    {
        delete vs;
        delete fs;
        return status;
    }

    GLProgram* prog = nullptr;
    status = CreateProgram(*vs, *gs, *fs, &prog);
    if (status != GLShaderStatus::OK)
    {
        delete vs;
        delete gs;
        delete fs;
        return status;
    }

    *progOut = prog;

    // No need to keep these around--the program doesn't reference them
    delete vs;
    delete gs;
    delete fs;

    return GLShaderStatus::OK;
}


GLShaderStatus
GLShaderLoader::CreateProgram(const std::string& vsSource,
                              const std::string& fsSource,
                              GLProgram** progOut)
{
    std::vector<std::string> vsSourceVec { vsSource };
    std::vector<std::string> fsSourceVec { fsSource };

    return CreateProgram(vsSourceVec, fsSourceVec, progOut);
}


GLShaderStatus
GLShaderLoader::CreateProgram(const std::string& vsSource,
                              const std::string& gsSource,
                              const std::string& fsSource,
                              GLProgram** progOut)
{
    std::vector<std::string> vsSourceVec { vsSource };
    std::vector<std::string> gsSourceVec { gsSource };
    std::vector<std::string> fsSourceVec { fsSource };

    return CreateProgram(vsSourceVec, gsSourceVec, fsSourceVec, progOut);
}
