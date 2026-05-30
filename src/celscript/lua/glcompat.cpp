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
#include <cstddef>                      // offsetof
#include <cstring>                      // memcpy
#include <memory>
#include <Eigen/Core>
#include <fmt/format.h>
#include <celengine/glsupport.h>
#include <celengine/shadermanager.h>    // CelestiaGLProgram::*Index
#include <celmath/vecgl.h>              // math::translate
#include "glcompat.h"


namespace math = celestia::math;

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
float g_pointSize = 1.0f; // NOSONAR(cpp:S5421) immediate-mode state shadow

// Lua-emulator blend state. Tracked in shadows so the engine's actual GL
// blend state (and Renderer::m_pipelineState cache) is never modified outside
// of Draw(), where we save/restore around the draw call.
bool   g_blendEnabled = false;                       // NOSONAR(cpp:S5421)
GLenum g_blendSrc     = GL_SRC_ALPHA;                // NOSONAR(cpp:S5421)
GLenum g_blendDst     = GL_ONE_MINUS_SRC_ALPHA;      // NOSONAR(cpp:S5421)

class GLSLProgram
{
public:
    explicit GLSLProgram(GLProgram&& prog) : m_prog(std::move(prog))
    {
        m_MVPMatrix = Mat4ShaderParameter(m_prog.getID(), "MVPMatrix");
        m_PointSize = FloatShaderParameter(m_prog.getID(), "PointSize");
        IntegerShaderParameter(m_prog.getID(), "u_tex") = 0;
    }
    ~GLSLProgram() = default;
    GLSLProgram(const GLSLProgram&) = delete;
    GLSLProgram& operator=(const GLSLProgram&) = delete;
    GLSLProgram(GLSLProgram&&) = default;
    GLSLProgram& operator=(GLSLProgram&&) = default;

    void use()
    {
        m_prog.use();
    }
    void setMVPMatrix(const Eigen::Matrix4f &m)
    {
        m_MVPMatrix = m;
    }
    void setPointSize(float s)
    {
        m_PointSize = s;
    }

private:
    GLProgram m_prog;
    Mat4ShaderParameter m_MVPMatrix;
    FloatShaderParameter m_PointSize;
};

#ifdef GL_ES
constexpr char glsl_version[] = "300 es";
#else
constexpr char glsl_version[] = "330";
#endif

constexpr char kVertexShader[] = R"glsl(#version {}
#define SHADER_COLOR {}
#define SHADER_TEXCOORD {}

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) in vec2 in_Position;
layout(location = 2) in vec2 in_TexCoord0;
#if SHADER_COLOR
layout(location = 8) in vec4 in_Color;
out vec4 v_color;
#endif
#if SHADER_TEXCOORD
out vec2 v_texCoord;
#endif

//uniform mat4 ModelViewMatrix;
//uniform mat4 ProjectionMatrix;
uniform mat4 MVPMatrix;
uniform float PointSize;

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
    gl_PointSize = PointSize;
}}
)glsl";

constexpr char kFragmentShader[] = R"glsl(#version {}
#define SHADER_COLOR {}
#define SHADER_TEXCOORD {}

#ifdef GL_ES
precision highp float;
#endif

#if SHADER_COLOR
in vec4 v_color;
#endif
#if SHADER_TEXCOORD
in vec2 v_texCoord;
uniform sampler2D u_tex;
#endif

out vec4 fragColor;

void main(void)
{{
#if SHADER_TEXCOORD
    fragColor = texture(u_tex, v_texCoord);
#endif
#if SHADER_COLOR
    fragColor = v_color;
#endif
}}
)glsl";

enum ShaderAttributes
{
    SHADER_COLOR    = 0,
    SHADER_TEXCOORD = 1,
    SHADER_COUNT    = 2
};

// GL_POINTS == 0 == GL_NONE, so we can't use GL_NONE as the "no primitive
// active" sentinel — that would silently swallow every POINTS draw. Use an
// out-of-range value instead.
constexpr GLenum kNoPrimitive = ~GLenum(0);

GLenum gPrimitive = kNoPrimitive;       // NOSONAR(cpp:S5421) immediate-mode state shadow
GLsizei gVertexCounter = 0;             // NOSONAR(cpp:S5421)
GLsizei gTexCoordCounter = 0;           // NOSONAR(cpp:S5421)
GLsizei gColorCounter = 0;              // NOSONAR(cpp:S5421)

struct Vertex
{
    float x, y, u, v, r, g, b, a;
};

std::array<Vertex, 16> vertices {};

GLProgram BuildProgram(const std::string &vertex, const std::string &fragment)
{
    GLShaderStatus status = GLShaderStatus::OK;
    auto vs = GLVertexShader::create(vertex, status);
    auto fs = GLFragmentShader::create(fragment, status);
    if (!vs.isValid() || !fs.isValid())
        return GLProgram{};

    auto builder = GLProgramBuilder::create(status);
    if (status != GLShaderStatus::OK)
        return GLProgram{};

    builder.attach(std::move(vs));
    builder.attach(std::move(fs));

    if (auto program = builder.link(status); status == GLShaderStatus::OK)
        return program;

    return GLProgram{};
}

GLSLProgram* FindGLProgram(ShaderAttributes attr)
{
    static std::array<GLSLProgram*, SHADER_COUNT> programs;
    if (programs[attr] == nullptr)
    {
        int color = attr == SHADER_COLOR ? 1 : 0;
        int texture = attr == SHADER_TEXCOORD ? 1 : 0;
        std::string vertex = fmt::format(kVertexShader, glsl_version, color, texture);
        std::string fragment = fmt::format(kFragmentShader, glsl_version, color, texture);
        auto glprog = BuildProgram(vertex, fragment);
        if (glprog.isValid())
            programs[attr] = std::make_unique<GLSLProgram>(std::move(glprog)).release();
    }
    return programs[attr];
}


GLuint gVao = 0;        // NOSONAR(cpp:S5421) lazily-created GL object handle
GLuint gVbo = 0;        // NOSONAR(cpp:S5421)

void EnsureBuffers()
{
    if (gVao == 0)
    {
        glGenVertexArrays(1, &gVao);
        glGenBuffers(1, &gVbo);
        glBindVertexArray(gVao);
        glBindBuffer(GL_ARRAY_BUFFER, gVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), nullptr, GL_DYNAMIC_DRAW);
    }
}

void Draw()
{
    if (gPrimitive == kNoPrimitive || gVertexCounter == 0)
        return; // Do nothing

    bool hasTexCoords       = gTexCoordCounter == gVertexCounter;
    bool hasColors          = gColorCounter == gVertexCounter;
    ShaderAttributes type   = hasTexCoords ? SHADER_TEXCOORD : SHADER_COLOR;

    // Core profile needs a VAO + a real VBO; client-side arrays are gone.
    GLint prevVao = 0;
    GLint prevVbo = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevVbo);

    // Save the engine's generic color attribute — celx_gl.cpp::gl_Color
    // calls glVertexAttrib4f on location 8 directly, so without this the
    // engine's HUD (which reads the generic value when its color array is
    // disabled) inherits whatever the last Lua gl.Color set.
    std::array<GLfloat, 4> prevGenericColor{};
    glGetVertexAttribfv(CelestiaGLProgram::ColorAttributeIndex,
                        GL_CURRENT_VERTEX_ATTRIB, prevGenericColor.data());
    GLint prevProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);

    EnsureBuffers();
    glBindVertexArray(gVao);
    glBindBuffer(GL_ARRAY_BUFFER, gVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * gVertexCounter, vertices.data());

    glEnableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glVertexAttribPointer(
        CelestiaGLProgram::VertexCoordAttributeIndex,
        2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        reinterpret_cast<const void*>(offsetof(Vertex, x))
    );

    if (hasTexCoords)
    {
        glEnableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex);
        glVertexAttribPointer(
            CelestiaGLProgram::TextureCoord0AttributeIndex,
            2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<const void*>(offsetof(Vertex, u))
        );
    }
    else
    {
        glDisableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex);
    }

    if (hasColors)
    {
        glEnableVertexAttribArray(CelestiaGLProgram::ColorAttributeIndex);
        glVertexAttribPointer(
            CelestiaGLProgram::ColorAttributeIndex,
            4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<const void*>(offsetof(Vertex, r))
        );
    }
    else
    {
        glDisableVertexAttribArray(CelestiaGLProgram::ColorAttributeIndex);
    }

    if (auto *prog = FindGLProgram(type); prog != nullptr)
    {
        prog->use();
        prog->setMVPMatrix(g_projectionStack[g_projectionPosition] * g_modelViewStack[g_modelViewPosition]);
        prog->setPointSize(g_pointSize);
#ifndef GL_ES
        // Desktop Core: gl_PointSize is only honored when this is enabled.
        // Use orig_glEnable because glEnable is macro-redirected to fpcEnable.
        GLboolean wasPointSizeEnabled = glIsEnabled(GL_PROGRAM_POINT_SIZE);
        if (!wasPointSizeEnabled)
            orig_glEnable(GL_PROGRAM_POINT_SIZE);
#endif
        // Snapshot real GL blend state, apply Lua-emulator blend state for this
        // draw only, then restore. This keeps the engine's Renderer pipeline
        // state cache in sync with actual GL state across the Lua hook.
        GLboolean prevBlend = glIsEnabled(GL_BLEND);
        GLint prevSrcRGB = 0, prevDstRGB = 0, prevSrcA = 0, prevDstA = 0;
        glGetIntegerv(GL_BLEND_SRC_RGB, &prevSrcRGB);
        glGetIntegerv(GL_BLEND_DST_RGB, &prevDstRGB);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &prevSrcA);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &prevDstA);
        if (g_blendEnabled && !prevBlend)
            orig_glEnable(GL_BLEND);
        else if (!g_blendEnabled && prevBlend)
            orig_glDisable(GL_BLEND);
        if (g_blendEnabled)
            glBlendFuncSeparate(g_blendSrc, g_blendDst, GL_ZERO, GL_ONE);

        glDrawArrays(gPrimitive, 0, gVertexCounter);

        // Restore blend state exactly as the engine left it.
        if (g_blendEnabled && !prevBlend)
            orig_glDisable(GL_BLEND);
        else if (!g_blendEnabled && prevBlend)
            orig_glEnable(GL_BLEND);
        glBlendFuncSeparate(static_cast<GLenum>(prevSrcRGB),
                                  static_cast<GLenum>(prevDstRGB),
                                  static_cast<GLenum>(prevSrcA),
                                  static_cast<GLenum>(prevDstA));
#ifndef GL_ES
        if (!wasPointSizeEnabled)
            orig_glDisable(GL_PROGRAM_POINT_SIZE);
#endif
    }

    // Restore previous binding so we don't pollute the engine's VAO.
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(prevVbo));
    glBindVertexArray(static_cast<GLuint>(prevVao));

    // Restore generic color attribute so the engine's HUD/labels don't
    // inherit the last Lua gl.Color value.
    glVertexAttrib4fv(CelestiaGLProgram::ColorAttributeIndex, prevGenericColor.data());
    if (prevProgram >= 0)
        glUseProgram(static_cast<GLuint>(prevProgram));

    // reset state
    gVertexCounter = 0;
    gTexCoordCounter = 0;
    gColorCounter = 0;
    gPrimitive = kNoPrimitive;
}

void PushVertex(float x, float y)
{
    if (gVertexCounter < static_cast<GLsizei>(vertices.size()))
    {
        vertices[gVertexCounter].x = x;
        vertices[gVertexCounter].y = y;
        gVertexCounter++;
    }
}

void PushTexCoord(float u, float v)
{
    if (gTexCoordCounter < static_cast<GLsizei>(vertices.size()))
    {
        vertices[gTexCoordCounter].u = u;
        vertices[gTexCoordCounter].v = v;
        gTexCoordCounter++;
    }
}

void PushColor(float r, float g, float b, float a)
{
    if (gColorCounter < static_cast<GLsizei>(vertices.size()))
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
fpcPointSize(float size) noexcept
{
    g_pointSize = size;
}

void
fpcPushMatrix() noexcept
{
    switch (g_matrixMode)
    {
    case GL_MODELVIEW:
        if (g_modelViewPosition < MODELVIEW_STACK_DEPTH - 1)
        {
            g_modelViewStack[g_modelViewPosition + 1] = g_modelViewStack[g_modelViewPosition];
            g_modelViewPosition++;
        }
        else
            assert(0 && "Matrix stack overflow");
        break;

    case GL_PROJECTION:
        if (g_projectionPosition < PROJECTION_STACK_DEPTH - 1)
        {
            g_projectionStack[g_projectionPosition + 1] = g_projectionStack[g_projectionPosition];
            g_projectionPosition++;
        }
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
            result = math::translate(g_modelViewStack[g_modelViewPosition], x, y, z);
            g_modelViewStack[g_modelViewPosition] = result;
        }
        break;

    case GL_PROJECTION:
        assert(g_projectionPosition < PROJECTION_STACK_DEPTH);
        if (g_projectionPosition < PROJECTION_STACK_DEPTH)
        {
            result = math::translate(g_projectionStack[g_projectionPosition], x, y, z);
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
    // Do NOT touch real GL state from the Lua hook. The engine maintains a
    // cached PipelineState that would otherwise get out of sync. We shadow
    // blend state and apply it only inside Draw() with save/restore.
    if (param == GL_BLEND)
        g_blendEnabled = true;
}

void fpcDisable(GLenum param) noexcept
{
    if (param == GL_BLEND)
        g_blendEnabled = false;
}

void fpcBlendFunc(GLenum src, GLenum dst) noexcept
{
    // Shadow only; applied inside Draw() with save/restore so the engine's
    // cached blend func stays consistent across the Lua hook.
    g_blendSrc = src;
    g_blendDst = dst;
}

void fpcBegin(GLenum param) noexcept
{
    if (gPrimitive == kNoPrimitive)
        gPrimitive = (param == GL_POLYGON || param == GL_QUADS) ? GL_TRIANGLE_FAN : param;
}

void fpcEnd() noexcept
{
    Draw();
}

void fpcColor4f(float r, float g, float b, float a) noexcept
{
    if (gPrimitive == kNoPrimitive)
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

void fpcLookAt(float ix, float iy, float iz, float cx, float cy, float cz, float ux, float uy, float uz) noexcept
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
