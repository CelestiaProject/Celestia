// glshader.cpp
//
// Copyright (C) 2004-2010, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// C++ wrapper for OpenGL shaders and shader programs

#include "glshader.h"

#include <cstddef>


GLShader::~GLShader()
{
    if (m_id != 0)
        glDeleteShader(m_id);
}


bool
GLShader::compile(const std::string& source)
{
    switch (type())
    {
    case VertexShader:
        m_id = glCreateShader(GL_VERTEX_SHADER);
        break;
    case FragmentShader:
        m_id = glCreateShader(GL_FRAGMENT_SHADER);
        break;
    default:
        break;
    }

    if (m_id == 0)
        return false;

    const char* sourceData = source.c_str();
    const GLint sourceLength = source.length();
    glShaderSource(m_id, 1, &sourceData, &sourceLength);

    glCompileShader(m_id);

    GLint compileStatus;
    glGetShaderiv(m_id, GL_COMPILE_STATUS, &compileStatus);

    GLint logLength = 0;
    glGetShaderiv(m_id, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        m_log = std::string('\0', static_cast<std::size_t>(logLength));
        GLsizei charsWritten = 0;
        glGetShaderInfoLog(m_id, logLength, &charsWritten, m_log.data());
        m_log.resize(charsWritten);
    }

    return compileStatus != GL_FALSE;
}


GLShaderProgram::~GLShaderProgram()
{
    if (m_vertexShader)
        m_vertexShader->unref();

    if (m_fragmentShader)
        m_fragmentShader->unref();

    if (m_id != 0)
        glDeleteProgram(m_id);
}


bool
GLShaderProgram::addVertexShader(GLVertexShader* shader)
{
    if (shader == nullptr || m_vertexShader != nullptr || shader->id() == 0)
        return false;

    m_vertexShader = shader;
    m_vertexShader->ref();
    glAttachShader(m_id, m_vertexShader->id());

    return true;
}


bool
GLShaderProgram::addFragmentShader(GLFragmentShader* shader)
{
    if (shader == nullptr || m_fragmentShader != nullptr || shader->id() == 0)
        return false;

    m_fragmentShader = shader;
    m_fragmentShader->ref();
    glAttachShader(m_id, m_fragmentShader->id());

    return true;
}


bool
GLShaderProgram::link()
{
    if (!m_vertexShader || !m_fragmentShader || m_linked)
        return false;

    glLinkProgram(m_id);

    GLint linkStatus = 0;
    glGetProgramiv(m_id, GL_LINK_STATUS, &linkStatus);

    GLint logLength = 0;
    glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        m_log = std::string('\0', static_cast<std::size_t>(logLength));
        GLsizei charsWritten = 0;
        glGetProgramInfoLog(m_id, logLength, &charsWritten, m_log.data());
        m_log.resize(charsWritten);
    }

    m_linked = linkStatus != GL_FALSE;
    return m_linked;
}


bool
GLShaderProgram::hasOpenGLShaderPrograms()
{
    return celestia::gl::ARB_shader_objects && celestia::gl::ARB_shading_language_100;
}


bool
GLShaderProgram::bind() const
{
    if (!m_linked)
        return false;

    glUseProgram(m_id);
    return true;
}


void
GLShaderProgram::setUniformValue(const char* name, float value)
{
    GLint location = glGetUniformLocation(m_id, name);
    if (location >= 0)
        glUniform1f(location, value);
}


void
GLShaderProgram::setSampler(const char* name, int value)
{
    GLint location = glGetUniformLocation(m_id, name);
    if (location >= 0)
        glUniform1i(location, value);
}


void
GLShaderProgram::setSamplerArray(const char* name, const GLint* values, int count)
{
    GLint location = glGetUniformLocation(m_id, name);
    if (location >= 0)
        glUniform1iv(location, count, values);
}


void
GLShaderProgram::setUniformValueArray(const char* name, const Eigen::Vector3f* values, int count)
{
    GLint location = glGetUniformLocation(m_id, name);
    if (location >= 0)
        glUniform3fv(location, count, values[0].data());
}


void
GLShaderProgram::setUniformValueArray(const char* name, const Eigen::Vector4f* values, int count)
{
    GLint location = glGetUniformLocation(m_id, name);
    if (location >= 0)
        glUniform4fv(location, count, values[0].data());
}


void
GLShaderProgram::setUniformValueArray(const char* name, const Eigen::Matrix4f* values, int count)
{
    GLint location = glGetUniformLocation(m_id, name);
    if (location >= 0)
        glUniformMatrix4fv(location, count, false, values[0].data());
}


void
GLShaderProgram::bindAttributeLocation(const char* name, int location)
{
    glBindAttribLocation(m_id, location, name);
}
