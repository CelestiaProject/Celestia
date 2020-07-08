// glshader.h
//
// Copyright (C) 2001-2006, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_GLSHADER_H_
#define _CELENGINE_GLSHADER_H_

#include <Eigen/Core>
#include <string>
#include <vector>
#include <iostream>
#include "glsupport.h"

class GLShaderLoader;

enum GLShaderStatus
{
    ShaderStatus_OK,
    ShaderStatus_CompileError,
    ShaderStatus_LinkError,
    ShaderStatus_OutOfMemory,
    ShaderStatus_EmptyProgram,
};

class GLShader
{
 protected:
    GLShader(GLuint _id);
    virtual ~GLShader();

 public:
    GLuint getID() const;

 private:
    GLuint id;

    GLShaderStatus compile(const std::vector<std::string>& source);

    friend class GLShaderLoader;
};


class GLVertexShader : public GLShader
{
 private:
    GLVertexShader(GLuint _id) : GLShader(_id) {};

 friend class GLShaderLoader;
};


class GLFragmentShader : public GLShader
{
 private:
    GLFragmentShader(GLuint _id) : GLShader(_id) {};

 friend class GLShaderLoader;
};


class GLProgram
{
 private:
    GLProgram(GLuint _id);

    void attach(const GLShader&);

 public:
    virtual ~GLProgram();

    GLShaderStatus link();

    void use() const;
    GLuint getID() const { return id; }

 private:
    GLuint id;

 friend class GLShaderLoader;
};

class ShaderParameter
{
 public:
    ShaderParameter() = default;
    ~ShaderParameter() = default;
    ShaderParameter(const ShaderParameter&) = default;
    ShaderParameter(ShaderParameter&&) = default;
    ShaderParameter& operator=(const ShaderParameter&) = default;
    ShaderParameter& operator=(ShaderParameter&&) = default;

    ShaderParameter(GLuint obj, const char* name)
    {
        slot = glGetUniformLocation(obj, name);
    }

    bool valid() const noexcept
    {
        return slot != -1;
    };

    operator bool() const noexcept
    {
        return valid();
    }
 protected:
    int slot { -1 };
};

class FloatShaderParameter : public ShaderParameter
{
 public:
    using ShaderParameter::ShaderParameter;
    FloatShaderParameter& operator=(float);
};


class Vec3ShaderParameter : public ShaderParameter
{
 public:
    using ShaderParameter::ShaderParameter;
    Vec3ShaderParameter& operator=(const Eigen::Vector3f&);
};


class Vec4ShaderParameter : public ShaderParameter
{
 public:
    using ShaderParameter::ShaderParameter;
    Vec4ShaderParameter& operator=(const Eigen::Vector4f&);
};


class IntegerShaderParameter : public ShaderParameter
{
 public:
    using ShaderParameter::ShaderParameter;
    IntegerShaderParameter& operator=(int);
};


class Mat3ShaderParameter : public ShaderParameter
{
 public:
    using ShaderParameter::ShaderParameter;
    Mat3ShaderParameter& operator=(const Eigen::Matrix3f&);
};


class Mat4ShaderParameter : public ShaderParameter
{
 public:
    using ShaderParameter::ShaderParameter;
    Mat4ShaderParameter& operator=(const Eigen::Matrix4f&);
};


class GLShaderLoader
{
 public:
    static GLShaderStatus CreateVertexShader(const std::vector<std::string>&,
                                             GLVertexShader**);
    static GLShaderStatus CreateFragmentShader(const std::vector<std::string>&,
                                               GLFragmentShader**);
    static GLShaderStatus CreateVertexShader(const std::string&,
                                             GLVertexShader**);
    static GLShaderStatus CreateFragmentShader(const std::string&,
                                               GLFragmentShader**);

    static GLShaderStatus CreateProgram(const GLVertexShader& vs,
                                        const GLFragmentShader& fs,
                                        GLProgram**);
    static GLShaderStatus CreateProgram(const std::vector<std::string>& vs,
                                        const std::vector<std::string>& fs,
                                        GLProgram**);
    static GLShaderStatus CreateProgram(const std::string& vsSource,
                                        const std::string& fsSource,
                                        GLProgram**);
};


extern std::ostream* g_shaderLogFile;

#endif // _CELENGINE_GLSHADER_H_
