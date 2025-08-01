// glshader.h
//
// Copyright (C) 2001-2006, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <iosfwd>
#include <string_view>

#include <Eigen/Core>

#include "glsupport.h"

enum class GLShaderStatus
{
    OK,
    CompileError,
    LinkError,
    OutOfMemory,
    EmptyProgram,
};

class GLShader
{
public:
    GLShader(const GLShader&) = delete;
    GLShader& operator=(const GLShader&) = delete;

    GLShader(GLShader&&) noexcept;
    GLShader& operator=(GLShader&&) noexcept;

    GLuint getID() const noexcept { return id; }
    bool isValid() const noexcept { return id != 0; }

protected:
    struct CreateToken {};

    explicit GLShader() = default;
    explicit GLShader(GLuint _id);
    ~GLShader() { destroy(); }

private:
    void destroy() const;

    GLuint id{ 0 };
};

class GLVertexShader : private GLShader
{
public:
    static constexpr GLenum ShaderType = GL_VERTEX_SHADER;

    GLVertexShader() = default;
    GLVertexShader(CreateToken, GLuint _id) : GLShader(_id) {}

    static GLVertexShader create(std::string_view source, GLShaderStatus& status);

    using GLShader::getID;
    using GLShader::isValid;
};

class GLGeometryShader : private GLShader
{
public:
    static constexpr GLenum ShaderType = GL_GEOMETRY_SHADER;

    GLGeometryShader() = default;
    GLGeometryShader(CreateToken, GLuint _id) : GLShader(_id) {}

    static GLGeometryShader create(std::string_view source, GLShaderStatus& status);

    using GLShader::getID;
    using GLShader::isValid;
};

class GLFragmentShader : private GLShader
{
public:
    static constexpr GLenum ShaderType = GL_FRAGMENT_SHADER;

    GLFragmentShader() = default;
    GLFragmentShader(CreateToken, GLuint _id) : GLShader(_id) {}

    static GLFragmentShader create(std::string_view source, GLShaderStatus& status);

    using GLShader::getID;
    using GLShader::isValid;
};

class GLProgramBuilder;

class GLProgram
{
public:
    GLProgram() = default;
    ~GLProgram() { destroy(); }

    GLProgram(const GLProgram&) = delete;
    GLProgram& operator=(const GLProgram&) = delete;

    GLProgram(GLProgram&&) noexcept;
    GLProgram& operator=(GLProgram&&) noexcept;

    GLuint getID() const noexcept { return id; }
    bool isValid() const noexcept { return id != 0; }

    void use() const;

private:
    explicit GLProgram(GLuint _id);
    void destroy() const;

    GLuint id{ 0 };

    friend class GLProgramBuilder;
};

class GLProgramBuilder
{
public:
    GLProgramBuilder() = default;
    ~GLProgramBuilder() { destroy(); }

    GLProgramBuilder(const GLProgramBuilder&) = delete;
    GLProgramBuilder& operator=(const GLProgramBuilder&) = delete;

    GLProgramBuilder(GLProgramBuilder&&) noexcept;
    GLProgramBuilder& operator=(GLProgramBuilder&&) noexcept;

    static GLProgramBuilder create(GLShaderStatus& status);

    GLuint getID() const noexcept { return id; }
    bool isValid() const noexcept { return id != 0; }

    void attach(GLFragmentShader&&);
    void attach(GLGeometryShader&&);
    void attach(GLVertexShader&&);

    void bindAttribute(GLuint index, const GLchar* name) const;

    GLProgram link(GLShaderStatus&);

private:
    explicit GLProgramBuilder(GLuint _id) : id(_id) {}
    void destroy() const;

    GLuint id{ 0 };
    GLFragmentShader fragmentShader;
    GLGeometryShader geometryShader;
    GLVertexShader vertexShader;
};

class FloatShaderParameter
{
public:
    FloatShaderParameter() = default;
    FloatShaderParameter(GLuint obj, const char* name);

    FloatShaderParameter& operator=(float);

private:
    int slot{ -1 };
};

class Vec2ShaderParameter
{
public:
    Vec2ShaderParameter() = default;
    Vec2ShaderParameter(GLuint obj, const char* name);

    Vec2ShaderParameter& operator=(const Eigen::Vector2f&);

private:
    int slot{ -1 };
};

class Vec3ShaderParameter
{
public:
    Vec3ShaderParameter() = default;
    Vec3ShaderParameter(GLuint obj, const char* name);

    Vec3ShaderParameter& operator=(const Eigen::Vector3f&);

private:
    int slot{ -1 };
};

class Vec4ShaderParameter
{
public:
    Vec4ShaderParameter() = default;
    Vec4ShaderParameter(GLuint obj, const char* name);

    Vec4ShaderParameter& operator=(const Eigen::Vector4f&);

private:
    int slot{ -1 };
};

class IntegerShaderParameter
{
public:
    IntegerShaderParameter() = default;
    IntegerShaderParameter(GLuint obj, const char* name);

    IntegerShaderParameter& operator=(int);

private:
    int slot{ -1 };
};

class Mat3ShaderParameter
{
public:
    Mat3ShaderParameter() = default;
    Mat3ShaderParameter(GLuint obj, const char* name);

    Mat3ShaderParameter& operator=(const Eigen::Matrix3f&);

private:
    int slot{ -1 };
};

class Mat4ShaderParameter
{
public:
    Mat4ShaderParameter() = default;
    Mat4ShaderParameter(GLuint obj, const char* name);

    Mat4ShaderParameter& operator=(const Eigen::Matrix4f&);

private:
    int slot{ -1 };
};

extern std::ostream* g_shaderLogFile;
