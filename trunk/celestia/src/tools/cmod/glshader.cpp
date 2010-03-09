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

using namespace std;


GLShader::~GLShader()
{
    if (m_id != 0)
    {
        glDeleteObjectARB(m_id);
    }
}


bool
GLShader::compile(const string& source)
{
    switch (type())
    {
    case VertexShader:
        m_id = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
        break;
    case FragmentShader:
        m_id = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
        break;
    default:
        break;
    }

    if (m_id == 0)
    {
        return false;
    }

    const char* sourceData = source.c_str();
    const GLint sourceLength = source.length();
    glShaderSourceARB(m_id, 1, &sourceData, &sourceLength);

    glCompileShaderARB(m_id);

    GLint compileStatus;
    glGetObjectParameterivARB(m_id, GL_OBJECT_COMPILE_STATUS_ARB, &compileStatus);

    GLint logLength = 0;
    glGetObjectParameterivARB(m_id, GL_OBJECT_INFO_LOG_LENGTH_ARB, &logLength);
    if (logLength > 0)
    {
        GLsizei charsWritten = 0;
        char* log = new char[logLength];
        glGetInfoLogARB(m_id, logLength, &charsWritten, log);

        m_log = string(log, charsWritten);
        delete[] log;
    }

    return compileStatus != GL_FALSE;
}


GLShaderProgram::GLShaderProgram() :
    m_vertexShader(NULL),
    m_fragmentShader(NULL),
    m_id(0),
    m_linked(false)
{
    m_id = glCreateProgramObjectARB();
}


GLShaderProgram::~GLShaderProgram()
{
    if (m_vertexShader)
    {
        m_vertexShader->unref();
    }

    if (m_fragmentShader)
    {
        m_fragmentShader->unref();
    }

    if (m_id != 0)
    {
        glDeleteObjectARB(m_id);
    }
}


bool
GLShaderProgram::addVertexShader(GLVertexShader* shader)
{
    if (shader == NULL || m_vertexShader != NULL)
    {
        return false;
    }

    if (shader->id() == 0)
    {
        return false;
    }

    m_vertexShader = shader;
    m_vertexShader->ref();
    glAttachObjectARB(m_id, m_vertexShader->id());

    return true;
}


bool
GLShaderProgram::addFragmentShader(GLFragmentShader* shader)
{
    if (shader == NULL || m_fragmentShader != NULL)
    {
        return false;
    }

    if (shader->id() == 0)
    {
        return false;
    }

    m_fragmentShader = shader;
    m_fragmentShader->ref();
    glAttachObjectARB(m_id, m_fragmentShader->id());

    return true;
}


bool
GLShaderProgram::link()
{
    if (!m_vertexShader || !m_fragmentShader)
    {
        return false;
    }

    if (m_linked)
    {
        return false;
    }

    glLinkProgramARB(m_id);

    GLint linkStatus = 0;
    glGetObjectParameterivARB(m_id, GL_OBJECT_LINK_STATUS_ARB, &linkStatus);

    GLint logLength = 0;
    glGetObjectParameterivARB(m_id, GL_OBJECT_INFO_LOG_LENGTH_ARB, &logLength);
    if (logLength > 0)
    {
        GLsizei charsWritten = 0;
        char* log = new char[logLength];
        glGetInfoLogARB(m_id, logLength, &charsWritten, log);

        m_log = string(log, charsWritten);
        delete[] log;
    }

    m_linked = linkStatus != GL_FALSE;
    return m_linked;
}


bool
GLShaderProgram::hasOpenGLShaderPrograms()
{
    return GLEW_ARB_shader_objects && GLEW_ARB_shading_language_100;
}


bool
GLShaderProgram::bind() const
{
    if (m_linked)
    {
        glUseProgramObjectARB(m_id);
        return true;
    }
    else
    {
        return false;
    }
}


void
GLShaderProgram::setUniformValue(const char* name, float value)
{
    GLint location = glGetUniformLocationARB(m_id, name);
    if (location >= 0)
    {
        glUniform1fARB(location, value);
    }
}


void
GLShaderProgram::setSampler(const char* name, int value)
{
    GLint location = glGetUniformLocationARB(m_id, name);
    if (location >= 0)
    {
        glUniform1iARB(location, value);
    }
}


void
GLShaderProgram::setSamplerArray(const char* name, const GLint* values, int count)
{
    GLint location = glGetUniformLocationARB(m_id, name);
    if (location >= 0)
    {
        glUniform1ivARB(location, count, values);
    }
}


void
GLShaderProgram::setUniformValueArray(const char* name, const Eigen::Vector3f* values, int count)
{
    GLint location = glGetUniformLocationARB(m_id, name);
    if (location >= 0)
    {
        glUniform3fvARB(location, count, values[0].data());
    }
}


void
GLShaderProgram::setUniformValueArray(const char* name, const Eigen::Vector4f* values, int count)
{
    GLint location = glGetUniformLocationARB(m_id, name);
    if (location >= 0)
    {
        glUniform4fvARB(location, count, values[0].data());
    }
}


void
GLShaderProgram::setUniformValueArray(const char* name, const Eigen::Matrix4f* values, int count)
{
    GLint location = glGetUniformLocationARB(m_id, name);
    if (location >= 0)
    {
        glUniformMatrix4fvARB(location, count, false, values[0].data());
    }
}


void
GLShaderProgram::bindAttributeLocation(const char* name, int location)
{
    glBindAttribLocationARB(m_id, location, name);
}
