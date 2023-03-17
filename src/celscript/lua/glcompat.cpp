// glcompat.h
//
// Copyright (C) 2020-present, the Celestia Development Team
//
// OpenGL 2.1 compatibility layer for OpenGL ES 2.0
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <array>
#include <cassert>
#include <cstring>                      // memcpy
#include <memory>
#include <Eigen/Core>
#include <fmt/format.h>
#include <celengine/glsupport.h>
#include <celengine/shadermanager.h>    // CelestiaGLProgram::*Index
#include <celmath/vecgl.h>              // celmath::translate
#include "glcompat.h"

namespace
{
constexpr int MODELVIEW_STACK_DEPTH = 8;
constexpr int PROJECTION_STACK_DEPTH = 4;
std::array<Eigen::Matrix4f, MODELVIEW_STACK_DEPTH> g_modelViewStack =
{
    Eigen::Matrix4f::Identity(),
    Eigen::Matrix4f::Identity(),
    Eigen::Matrix4f::Identity(),
    Eigen::Matrix4f::Identity(),
    Eigen::Matrix4f::Identity(),
    Eigen::Matrix4f::Identity(),
    Eigen::Matrix4f::Identity(),
    Eigen::Matrix4f::Identity()

};
std::array<Eigen::Matrix4f, PROJECTION_STACK_DEPTH> g_projectionStack =
{
    Eigen::Matrix4f::Identity(),
    Eigen::Matrix4f::Identity(),
    Eigen::Matrix4f::Identity(),
    Eigen::Matrix4f::Identity()
};

int g_modelViewPosition = 0;
int g_projectionPosition = 0;
int g_matrixMode = GL_MODELVIEW;

class GLSLProgram
{
public:
    explicit GLSLProgram(GLProgram *prog) : m_prog(prog)
    {
        m_MVPMatrix = Mat4ShaderParameter(m_prog->getID(), "MVPMatrix");
        IntegerShaderParameter(m_prog->getID(), "u_tex") = 0;
    }
    ~GLSLProgram() = default;
    GLSLProgram(const GLSLProgram&) = delete;
    GLSLProgram& operator=(const GLSLProgram&) = delete;
    GLSLProgram(GLSLProgram&&) = default;
    GLSLProgram& operator=(GLSLProgram&&) = default;

    void use()
    {
        m_prog->use();
    }
    void setMVPMatrix(const Eigen::Matrix4f &m)
    {
        m_MVPMatrix = m;
    }

private:
    std::unique_ptr<GLProgram> m_prog;
    Mat4ShaderParameter m_MVPMatrix;
};

#ifdef GL_ES
const char glsl_version[] = "100";
#else
const char glsl_version[] = "120";
#endif

const char kVertexShader[] = R"glsl(
#version {}
#define SHADER_COLOR {}
#define SHADER_TEXCOORD {}

#ifdef GL_ES
precision highp float;
#endif

attribute vec2 in_Position;
attribute vec2 in_TexCoord0;
#if SHADER_COLOR
attribute vec4 in_Color;
varying vec4 v_color;
#endif
#if SHADER_TEXCOORD
varying vec2 v_texCoord;
#endif

//uniform mat4 ModelViewMatrix;
//uniform mat4 ProjectionMatrix;
uniform mat4 MVPMatrix;

invariant gl_Position;

void main(void)
{{
#if SHADER_COLOR
    v_color = in_Color;
#endif
#if SHADER_TEXCOORD
    v_texCoord = in_TexCoord0;
#endif
    gl_Position = MVPMatrix * vec4(in_Position, 0.0, 1.0);
}}
)glsl";

const char kFragmentShader[] = R"glsl(
#version {}
#define SHADER_COLOR {}
#define SHADER_TEXCOORD {}

#ifdef GL_ES
precision highp float;
#endif

#if SHADER_COLOR
varying vec4 v_color;
#endif
#if SHADER_TEXCOORD
varying vec2 v_texCoord;
uniform sampler2D u_tex;
#endif

void main(void)
{{
#if SHADER_TEXCOORD
    gl_FragColor = texture2D(u_tex, v_texCoord);
#endif
#if SHADER_COLOR
    gl_FragColor = v_color;
#endif
}}
)glsl";

enum ShaderAttributes
{
    SHADER_COLOR    = 0,
    SHADER_TEXCOORD = 1,
    SHADER_COUNT    = 2
};

GLenum gPrimitive = GL_NONE;
int gVertexCounter = 0;
int gTexCoordCounter = 0;
int gColorCounter = 0;

struct Vertex
{
    float x, y, u, v, r, g, b, a;
};

std::array<Vertex, 16> vertices {};

GLProgram* BuildProgram(const std::string &vertex, const std::string &fragment)
{
    GLProgram* prog = nullptr;
    auto status = GLShaderLoader::CreateProgram(vertex, fragment, &prog);
    if (status != ShaderStatus_OK)
    {
        delete prog;
        return nullptr;
    }
    glBindAttribLocation(prog->getID(), CelestiaGLProgram::VertexCoordAttributeIndex, "in_Position");
    glBindAttribLocation(prog->getID(), CelestiaGLProgram::TextureCoord0AttributeIndex, "in_TexCoord0");
    glBindAttribLocation(prog->getID(), CelestiaGLProgram::ColorAttributeIndex, "in_Color");
    status = prog->link();
    if (status != ShaderStatus_OK)
    {
        delete prog;
        return nullptr;
    }
    return prog;
}

GLSLProgram* FindGLProgram(ShaderAttributes attr)
{
    static std::array<GLSLProgram*, SHADER_COUNT> programs;
    auto *prog = programs[attr];
    if (prog == nullptr)
    {
        int color = attr == SHADER_COLOR ? 1 : 0;
        int texture = attr == SHADER_TEXCOORD ? 1 : 0;
        std::string vertex = fmt::format(kVertexShader, glsl_version, color, texture);
        std::string fragment = fmt::format(kFragmentShader, glsl_version, color, texture);
        auto *glprog = BuildProgram(vertex, fragment);
        if (glprog != nullptr)
            programs[attr] = new GLSLProgram(glprog);
    }
    return prog;
}


void Draw()
{
    if (gPrimitive == GL_NONE || gVertexCounter == 0)
        return; // Do nothing

    bool hasTexCoords       = gTexCoordCounter == gVertexCounter;
    bool hasColors          = gColorCounter == gVertexCounter;
    ShaderAttributes type   = hasTexCoords ? SHADER_TEXCOORD : SHADER_COLOR;

    glEnableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glVertexAttribPointer(
        CelestiaGLProgram::VertexCoordAttributeIndex,
        2, GL_FLOAT, false, sizeof(Vertex), &vertices[0].x
    );

    if (hasTexCoords)
    {
        glEnableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex);
        glVertexAttribPointer(
            CelestiaGLProgram::TextureCoord0AttributeIndex,
            2, GL_FLOAT, false, sizeof(Vertex), &vertices[0].u
        );
    }

    if (hasColors)
    {
        glEnableVertexAttribArray(CelestiaGLProgram::ColorAttributeIndex);
        glVertexAttribPointer(
            CelestiaGLProgram::ColorAttributeIndex,
            4, GL_FLOAT, false, sizeof(Vertex), &vertices[0].r
        );
    }

    if (auto *prog = FindGLProgram(type); prog != nullptr)
    {
        prog->use();
        prog->setMVPMatrix(g_projectionStack[g_projectionPosition] * g_modelViewStack[g_modelViewPosition]);
        glDrawArrays(gPrimitive, 0, gVertexCounter);
    }

    glDisableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    if (hasTexCoords)
        glDisableVertexAttribArray(CelestiaGLProgram::TextureCoord3AttributeIndex);
    if (hasColors)
        glDisableVertexAttribArray(CelestiaGLProgram::ColorAttributeIndex);

    // reset state
    gVertexCounter = 0;
    gTexCoordCounter = 0;
    gColorCounter = 0;
    gPrimitive = GL_NONE;
}

void PushVertex(float x, float y)
{
    if (gVertexCounter < vertices.size())
    {
        vertices[gVertexCounter].x = x;
        vertices[gVertexCounter].y = y;
        gVertexCounter++;
    }
}

void PushTexCoord(float u, float v)
{
    if (gTexCoordCounter < vertices.size())
    {
        vertices[gTexCoordCounter].u = u;
        vertices[gTexCoordCounter].v = v;
        gTexCoordCounter++;
    }
}

void PushColor(float r, float g, float b, float a)
{
    if (gColorCounter < vertices.size())
    {
        vertices[gColorCounter].r = r;
        vertices[gColorCounter].g = g;
        vertices[gColorCounter].b = b;
        vertices[gColorCounter].a = a;
        gColorCounter++;
    }
}
} // namespace

void
fpcMatrixMode(int _g_matrixMode) noexcept
{
    g_matrixMode = _g_matrixMode;
}

void
fpcPushMatrix() noexcept
{
    switch (g_matrixMode)
    {
    case GL_MODELVIEW:
        if (g_modelViewPosition < MODELVIEW_STACK_DEPTH - 1)
            g_modelViewPosition++;
        else
            assert(0 && "Matrix stack overflow");
        break;

    case GL_PROJECTION:
        if (g_projectionPosition < PROJECTION_STACK_DEPTH - 1)
            g_projectionPosition++;
        else
            assert(0 && "Matrix stack overflow");
        break;

    default:
        assert(0 && "Incorrect matrix g_matrixMode");
    }
}

void
fpcPopMatrix() noexcept
{
    switch (g_matrixMode)
    {
    case GL_MODELVIEW:
        if (g_modelViewPosition > 0)
            g_modelViewPosition--;
        else
            assert(0 && "Matrix stack underflow");
        break;

    case GL_PROJECTION:
        if (g_projectionPosition > 0)
            g_projectionPosition--;
        else
            assert(0 && "Matrix stack underflow");
        break;

    default:
        assert(0 && "Incorrect matrix g_matrixMode");
    }
}

void
fpcLoadIdentity() noexcept
{
    switch (g_matrixMode)
    {
    case GL_MODELVIEW:
        assert(g_modelViewPosition < MODELVIEW_STACK_DEPTH);
        if (g_modelViewPosition < MODELVIEW_STACK_DEPTH)
            g_modelViewStack[g_modelViewPosition] = Eigen::Matrix4f::Identity();
        break;

    case GL_PROJECTION:
        assert(g_projectionPosition < PROJECTION_STACK_DEPTH);
        if (g_projectionPosition < PROJECTION_STACK_DEPTH)
            g_projectionStack[g_projectionPosition] = Eigen::Matrix4f::Identity();
        break;

    default:
        assert(0 && "Incorrect matrix mode");
    }
}

void
fpcLoadMatrixf(const float *data) noexcept
{
    switch (g_matrixMode)
    {
    case GL_MODELVIEW:
        g_modelViewStack[g_modelViewPosition] = Eigen::Map<Eigen::Matrix4f>(const_cast<float*>(data));
        break;
    case GL_PROJECTION:
        g_projectionStack[g_projectionPosition] = Eigen::Map<Eigen::Matrix4f>(const_cast<float*>(data));
        break;
    default:
        assert("Incorrect matrix g_matrixMode");
    }
}

void fpcTranslatef(float x, float y, float z) noexcept
{
    Eigen::Matrix4f result;

    switch (g_matrixMode)
    {
    case GL_MODELVIEW:
        assert(g_modelViewPosition < MODELVIEW_STACK_DEPTH);
        if (g_modelViewPosition < MODELVIEW_STACK_DEPTH)
        {
            result = celmath::translate(g_modelViewStack[g_modelViewPosition], x, y, z);
            g_modelViewStack[g_modelViewPosition] = result;
        }
        break;

    case GL_PROJECTION:
        assert(g_projectionPosition < PROJECTION_STACK_DEPTH);
        if (g_projectionPosition < PROJECTION_STACK_DEPTH)
        {
            result = celmath::translate(g_projectionStack[g_projectionPosition], x, y, z);
            g_projectionStack[g_projectionPosition] = result;
        }
        break;

    default:
        assert(0 && "Incorrect matrix mode");
    }
}

void fpcMultMatrixf(const float *m) noexcept
{
    Eigen::Matrix4f result;

    switch (g_matrixMode)
    {
    case GL_MODELVIEW:
        assert(g_modelViewPosition < MODELVIEW_STACK_DEPTH);
        if (g_modelViewPosition < MODELVIEW_STACK_DEPTH)
        {
            result = g_modelViewStack[g_modelViewPosition] * Eigen::Map<Eigen::Matrix4f>(const_cast<float*>(m));
            g_modelViewStack[g_modelViewPosition] = result;
        }
        break;

    case GL_PROJECTION:
        assert(g_projectionPosition < PROJECTION_STACK_DEPTH);
        if (g_projectionPosition < PROJECTION_STACK_DEPTH)
        {
            result = g_projectionStack[g_projectionPosition] * Eigen::Map<Eigen::Matrix4f>(const_cast<float*>(m));
            g_projectionStack[g_projectionPosition] = result;
        }
        break;

    default:
        assert(0 && "Incorrect matrix mode");
    }
}

void fpcFrustum(float l, float r, float b, float t, float n, float f) noexcept
{
    Eigen::Matrix4f m;
    float rl = r - l;
    float tb = t - b;
    float fn = f - n;
    m << 2.0f*n/rl,      0.0f,  (r + l)/rl,         0.0f,
              0.0f, 2.0f*n/tb,  (t + b)/tb,         0.0f,
              0.0f,      0.0f, -(f + n)/fn, -2.0f*f*n/fn,
              0.0f,      0.0f,       -1.0f,         0.0f;

    fpcMultMatrixf(m.data());
}

void fpcOrtho(float l, float r, float b, float t, float n, float f) noexcept
{
    Eigen::Matrix4f m;
    float rl = r - l;
    float tb = t - b;
    float fn = f - n;
    m << 2.0f/rl,    0.0f,     0.0f, -(r + l)/rl,
            0.0f, 2.0f/tb,     0.0f, -(t + b)/tb,
            0.0f,    0.0f, -2.0f/fn, -(f + n)/fn,
            0.0f,    0.0f,     0.0f,        1.0f;

    fpcMultMatrixf(m.data());
}

void fpcGetFloatv(GLenum pname, GLfloat *params) noexcept
{
    switch (pname)
    {
    case GL_MODELVIEW_MATRIX:
        memcpy(params, g_modelViewStack[g_modelViewPosition].data(), 16*sizeof(float));
        break;
    case GL_PROJECTION_MATRIX:
        memcpy(params, g_projectionStack[g_projectionPosition].data(), 16*sizeof(float));
        break;
    default:
        orig_glGetFloatv(pname, params);
        break;
    }
}

void fpcEnable(GLenum param) noexcept
{
    switch(param)
    {
#ifndef GL_ES
    case GL_LINE_SMOOTH:
#endif
    case GL_BLEND:
        orig_glEnable(param);
    default:
        break;
    }
}

void fpcDisable(GLenum param) noexcept
{
    switch(param)
    {
#ifndef GL_ES
    case GL_LINE_SMOOTH:
#endif
    case GL_BLEND:
        orig_glDisable(param);
    default:
        break;
    }
}

void fpcBegin(GLenum param) noexcept
{
    if (gPrimitive == 0)
        gPrimitive = (param == GL_POLYGON || param == GL_QUADS) ? GL_TRIANGLE_FAN : param;
}

void fpcEnd() noexcept
{
    Draw();
}

void fpcColor4f(float r, float g, float b, float a) noexcept
{
    if (gPrimitive == GL_NONE)
        glVertexAttrib4f(CelestiaGLProgram::ColorAttributeIndex, r, g, b, a);
    else
        PushColor(r, g, b, a);
}

void fpcVertex2f(float x, float y) noexcept
{
    PushVertex(x, y);
}

void fpcTexCoord2f(float x, float y) noexcept
{
    PushTexCoord(x, y);
}

void gluLookAt(float ix, float iy, float iz, float cx, float cy, float cz, float ux, float uy, float uz) noexcept
{
    Eigen::Vector3f forward(cx - ix, cy - iy, cz - iz);
    forward.normalize();
    Eigen::Vector3f up(ux, uy, uz);
    Eigen::Vector3f side = forward.cross(up).normalized();
    // Recompute up as: up = side x forward
    up = side.cross(forward);

    Eigen::Matrix4f m(Eigen::Matrix4f::Identity());
    m.col(0).head(3) = side;
    m.col(1).head(3) = up;
    m.col(2).head(3) = -forward;

    fpcMultMatrixf(&m(0, 0));
    fpcTranslatef(-ix, -iy, -iz);
}
