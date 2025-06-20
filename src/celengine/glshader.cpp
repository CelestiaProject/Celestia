// glshader.cpp
//
// Copyright (C) 2001-2006, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "glshader.h"

#include <cassert>
#include <limits>
#include <string>
#include <utility>

#include <fmt/ostream.h>

#include <celutil/logger.h>

using namespace std::string_view_literals;

using celestia::util::GetLogger;

namespace
{
constexpr std::string_view
shaderType(GLenum shaderType)
{
    switch (shaderType)
    {
    case GL_VERTEX_SHADER:
        return "vertex"sv;
    case GL_GEOMETRY_SHADER:
        return "geometry"sv;
    case GL_FRAGMENT_SHADER:
        return "fragment"sv;
    default:
        assert(0);
        return std::string_view{};
    }
}

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

template<typename... Args>
void
writeShaderLog(std::string_view format, Args&&... args)
{
    if (g_shaderLogFile == nullptr)
        return;

    fmt::print(*g_shaderLogFile, format, std::forward<Args>(args)...);
}

GLShaderStatus
compile(GLuint id, std::string_view source)
{
    if (source.empty())
        return GLShaderStatus::EmptyProgram;

    if (source.size() > std::numeric_limits<GLint>::max())
        return GLShaderStatus::OutOfMemory;

    const char* ptr = source.data();
    GLint length = static_cast<GLint>(source.size());

    glShaderSource(id, 1, &ptr, &length);

    // Actually compile the shader
    glCompileShader(id);

    GLint compileSuccess;
    glGetShaderiv(id, GL_COMPILE_STATUS, &compileSuccess);
    if (compileSuccess != GL_TRUE)
        return GLShaderStatus::CompileError;

    return GLShaderStatus::OK;
}

template<typename Shader, typename Key>
Shader
createShader(Key key, std::string_view source, GLShaderStatus& status)
{
    GLuint id = glCreateShader(Shader::ShaderType);
    if (id == 0)
    {
        status = GLShaderStatus::OutOfMemory;
        writeShaderLog("Could not obtain {} shader id\n", shaderType(Shader::ShaderType));
        return Shader();
    }

    Shader shader{ key, id };
    status = compile(id, source);
    if (status != GLShaderStatus::OK)
    {
        writeShaderLog("Error compiling {} shader:\n{}", shaderType(Shader::ShaderType), GetInfoLog(id));
        return Shader();
    }

    return shader;
}

} // end unnamed namespace

std::ostream* g_shaderLogFile = nullptr;

GLShader::GLShader(GLuint _id) :
    id(_id)
{
}

GLShader::GLShader(GLShader&& other) noexcept :
    id(other.id)
{
    other.id = 0;
}

GLShader&
GLShader::operator=(GLShader&& other) noexcept
{
    if (this != &other)
    {
        destroy();
        id = other.id;
        other.id = 0;
    }

    return *this;
}

void
GLShader::destroy() const
{
    if (id != 0)
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

GLProgram::GLProgram(GLProgram&& other) noexcept :
    id(other.id)
{
    other.id = 0;
}

GLProgram&
GLProgram::operator=(GLProgram&& other) noexcept
{
    if (this != &other)
    {
        destroy();
        id = other.id;
        other.id = 0;
    }

    return *this;
}

void
GLProgram::destroy() const
{
    if (id != 0)
        glDeleteProgram(id);
}

void
GLProgram::use() const
{
    glUseProgram(id);
}

GLVertexShader
GLVertexShader::create(std::string_view source, GLShaderStatus& status)
{
    return createShader<GLVertexShader>(CreateToken{}, source, status);
}

GLGeometryShader
GLGeometryShader::create(std::string_view source, GLShaderStatus& status)
{
    return createShader<GLGeometryShader>(CreateToken{}, source, status);
}

GLFragmentShader
GLFragmentShader::create(std::string_view source, GLShaderStatus& status)
{
    return createShader<GLFragmentShader>(CreateToken{}, source, status);
}

GLProgramBuilder::GLProgramBuilder(GLProgramBuilder&& other) noexcept :
    id(other.id)
{
    other.id = 0;
}

GLProgramBuilder&
GLProgramBuilder::operator=(GLProgramBuilder&& other) noexcept
{
    if (this != &other)
    {
        destroy();
        id = other.id;
        other.id = 0;
    }

    return *this;
}

void
GLProgramBuilder::destroy() const
{
    if (id != 0)
        glDeleteProgram(id);
}

GLProgramBuilder
GLProgramBuilder::create(GLShaderStatus& status)
{
    GLuint progId = glCreateProgram();
    if (progId == 0)
    {
        status = GLShaderStatus::OutOfMemory;
        return GLProgramBuilder{};
    }

    status = GLShaderStatus::OK;
    return GLProgramBuilder{ progId };
}

void
GLProgramBuilder::attach(GLVertexShader&& vs)
{
    vertexShader = std::move(vs);
}

void
GLProgramBuilder::attach(GLGeometryShader&& gs)
{
    geometryShader = std::move(gs);
}

void
GLProgramBuilder::attach(GLFragmentShader&& fs)
{
    fragmentShader = std::move(fs);
}

void
GLProgramBuilder::bindAttribute(GLuint index, const GLchar* name) const
{
    if (isValid())
        glBindAttribLocation(id, index, name);
}

GLProgram
GLProgramBuilder::link(GLShaderStatus& status)
{
    if (!isValid())
    {
        status = GLShaderStatus::OutOfMemory;
        return GLProgram{};
    }

    if (vertexShader.isValid())
        glAttachShader(id, vertexShader.getID());
    if (geometryShader.isValid())
        glAttachShader(id, geometryShader.getID());
    if (fragmentShader.isValid())
        glAttachShader(id, fragmentShader.getID());

    glLinkProgram(id);

    GLint linkSuccess;
    glGetProgramiv(id, GL_LINK_STATUS, &linkSuccess);
    if (linkSuccess != GL_TRUE)
    {
        writeShaderLog("Error linking shader program:\n{}", GetInfoLog(id));
        status = GLShaderStatus::LinkError;
    }

    status = GLShaderStatus::OK;
    GLProgram program{ id };
    id = 0;
    return program;
}
