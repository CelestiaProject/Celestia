// glshader.h
//
// Copyright (C) 2004-2010, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// C++ wrapper for OpenGL shaders and shader programs

#pragma once

#include <string>

#include <Eigen/Core>

#include "glsupport.h"

class GLVertexShader;
class GLFragmentShader;


class GLShader
{
public:
    enum ShaderType
    {
        VertexShader,
        FragmentShader,
        GeometryShader
    };

    virtual ~GLShader();

    int ref()
    {
        return ++m_refCount;
    }

    void unref()
    {
        --m_refCount;
        if (m_refCount <= 0)
        {
            delete this;
        }
    }

    ShaderType type() const
    {
        return m_type;
    }

    GLuint id() const
    {
        return m_id;
    }

    bool compile(const std::string& source);

    std::string log() const
    {
        return m_log;
    }

private:
    explicit GLShader(ShaderType type) : m_type(type) {}

    ShaderType m_type;
    GLuint m_id{ 0 };
    std::string m_log{};
    int m_refCount{ 1 };

    friend class GLVertexShader;
    friend class GLFragmentShader;
};


class GLVertexShader : public GLShader
{
public:
    GLVertexShader() :
        GLShader(VertexShader)
    {
    }
};


class GLFragmentShader : public GLShader
{
public:
    GLFragmentShader() :
        GLShader(FragmentShader)
    {
    }
};


class GLShaderProgram
{
public:
    GLShaderProgram() = default;
    ~GLShaderProgram();

    bool addVertexShader(GLVertexShader* shader);
    bool addFragmentShader(GLFragmentShader* shader);
    bool link();

    GLuint id() const
    {
        return m_id;
    }

    bool bind() const;
    bool isLinked() const
    {
        return m_linked;
    }

    std::string log() const
    {
        return m_log;
    }

    static bool hasOpenGLShaderPrograms();

    void setUniformValue(const char* name, float value);
    void setSampler(const char* name, int value);
    void setSamplerArray(const char* name, const GLint* values, int count);
    template<typename DERIVED> void setUniformValue(const char* name, const Eigen::MatrixBase<DERIVED>& value);
    void setUniformValueArray(const char* name, const Eigen::Vector3f* values, int count);
    void setUniformValueArray(const char* name, const Eigen::Vector4f* values, int count);
    void setUniformValueArray(const char* name, const Eigen::Matrix4f* values, int count);
    void bindAttributeLocation(const char* name, int location);

private:
    GLVertexShader* m_vertexShader{ nullptr };
    GLFragmentShader* m_fragmentShader{ nullptr };
    GLuint m_id{ glCreateProgram() };
    std::string m_log{};
    bool m_linked{ false };
};


template<typename SCALAR, int ROWS, int COLS> inline void
glsh_setUniformValue(GLuint /*id*/, const char* /*name*/, const SCALAR* /*data*/)
{
}

// Template specializations to handle various Eigen types
// TODO: add support for more matrix and vector sizes
template<> inline void
glsh_setUniformValue<float, 3, 1>(GLuint id, const char *name, const float* data)
{
    GLint location = glGetUniformLocation(id, name);
    if (location >= 0)
    {
        glUniform3fv(location, 1, data);
    }
}

template<> inline void
glsh_setUniformValue<float, 4, 1>(GLuint id, const char *name, const float* data)
{
    GLint location = glGetUniformLocation(id, name);
    if (location >= 0)
    {
        glUniform4fv(location, 1, data);
    }
}

template<> inline void
glsh_setUniformValue<float, 4, 4>(GLuint id, const char *name, const float* data)
{
    GLint location = glGetUniformLocation(id, name);
    if (location >= 0)
    {
        glUniformMatrix4fv(location, 1, GL_FALSE, data);
    }
}


template<typename DERIVED> void
GLShaderProgram::setUniformValue(const char* name, const Eigen::MatrixBase<DERIVED>& value)
{
    glsh_setUniformValue<typename Eigen::internal::traits<DERIVED>::Scalar, Eigen::MatrixBase<DERIVED>::RowsAtCompileTime, Eigen::MatrixBase<DERIVED>::ColsAtCompileTime>(m_id, name, value.eval().data());
}
