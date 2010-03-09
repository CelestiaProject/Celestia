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

#include <GL/glew.h>
#include <Eigen/Core>
#include <string>


class GLVertexShader;
class GLFragmentShader;


class GLShader
{
public:
    friend class GLVertexShader;
    friend class GLFragmentShader;

    enum ShaderType
    {
        VertexShader,
        FragmentShader,
        GeometryShader
    };

private:
    GLShader(ShaderType type) :
        m_type(type),
        m_id(0),
        m_refCount(1)
    {
    }

public:
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

    GLhandleARB id() const
    {
        return m_id;
    }

    bool compile(const std::string& source);

    std::string log() const
    {
        return m_log;
    }

private:
    ShaderType m_type;
    GLhandleARB m_id;
    std::string m_log;
    int m_refCount;
};


class GLVertexShader : public GLShader
{
public:
    GLVertexShader() :
        GLShader(VertexShader)
    {
    }

    virtual ~GLVertexShader()
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

    virtual ~GLFragmentShader()
    {
    }
};


class GLShaderProgram
{
public:
    GLShaderProgram();
    ~GLShaderProgram();

    bool addVertexShader(GLVertexShader* shader);
    bool addFragmentShader(GLFragmentShader* shader);
    bool link();

    GLhandleARB id() const
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
    GLVertexShader* m_vertexShader;
    GLFragmentShader* m_fragmentShader;
    GLhandleARB m_id;
    std::string m_log;
    bool m_linked;
};


template<typename SCALAR, int ROWS, int COLS> inline void
glsh_setUniformValue(GLhandleARB id, const char* name, const SCALAR* data)
{
}

// Template specializations to handle various Eigen types
// TODO: add support for more matrix and vector sizes
template<> inline void
glsh_setUniformValue<float, 3, 1>(GLhandleARB id, const char *name, const float* data)
{
    GLint location = glGetUniformLocationARB(id, name);
    if (location >= 0)
    {
        glUniform3fvARB(location, 1, data);
    }
}

template<> inline void
glsh_setUniformValue<float, 4, 1>(GLhandleARB id, const char *name, const float* data)
{
    GLint location = glGetUniformLocationARB(id, name);
    if (location >= 0)
    {
        glUniform4fvARB(location, 1, data);
    }
}

template<> inline void
glsh_setUniformValue<float, 4, 4>(GLhandleARB id, const char *name, const float* data)
{
    GLint location = glGetUniformLocationARB(id, name);
    if (location >= 0)
    {
        glUniformMatrix4fvARB(location, 1, GL_FALSE, data);
    }
}


template<typename DERIVED> void
GLShaderProgram::setUniformValue(const char* name, const Eigen::MatrixBase<DERIVED>& value)
{
    glsh_setUniformValue<typename Eigen::ei_traits<DERIVED>::Scalar, Eigen::MatrixBase<DERIVED>::RowsAtCompileTime, Eigen::MatrixBase<DERIVED>::ColsAtCompileTime>(m_id, name, value.eval().data());
}
