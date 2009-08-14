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
#include <GL/glew.h>

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
    GLShader(GLhandleARB _id);
    virtual ~GLShader();

 public:
    GLhandleARB getID() const;

 private:
    GLhandleARB id;

    GLShaderStatus compile(const std::vector<std::string>& source);

    friend class GLShaderLoader;
};


class GLVertexShader : public GLShader
{
 private:
    GLVertexShader(GLhandleARB _id) : GLShader(_id) {};

 friend class GLShaderLoader;
};


class GLFragmentShader : public GLShader
{
 private:
    GLFragmentShader(GLhandleARB _id) : GLShader(_id) {};

 friend class GLShaderLoader;
};


class GLProgram
{
 private:
    GLProgram(GLhandleARB _id);

    void attach(const GLShader&);

 public:
    virtual ~GLProgram();

    GLShaderStatus link();

    void use() const;
    GLhandleARB getID() const { return id; }

 private:
    GLhandleARB id;

 friend class GLShaderLoader;
};


class FloatShaderParameter
{
 public:
    FloatShaderParameter();
    FloatShaderParameter(GLhandleARB _obj, const char* name);

    FloatShaderParameter& operator=(float);
    
 private:
    int slot;
};


class Vec3ShaderParameter
{
 public:
    Vec3ShaderParameter();
    Vec3ShaderParameter(GLhandleARB _obj, const char* name);

    Vec3ShaderParameter& operator=(const Eigen::Vector3f&);

 private:
    int slot;
};


class Vec4ShaderParameter
{
 public:
    Vec4ShaderParameter();
    Vec4ShaderParameter(GLhandleARB _obj, const char* name);

    Vec4ShaderParameter& operator=(const Eigen::Vector4f&);

 private:
    int slot;
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
    static GLShaderStatus CreateProgram(const std::string& vs,
                                        const std::string& fs,
                                        GLProgram**);
};


extern std::ostream* g_shaderLogFile;

#endif // _CELENGINE_GLSHADER_H_
