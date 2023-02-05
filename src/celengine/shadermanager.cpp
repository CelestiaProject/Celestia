// shadermanager.cpp
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <optional>
#include <ostream>
#include <sstream>
#include <system_error>
#include <tuple>

#include <fmt/format.h>

#include <celcompat/filesystem.h>
#include <celutil/logger.h>
#include "atmosphere.h"
#include "glsupport.h"
#include "lightenv.h"
#include "shadermanager.h"


using celestia::util::GetLogger;
using namespace std::string_view_literals;
namespace gl = celestia::gl;

#define POINT_FADE 1

namespace
{
constexpr const int ShadowSampleKernelWidth = 2;

enum ShaderVariableType
{
    Shader_Float,
    Shader_Vector2,
    Shader_Vector3,
    Shader_Vector4,
    Shader_Matrix4,
    Shader_Sampler1D,
    Shader_Sampler2D,
    Shader_Sampler3D,
    Shader_SamplerCube,
    Shader_Sampler1DShadow,
    Shader_Sampler2DShadow,
};

constexpr std::string_view errorVertexShaderSource = R"glsl(
attribute vec4 in_Position;\n
void main(void)
{
    gl_Position = MVPMatrix * in_Position;
}
)glsl"sv;

constexpr std::string_view errorFragmentShaderSource = R"glsl(
void main(void)
{
   gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
)glsl"sv;

#ifndef GL_ES
constexpr std::string_view VersionHeader = "#version 120\n"sv;
constexpr std::string_view VersionHeaderGL3 = "#version 150\n"sv;
constexpr std::string_view CommonHeader = "\n"sv;
#else
constexpr std::string_view VersionHeader = "#version 100\n"sv;
constexpr std::string_view VersionHeaderGL3 = "#version 320 es\n"sv;
constexpr std::string_view CommonHeader = "precision highp float;\n"sv;
#endif
constexpr std::string_view VertexHeader = R"glsl(
uniform mat4 ModelViewMatrix;
uniform mat4 ProjectionMatrix;
uniform mat4 MVPMatrix;

invariant gl_Position;
)glsl"sv;

constexpr std::string_view GeomHeaderGL3 = VertexHeader;

constexpr std::string_view VPFunctionFishEye = R"glsl(
vec4 calc_vp(vec4 in_Position)
{
    float PID2 = 1.570796326794896619231322;
    vec4 inPos = ModelViewMatrix * in_Position;
    float l = length(inPos.xy);
    if (l != 0.0)
    {
        float phi = atan(l, -inPos.z);
        float lensR = phi / PID2;
        inPos.xy *= (lensR / l);
    }
    return ProjectionMatrix * inPos;
}
void set_vp(vec4 in_Position)
{
    gl_Position = calc_vp(in_Position);
}
)glsl"sv;

constexpr std::string_view VPFunctionUsual = R"glsl(
vec4 calc_vp(vec4 in_Position)
{
    return MVPMatrix * in_Position;
}
void set_vp(vec4 in_Position)
{
    gl_Position = calc_vp(in_Position);
}
)glsl"sv;

constexpr std::string_view
VPFunction(bool enabled)
{
    return enabled ? VPFunctionFishEye : VPFunctionUsual;
}

constexpr std::string_view NormalVertexPosition = R"glsl(
    set_vp(in_Position);
)glsl"sv;

constexpr std::string_view LineVertexPosition = R"glsl(
    vec4 thisPos = calc_vp(in_Position);
    vec4 nextPos = calc_vp(in_PositionNext);
    float w = thisPos.w;
    thisPos /= w;
    nextPos /= nextPos.w;
    vec2 transform = normalize(nextPos.xy - thisPos.xy);
    transform = vec2(transform.y * lineWidthX, -transform.x * lineWidthY) * in_ScaleFactor;
    gl_Position = thisPos;
    gl_Position.xy += transform;
    gl_Position *= w;
)glsl"sv;

std::string_view
VertexPosition(const ShaderProperties& props)
{
    return (props.texUsage & ShaderProperties::LineAsTriangles) ? LineVertexPosition : NormalVertexPosition;
}

constexpr std::string_view FragmentHeader = ""sv;

constexpr std::string_view CommonAttribs = R"glsl(
attribute vec4 in_Position;
attribute vec3 in_Normal;
attribute vec4 in_TexCoord0;
attribute vec4 in_TexCoord1;
attribute vec4 in_TexCoord2;
attribute vec4 in_TexCoord3;
attribute vec4 in_Color;
)glsl"sv;

std::string
LightProperty(unsigned int i, std::string_view property)
{
#ifndef USE_GLSL_STRUCTS
    return fmt::format("light{}_{}", i, property);
#else
    return fmt::format("lights[{}].{}", i, property);
#endif
}

std::string
FragLightProperty(unsigned int i, std::string_view property)
{
    return fmt::format("light{}{}", property, i);
}

std::string
ApplyShadow(bool hasSpecular)
{
    std::string_view code = R"glsl(
if (cosNormalLightDir > 0.0)
{{
    shadowMapCoeff = calculateShadow();
    diff.rgb *= shadowMapCoeff;
    {}
}}
)glsl"sv;

    return fmt::format(code, hasSpecular ? "spec.rgb *= shadowMapCoeff;" : "");
}

constexpr std::string_view
ShaderTypeString(ShaderVariableType type)
{
    switch (type)
    {
    case Shader_Float:
        return "float";
    case Shader_Vector2:
        return "vec2";
    case Shader_Vector3:
        return "vec3";
    case Shader_Vector4:
        return "vec4";
    case Shader_Matrix4:
        return "mat4";
    case Shader_Sampler1D:
        return "sampler1D";
    case Shader_Sampler2D:
        return "sampler2D";
    case Shader_Sampler3D:
        return "sampler3D";
    case Shader_SamplerCube:
        return "samplerCube";
    case Shader_Sampler1DShadow:
        return "sampler1DShadow";
    case Shader_Sampler2DShadow:
        return "sampler2DShadow";
    default:
        return "unknown";
    }
}

std::string
IndexedParameter(std::string_view name, unsigned int index0)
{
    return fmt::format("{}{}", name, index0);
}

std::string
IndexedParameter(std::string_view name, unsigned int index0, unsigned int index1)
{
    return fmt::format("{}{}_{}", name, index0, index1);
}


class Sh_ExpressionContents
{
protected:
    Sh_ExpressionContents() = default;
    virtual ~Sh_ExpressionContents() = default;

public:
    virtual std::string toString() const = 0;
    virtual int precedence() const = 0;
    std::string toStringPrecedence(int parentPrecedence) const
    {
        if (parentPrecedence >= precedence())
            return fmt::format("({})", toString());
        else
            return toString();
    }

    int addRef() const
    {
        return ++m_refCount;
    }

    int release() const
    {
        int n = --m_refCount;
        if (m_refCount == 0)
            delete this;
        return n;
    }

private:
    mutable int m_refCount{0};
};


class Sh_ConstantExpression : public Sh_ExpressionContents
{
public:
    Sh_ConstantExpression(float value) : m_value(value) {}
    std::string toString() const override
    {
        return std::to_string(m_value);
    }

    int precedence() const override { return 100; }

private:
    float m_value;
};


class Sh_Expression
{
public:
    Sh_Expression(const Sh_ExpressionContents* expr) :
        m_contents(expr)
    {
        m_contents->addRef();
    }

    Sh_Expression(const Sh_Expression& other) :
        m_contents(other.m_contents)
    {
        m_contents->addRef();
    }

    Sh_Expression& operator=(const Sh_Expression& other)
    {
        if (other.m_contents != m_contents)
        {
            m_contents->release();
            m_contents = other.m_contents;
            m_contents->addRef();
        }

        return *this;
    }

    Sh_Expression(float f) :
        m_contents(new Sh_ConstantExpression(f))
    {
        m_contents->addRef();
    }

    ~Sh_Expression()
    {
        m_contents->release();
    }

    std::string toString() const
    {
        return m_contents->toString();
    }

    std::string toStringPrecedence(int parentPrecedence) const
    {
        return m_contents->toStringPrecedence(parentPrecedence);
    }

    int precedence() const
    {
        return m_contents->precedence();
    }

    // [] operator acts like swizzle
    Sh_Expression operator[](const std::string& components) const;

private:
    const Sh_ExpressionContents* m_contents;
};


class Sh_VariableExpression : public Sh_ExpressionContents
{
public:
    Sh_VariableExpression(std::string_view name) : m_name(name) {}
    explicit Sh_VariableExpression(const std::string &name) : m_name(name) {}

    std::string toString() const override
    {
        return m_name;
    }

    int precedence() const override { return 100; }

private:
    const std::string m_name;
};

class Sh_SwizzleExpression : public Sh_ExpressionContents
{
public:
    Sh_SwizzleExpression(const Sh_Expression& expr, const std::string& components) :
        m_expr(expr),
        m_components(components)
    {
    }

    std::string toString() const override
    {
        return m_expr.toStringPrecedence(precedence()) + "." + m_components;
    }

    int precedence() const override { return 99; }

private:
    const Sh_Expression m_expr;
    const std::string m_components;
};

class Sh_BinaryExpression : public Sh_ExpressionContents
{
public:
    Sh_BinaryExpression(const std::string& op, int precedence, const Sh_Expression& left, const Sh_Expression& right) :
        m_op(op),
        m_precedence(precedence),
        m_left(left),
        m_right(right) {};

    std::string toString() const override
    {
        return left().toStringPrecedence(precedence()) + op() + right().toStringPrecedence(precedence());
    }

    const Sh_Expression& left() const { return m_left; }
    const Sh_Expression& right() const { return m_right; }
    std::string op() const { return m_op; }
    int precedence() const override { return m_precedence; }

private:
    std::string m_op;
    int m_precedence;
    const Sh_Expression m_left;
    const Sh_Expression m_right;
};

class Sh_AdditionExpression : public Sh_BinaryExpression
{
public:
    Sh_AdditionExpression(const Sh_Expression& left, const Sh_Expression& right) :
        Sh_BinaryExpression("+", 1, left, right) {}
};

class Sh_SubtractionExpression : public Sh_BinaryExpression
{
public:
    Sh_SubtractionExpression(const Sh_Expression& left, const Sh_Expression& right) :
        Sh_BinaryExpression("-", 1, left, right) {}
};

class Sh_MultiplicationExpression : public Sh_BinaryExpression
{
public:
    Sh_MultiplicationExpression(const Sh_Expression& left, const Sh_Expression& right) :
        Sh_BinaryExpression("*", 2, left, right) {}
};

class Sh_DivisionExpression : public Sh_BinaryExpression
{
public:
    Sh_DivisionExpression(const Sh_Expression& left, const Sh_Expression& right) :
        Sh_BinaryExpression("/", 2, left, right) {}
};

Sh_Expression operator+(const Sh_Expression& left, const Sh_Expression& right)
{
    return new Sh_AdditionExpression(left, right);
}

Sh_Expression operator-(const Sh_Expression& left, const Sh_Expression& right)
{
    return new Sh_SubtractionExpression(left, right);
}

Sh_Expression operator*(const Sh_Expression& left, const Sh_Expression& right)
{
    return new Sh_MultiplicationExpression(left, right);
}

Sh_Expression operator/(const Sh_Expression& left, const Sh_Expression& right)
{
    return new Sh_DivisionExpression(left, right);
}

Sh_Expression Sh_Expression::operator[](const std::string& components) const
{
    return new Sh_SwizzleExpression(*this, components);
}

class Sh_UnaryFunctionExpression : public Sh_ExpressionContents
{
public:
    Sh_UnaryFunctionExpression(std::string_view name, const Sh_Expression& arg0) :
        m_name(name), m_arg0(arg0) {}

    std::string toString() const override
    {
        return fmt::format("{}({})", m_name, m_arg0.toString());
    }

    int precedence() const override { return 100; }

private:
    std::string m_name;
    const Sh_Expression m_arg0;
};

class Sh_BinaryFunctionExpression : public Sh_ExpressionContents
{
public:
    Sh_BinaryFunctionExpression(std::string_view name, const Sh_Expression& arg0, const Sh_Expression& arg1) :
        m_name(name), m_arg0(arg0), m_arg1(arg1) {}

    std::string toString() const override
    {
        return fmt::format("{}({}, {})", m_name, m_arg0.toString(), m_arg1.toString());
    }

    int precedence() const override { return 100; }

private:
    std::string m_name;
    const Sh_Expression m_arg0;
    const Sh_Expression m_arg1;
};

class Sh_TernaryFunctionExpression : public Sh_ExpressionContents
{
public:
    Sh_TernaryFunctionExpression(std::string_view name, const Sh_Expression& arg0, const Sh_Expression& arg1, const Sh_Expression& arg2) :
        m_name(name), m_arg0(arg0), m_arg1(arg1), m_arg2(arg2) {}

    std::string toString() const override
    {
        return fmt::format("{}({}, {}, {})", m_name, m_arg0.toString(), m_arg1.toString(), m_arg2.toString());
    }

    int precedence() const override { return 100; }

private:
    std::string m_name;
    const Sh_Expression m_arg0;
    const Sh_Expression m_arg1;
    const Sh_Expression m_arg2;
};

Sh_Expression vec2(const Sh_Expression& x, const Sh_Expression& y)
{
    return new Sh_BinaryFunctionExpression("vec2", x, y);
}

Sh_Expression vec3(const Sh_Expression& x, const Sh_Expression& y, const Sh_Expression& z)
{
    return new Sh_TernaryFunctionExpression("vec3", x, y, z);
}

Sh_Expression dot(const Sh_Expression& v0, const Sh_Expression& v1)
{
    return new Sh_BinaryFunctionExpression("dot", v0, v1);
}

Sh_Expression cross(const Sh_Expression& v0, const Sh_Expression& v1)
{
    return new Sh_BinaryFunctionExpression("cross", v0, v1);
}

Sh_Expression sqrt(const Sh_Expression& v0)
{
    return new Sh_UnaryFunctionExpression("sqrt", v0);
}

Sh_Expression length(const Sh_Expression& v0)
{
    return new Sh_UnaryFunctionExpression("length", v0);
}

Sh_Expression normalize(const Sh_Expression& v0)
{
    return new Sh_UnaryFunctionExpression("normalize", v0);
}

Sh_Expression step(const Sh_Expression& f, const Sh_Expression& v)
{
    return new Sh_BinaryFunctionExpression("step", f, v);
}

Sh_Expression mix(const Sh_Expression& v0, const Sh_Expression& v1, const Sh_Expression& alpha)
{
    return new Sh_TernaryFunctionExpression("mix", v0, v1, alpha);
}

Sh_Expression min(const Sh_Expression& v0, const Sh_Expression& v1)
{
    return new Sh_BinaryFunctionExpression("min", v0, v1);
}

Sh_Expression max(const Sh_Expression& v0, const Sh_Expression& v1)
{
    return new Sh_BinaryFunctionExpression("max", v0, v1);
}

Sh_Expression texture2D(const Sh_Expression& sampler, const Sh_Expression& texCoord)
{
    return new Sh_BinaryFunctionExpression("texture2D", sampler, texCoord);
}

Sh_Expression texture2DLod(const Sh_Expression& sampler, const Sh_Expression& texCoord, const Sh_Expression& lod)
{
    return new Sh_TernaryFunctionExpression("texture2DLod", sampler, texCoord, lod);
}

Sh_Expression texture2DLodBias(const Sh_Expression& sampler, const Sh_Expression& texCoord, const Sh_Expression& lodBias)
{
    // Use the optional third argument to texture2D to specify the LOD bias. Implemented with
    // a different function name here for clarity when it's used in a shader.
    return new Sh_TernaryFunctionExpression("texture2D", sampler, texCoord, lodBias);
}

Sh_Expression sampler2D(std::string_view name)
{
    return new Sh_VariableExpression(name);
}

Sh_Expression sh_vec3(std::string_view name)
{
    return new Sh_VariableExpression(name);
}

Sh_Expression sh_vec4(std::string_view name)
{
    return new Sh_VariableExpression(name);
}

Sh_Expression sh_float(std::string_view name)
{
    return new Sh_VariableExpression(name);
}

Sh_Expression indexedUniform(std::string_view name, unsigned int index0)
{
    auto buf = fmt::format("{}{}", name, index0);
    return new Sh_VariableExpression(buf);
}

Sh_Expression ringShadowTexCoord(unsigned int index)
{
    auto buf = fmt::format("ringShadowTexCoord.{}", "xyzw"[index]);
    return new Sh_VariableExpression(buf);
}

Sh_Expression cloudShadowTexCoord(unsigned int index)
{
    auto buf = fmt::format("cloudShadowTexCoord{}", index);
    return new Sh_VariableExpression(std::string(buf));
}

std::string
DeclareUniform(std::string_view name, ShaderVariableType type)
{
    return fmt::format("uniform {} {};\n", ShaderTypeString(type), name);
}

std::string
DeclareVarying(std::string_view name, ShaderVariableType type)
{
    return fmt::format("varying {} {};\n", ShaderTypeString(type), name);
}

std::string
DeclareAttribute(std::string_view name, ShaderVariableType type)
{
    return fmt::format("attribute {} {};\n", ShaderTypeString(type), name);
}

std::string
DeclareLocal(std::string_view name, ShaderVariableType type)
{
    return fmt::format("{} {};\n", ShaderTypeString(type), name);
}

std::string
DeclareLocal(std::string_view name, ShaderVariableType type, const Sh_Expression& expr)
{
    return fmt::format("{} {} = {};\n", ShaderTypeString(type), name, expr.toString());
}

std::string
assign(std::string_view variableName, const Sh_Expression& expr)
{
    return fmt::format("{} = {};\n", variableName, expr.toString());
}

std::string
addAssign(std::string_view variableName, const Sh_Expression& expr)
{
    return fmt::format("{} += {};\n", variableName, expr.toString());
}

std::string
mulAssign(std::string_view variableName, const Sh_Expression& expr)
{
    return fmt::format("{} *= {};\n", variableName, expr.toString());
}

std::string
RingShadowTexCoord(unsigned int index)
{
    return fmt::format("ringShadowTexCoord.{}", "xyzw"[index]);
}

std::string
CloudShadowTexCoord(unsigned int index)
{
    return fmt::format("cloudShadowTexCoord{}", index);
}

void
DumpShaderSource(std::ostream& out, const std::string& source)
{
    bool newline = true;
    unsigned int lineNumber = 0;

    for (unsigned int i = 0; i < source.length(); i++)
    {
        if (newline)
        {
            lineNumber++;
            out << std::setw(3) << lineNumber << ": ";
            newline = false;
        }

        out << source[i];
        if (source[i] == '\n')
            newline = true;
    }

    out.flush();
}


inline void DumpVSSource(const std::string& source)
{
    if (g_shaderLogFile != nullptr)
    {
        *g_shaderLogFile << "Vertex shader source:\n";
        DumpShaderSource(*g_shaderLogFile, source);
        *g_shaderLogFile << '\n';
    }
}

inline void DumpVSSource(std::ostringstream& source)
{
    DumpVSSource(source.str());
}

inline void DumpGSSource(const std::string& source)
{
    if (g_shaderLogFile != nullptr)
    {
        *g_shaderLogFile << "Geometry shader source:\n";
        DumpShaderSource(*g_shaderLogFile, source);
        *g_shaderLogFile << '\n';
    }
}

inline void DumpFSSource(const std::string& source)
{
    if (g_shaderLogFile != nullptr)
    {
        *g_shaderLogFile << "Fragment shader source:\n";
        DumpShaderSource(*g_shaderLogFile, source);
        *g_shaderLogFile << '\n';
    }
}

inline void DumpFSSource(std::ostringstream& source)
{
    DumpFSSource(source.str());
}

std::string
DeclareLights(const ShaderProperties& props)
{
    if (props.nLights == 0)
        return {};

    std::ostringstream stream;
#ifndef USE_GLSL_STRUCTS
    for (unsigned int i = 0; i < props.nLights; i++)
    {
        stream << DeclareUniform(fmt::format("light{}_direction", i), Shader_Vector3);
        stream << DeclareUniform(fmt::format("light{}_diffuse", i), Shader_Vector3);
        if (props.hasSpecular())
        {
            stream << DeclareUniform(fmt::format("light{}_specular", i), Shader_Vector3);
            stream << DeclareUniform(fmt::format("light{}_halfVector", i), Shader_Vector3);
        }
        if (props.texUsage & ShaderProperties::NightTexture)
            stream << DeclareUniform(fmt::format("light{}_brightness", i), Shader_Float);
    }

#else
    stream << "uniform struct {\n";
    stream << "   vec3 direction;\n";
    stream << "   vec3 diffuse;\n";
    stream << "   vec3 specular;\n";
    stream << "   vec3 halfVector;\n";
    if (props.texUsage & ShaderProperties::NightTexture)
        stream << "   float brightness;\n";
    stream << "} lights[" << props.nLights << "];\n";
#endif

    return stream.str();
}


std::string
SeparateDiffuse(unsigned int i)
{
    // Used for packing multiple diffuse factors into the diffuse color.
    return fmt::format("diffFactors.{}", "xyzw"[i & 3]);
}


std::string
SeparateSpecular(unsigned int i)
{
    // Used for packing multiple specular factors into the specular color.
    return fmt::format("specFactors.{}", "xyzw"[i & 3]);
}


// Used by rings shader
std::string
ShadowDepth(unsigned int i)
{
    return fmt::format("shadowDepths.{}", "xyzw"[i & 3]);
}


std::string
TexCoord2D(unsigned int i)
{
    return fmt::format("in_TexCoord{}.st", i);
}


// Tangent space light direction
std::string
LightDir_tan(unsigned int i)
{
    return fmt::format("lightDir_tan_{}", i);
}


std::string
ScatteredColor(unsigned int i)
{
    return fmt::format("scatteredColor{}", i);
}


std::string
TangentSpaceTransform(std::string_view dst, std::string_view src)
{
    return fmt::format("{0}.x = dot(in_Tangent, {1});\n"
                       "{0}.y = dot(-bitangent, {1});\n"
                       "{0}.z = dot(in_Normal, {1});\n",
                       dst, src);
}


constexpr std::string_view
NightTextureBlend()
{
#if 1
    // Output the blend factor for night lights textures
    return "totalLight = 1.0 - totalLight;\ntotalLight = totalLight * totalLight * totalLight * totalLight;\n"sv;
#else
    // Alternate night light blend function; much sharper falloff near terminator.
    return "totalLight = clamp(10.0 * (0.1 - totalLight), 0.0, 1.0);\n"sv;
#endif
}


// Return true if the color sum from all light sources should be computed in
// the vertex shader, and false if it will be done by the pixel shader.
bool
VSComputesColorSum(const ShaderProperties& props)
{
    return !props.usesShadows() && (props.lightModel & ShaderProperties::PerPixelSpecularModel) == 0;
}


std::string
AssignDiffuse(unsigned int lightIndex, const ShaderProperties& props)
{
    if (VSComputesColorSum(props))
        return fmt::format("diff.rgb += {} * ", LightProperty(lightIndex, "diffuse"));
    else
        return fmt::format(" = {}", SeparateDiffuse(lightIndex));
}


// Values used in generated shaders:
//    N - surface normal
//    V - view vector: the normalized direction from vertex to eye
//    L - light direction: normalized direction from vertex to light
//    H - half vector for Blinn-Phong lighting: normalized, bisects L and V
//    R - reflected light direction
//    NL - dot product of light and normal vectors
//    NV - dot product of light and view vectors
//    NH - dot product of light and half vectors

std::string
AddDirectionalLightContrib(unsigned int i, const ShaderProperties& props)
{
    std::string source;

    if (props.lightModel == ShaderProperties::ParticleDiffuseModel)
    {
        // The ParticleDiffuse model doesn't use a surface normal; vertices
        // are lit as if they were infinitesimal spherical particles,
        // unaffected by light direction except when considering shadows.
        source += assign("NL", 1.0f);
    }
    else
    {
        source += assign("NL", max(0.0f, dot(sh_vec3("in_Normal"), sh_vec3(LightProperty(i, "direction")))));
    }

    if (props.lightModel == ShaderProperties::SpecularModel)
    {
        source += assign("H", normalize(sh_vec3(LightProperty(i, "direction")) + normalize(sh_vec3("eyePosition") - sh_vec3("in_Position.xyz"))));
        source += assign("NH", max(0.0f, dot(sh_vec3("in_Normal"), sh_vec3("H"))));

        // The calculation below uses the infinite viewer approximation. It's slightly faster,
        // but results in less accurate rendering.
        // source += "NH = max(0.0, dot(in_Normal, " + LightProperty(i, "halfVector") + "));\n";
    }

    if (props.usesTangentSpaceLighting())
    {
        source += TangentSpaceTransform(LightDir_tan(i), LightProperty(i, "direction"));
        // Diffuse color is computed in the fragment shader
    }
    else if ((props.lightModel & ShaderProperties::PerPixelSpecularModel) != 0)
    {
        if ((props.lightModel & ShaderProperties::LunarLambertModel) != 0)
            source += AssignDiffuse(i, props) + " mix(NL, NL / (max(NV, 0.001) + NL), lunarLambert);\n";
        else
            source += SeparateDiffuse(i) + " = NL;\n";
    }
#if 0
    else if (props.lightModel == ShaderProperties::OrenNayarModel)
    {
        source += "float cosAlpha = min(NV, NL);\n";
        source += "float cosBeta = max(NV, NL);\n";
        source += "float sinAlpha = sqrt(1.0 - cosAlpha * cosAlpha);\n";
        source += "float sinBeta = sqrt(1.0 - cosBeta * cosBeta);\n";
        source += "float tanBeta = sinBeta / cosBeta;\n";
        source += "float cosAzimuth = dot(normalize(eye - in_Normal * NV), normalize(light - in_Normal * NL));\n";
        // TODO: precalculate these constants; place them in uniform values
        source += "float roughness2 = 0.7 * 0.7;\n";
        source += "float A = 1.0f - (0.5f * roughness2) / (roughness2 + 0.33);\n";
        source += "float B = (0.45f * roughness2) / (roughness2 + 0.09);\n";
        // TODO: add normalization factor so that max brightness is always 1
        // TODO: add gamma correction
        source += "float d = NL * (A + B * sinAlpha * tanBeta * max(0.0, cosAzimuth));\n";
        if (props.usesShadows())
        {
            source += SeparateDiffuse(i) += " = d;\n";
        }
        else
        {
            source += "diff.rgb += " + LightProperty(i, "diffuse") + " * d;\n";
        }
    }
#endif
    else if ((props.lightModel & ShaderProperties::LunarLambertModel) != 0)
    {
        source += AssignDiffuse(i, props) + " mix(NL, NL / (max(NV, 0.001) + NL), lunarLambert);\n";
    }
    else if (props.usesShadows())
    {
        // When there are shadows, we need to track the diffuse contributions
        // separately for each light.
        source += SeparateDiffuse(i) + " = NL;\n";
        if (props.hasSpecular())
        {
            source += SeparateSpecular(i) + " = pow(NH, shininess);\n";
        }
    }
    else
    {
        source += "diff.rgb += " + LightProperty(i, "diffuse") + " * NL;\n";
        if (props.hasSpecular())
        {
            source += "spec.rgb += " + LightProperty(i, "specular") +
                " * (pow(NH, shininess) * NL);\n";
        }
    }

    if (((props.texUsage & ShaderProperties::NightTexture) != 0) && VSComputesColorSum(props))
    {
        source += "totalLight += NL * " + LightProperty(i, "brightness") + ";\n";
    }

    return source;
}


std::string
BeginLightSourceShadows(const ShaderProperties& props, unsigned int light)
{
    std::string source;

    if (props.usesTangentSpaceLighting())
    {
        if (props.hasShadowsForLight(light))
            source += assign("shadow", 1.0f);
    }
    else
    {
        source += assign("shadow", sh_float(SeparateDiffuse(light)));
    }

    if (props.hasRingShadowForLight(light))
    {
        if (light == 0)
            source += DeclareLocal("ringShadowTexCoordX", Shader_Float);
        source += assign("ringShadowTexCoordX", ringShadowTexCoord(light));

#ifdef GL_ES
        if (!gl::OES_texture_border_clamp)
            source += "if (ringShadowTexCoordX >= 0.0 && ringShadowTexCoordX <= 1.0)\n{\n";
#endif
        if (gl::ARB_shader_texture_lod)
        {
            source += mulAssign("shadow",
                      (1.0f - texture2DLod(sampler2D("ringTex"), vec2(sh_float("ringShadowTexCoordX"), 0.0f), indexedUniform("ringShadowLOD", light))["a"]));
        }
        else
        {
            // Fallback when the texture2Dlod function is unavailable. This would be a good option
            // for all GPUs except that some (GeForce 8 series, possibly others) have trouble with the
            // LOD bias. It seems that the LOD bias isn't actually implemented in hardware, and is
            // instead implemented by emitting shader instructions to compute the texture LOD using
            // the derivative instructions and adding the bias to this result. Unfortunately, the
            // derivative is computed from the plane equation of the triangle, which means that there
            // are discontinuities between triangles.
            source += mulAssign("shadow",
                      (1.0f - texture2DLodBias(sampler2D("ringTex"), vec2(sh_float("ringShadowTexCoordX"), 0.0f), indexedUniform("ringShadowLOD", light))["a"]));
        }
#ifdef GL_ES
        if (!gl::OES_texture_border_clamp)
            source += "}\n";
#endif
    }

    if (props.hasCloudShadowForLight(light))
    {
        source += mulAssign("shadow", 1.0f - texture2D(sampler2D("cloudShadowTex"), cloudShadowTexCoord(light))["a"] * 0.75f);
    }

    return source;
}


// Calculate the depth of an eclipse shadow at the current fragment. Eclipse
// shadows are circular, decreasing in depth from maxDepth at the center to
// zero at the edge of the penumbra.
// Eclipse shadows are approximate. They assume that the both the sun and
// occluding body are spherical. An oblate planet is treated as if its polar
// radius were equal to its equatorial radius. This produces quite accurate
// eclipses for major moons around giant planets, which orbit close to the
// equatorial plane of the planet.
//
// The radius of the shadow umbra and penumbra are computed accurately
// (to the limit of the spherical approximation.) The maximum shadow depth
// is also calculated accurately. However, the shadow falloff from from
// the umbra to the edge of the penumbra is approximated as linear.
std::string
Shadow(unsigned int light, unsigned int shadow)
{
    std::string source;

    source += "shadowCenter.s = dot(vec4(position_obj, 1.0), " +
        IndexedParameter("shadowTexGenS", light, shadow) + ") - 0.5;\n";
    source += "shadowCenter.t = dot(vec4(position_obj, 1.0), " +
        IndexedParameter("shadowTexGenT", light, shadow) + ") - 0.5;\n";

    // The shadow shadow consists of a circular region of constant depth (maxDepth),
    // surrounded by a ring of linear falloff from maxDepth to zero. For a total
    // eclipse, maxDepth is zero. In reality, the falloff function is much more complex:
    // to calculate the exact amount of sunlight blocked, we need to calculate the
    // a circle-circle intersection area.
    // (See http://mathworld.wolfram.com/Circle-CircleIntersection.html)

    // The code generated below will compute:
    // r = 2 * sqrt(dot(shadowCenter, shadowCenter));
    // shadowR = clamp((r - 1) * shadowFalloff, 0, shadowMaxDepth)
    source += "shadowR = clamp((2.0 * sqrt(dot(shadowCenter, shadowCenter)) - 1.0) * " +
        IndexedParameter("shadowFalloff", light, shadow) + ", 0.0, " +
        IndexedParameter("shadowMaxDepth", light, shadow) + ");\n";

    source += "shadow *= 1.0 - shadowR;\n";

    return source;
}


std::string
ShadowsForLightSource(const ShaderProperties& props, unsigned int light)
{
    std::string source = BeginLightSourceShadows(props, light);

    for (unsigned int i = 0; i < props.getEclipseShadowCountForLight(light); i++)
        source += Shadow(light, i);

    return source;
}


std::string
ScatteringPhaseFunctions(const ShaderProperties& /*unused*/)
{
    std::string source;

    // Evaluate the Mie and Rayleigh phase functions; both are functions of the cosine
    // of the angle between the view vector and light vector
    source += "    float phMie = (1.0 - mieK * mieK) / ((1.0 - mieK * cosTheta) * (1.0 - mieK * cosTheta));\n";

    // Ignore Rayleigh phase function and treat Rayleigh scattering as isotropic
    // source += "    float phRayleigh = (1.0 + cosTheta * cosTheta);\n";
    source += "    float phRayleigh = 1.0;\n";

    return source;
}


std::string
AtmosphericEffects(const ShaderProperties& props)
{
    std::string source;

    source += "{\n";
    // Compute the intersection of the view direction and the cloud layer (currently assumed to be a sphere)
    source += "    float rq = dot(eyePosition, eyeDir);\n";
    source += "    float qq = dot(eyePosition, eyePosition) - atmosphereRadius.y;\n";
    source += "    float d = sqrt(max(rq * rq - qq, 0.0));\n";
    source += "    vec3 atmEnter = eyePosition + min(0.0, (-rq + d)) * eyeDir;\n";
    source += "    vec3 atmLeave = in_Position.xyz;\n";

    source += "    vec3 atmSamplePoint = (atmEnter + atmLeave) * 0.5;\n";
    //source += "    vec3 atmSamplePoint = atmEnter * 0.2 + atmLeave * 0.8;\n";

    // Compute the distance through the atmosphere from the sample point to the sun
    source += "    vec3 atmSamplePointSun = atmEnter * 0.5 + atmLeave * 0.5;\n";
    source += "    rq = dot(atmSamplePointSun, " + LightProperty(0, "direction") + ");\n";
    source += "    qq = dot(atmSamplePointSun, atmSamplePointSun) - atmosphereRadius.y;\n";
    source += "    d = sqrt(max(rq * rq - qq, 0.0));\n";
    source += "    float distSun = -rq + d;\n";
    source += "    float distAtm = length(atmEnter - atmLeave);\n";

    // Compute the density of the atmosphere at the sample point; it falls off exponentially
    // with the height above the planet's surface.
#if 0
    source += "    float h = max(0.0, length(atmSamplePoint) - atmosphereRadius.z);\n";
    source += "    float density = exp(-h * mieH);\n";
#else
    source += "    float density = 0.0;\n";
    source += "    atmSamplePoint = atmEnter * 0.333 + atmLeave * 0.667;\n";
    //source += "    atmSamplePoint = atmEnter * 0.1 + atmLeave * 0.9;\n";
    source += "    float h = max(0.0, length(atmSamplePoint) - atmosphereRadius.z);\n";
    source += "    density += exp(-h * mieH);\n";
    source += "    atmSamplePoint = atmEnter * 0.667 + atmLeave * 0.333;\n";
    //source += "    atmSamplePoint = atmEnter * 0.9 + atmLeave * 0.1;\n";
    source += "    h = max(0.0, length(atmSamplePoint) - atmosphereRadius.z);\n";
    source += "    density += exp(-h * mieH);\n";
#endif

    bool hasAbsorption = true;

    std::string scatter;
    if (hasAbsorption)
    {
        source += "    vec3 sunColor = exp(-extinctionCoeff * density * distSun);\n";
        source += "    vec3 ex = exp(-extinctionCoeff * density * distAtm);\n";

        scatter = "(1.0 - exp(-scatterCoeffSum * density * distAtm))";
    }
#if 0
    else
    {
        source += "    vec3 sunColor = exp(-scatterCoeffSum * density * distSun);\n";
        source += "    vec3 ex = exp(-scatterCoeffSum * density * distAtm);\n";

        // If there's no absorption, the extinction coefficients are just the scattering coefficients,
        // so there's no need to recompute the scattering.
        scatter = "(1.0 - ex)";
    }
#endif

    // If we're rendering the sky dome, compute the phase functions in the fragment shader
    // rather than the vertex shader in order to avoid artifacts from coarse tessellation.
    if (props.lightModel == ShaderProperties::AtmosphereModel)
    {
        source += "    scatterEx = ex;\n";
        source += "    " + ScatteredColor(0) + " = sunColor * " + scatter + ";\n";
    }
    else
    {
        source += "    float cosTheta = dot(eyeDir, " + LightProperty(0, "direction") + ");\n";
        source += ScatteringPhaseFunctions(props);

        source += "    scatterEx = ex;\n";

        source += "    scatterColor = (phRayleigh * rayleighCoeff + phMie * mieCoeff) * invScatterCoeffSum * sunColor * " + scatter + ";\n";
    }


    // Optional exposure control
    //source += "    1.0 - (scatterIn * exp(-5.0 * max(scatterIn.x, max(scatterIn.y, scatterIn.z))));\n";

    source += "}\n";

    return source;
}


#if 0
// Integrate the atmosphere by summation--slow, but higher quality
std::string
AtmosphericEffects(const ShaderProperties& props, unsigned int nSamples)
{
    std::string source;

    source += "{\n";
    // Compute the intersection of the view direction and the cloud layer (currently assumed to be a sphere)
    source += "    float rq = dot(eyePosition, eyeDir);\n";
    source += "    float qq = dot(eyePosition, eyePosition) - atmosphereRadius.y;\n";
    source += "    float d = sqrt(max(rq * rq - qq, 0.0));\n";
    source += "    vec3 atmEnter = eyePosition + min(0.0, (-rq + d)) * eyeDir;\n";
    source += "    vec3 atmLeave = in_Position.xyz;\n";

    source += "    vec3 step = (atmLeave - atmEnter) * (1.0 / 10.0);\n";
    source += "    float stepLength = length(step);\n";
    source += "    vec3 atmSamplePoint = atmEnter + step * 0.5;\n";
    source += "    vec3 scatter = vec3(0.0);\n";
    source += "    vec3 ex = vec3(1.0);\n";
    source += "    float tau = 0.0;\n";
    source += "    for (int i = 0; i < 10; ++i) {\n";

    // Compute the distance through the atmosphere from the sample point to the sun
    source += "        rq = dot(atmSamplePoint, " + LightProperty(0, "direction") + ");\n";
    source += "        qq = dot(atmSamplePoint, atmSamplePoint) - atmosphereRadius.y;\n";
    source += "        d = sqrt(max(rq * rq - qq, 0.0));\n";
    source += "        float distSun = -rq + d;\n";

    // Compute the density of the atmosphere at the sample point; it falls off exponentially
    // with the height above the planet's surface.
    source += "        float h = max(0.0, length(atmSamplePoint) - atmosphereRadius.z);\n";
    source += "        float d = exp(-h * mieH);\n";
    source += "        tau += d * stepLength;\n";
    source += "        vec3 sunColor = exp(-extinctionCoeff * d * distSun);\n";
    source += "        ex = exp(-extinctionCoeff * tau);\n";
    source += "        scatter += ex * sunColor * d * 0.1;\n";
    source += "        atmSamplePoint += step;\n";
    source += "    }\n";

    // If we're rendering the sky dome, compute the phase functions in the fragment shader
    // rather than the vertex shader in order to avoid artifacts from coarse tessellation.
    if (props.lightModel == ShaderProperties::AtmosphereModel)
    {
        source += "    scatterEx = ex;\n";
        source += "    " + ScatteredColor(i) + " = scatter;\n";
    }
    else
    {
        source += "    float cosTheta = dot(eyeDir, " + LightProperty(0, "direction") + ");\n";
        source += ScatteringPhaseFunctions(props);

        source += "    scatterEx = ex;\n";

        source += "    scatterColor = (phRayleigh * rayleighCoeff + phMie * mieCoeff) * invScatterCoeffSum * scatter;\n";
    }
    // Optional exposure control
    //source += "    1.0 - (scatterColor * exp(-5.0 * max(scatterIn.x, max(scatterIn.y, scatterIn.z))));\n";

    source += "}\n";

    return source;
}
#endif


std::string
ScatteringConstantDeclarations(const ShaderProperties& /*props*/)
{
    std::string source;

    source += DeclareUniform("atmosphereRadius", Shader_Vector3);
    source += DeclareUniform("mieCoeff", Shader_Float);
    source += DeclareUniform("mieH", Shader_Float);
    source += DeclareUniform("mieK", Shader_Float);
    source += DeclareUniform("rayleighCoeff", Shader_Vector3);
    source += DeclareUniform("rayleighH", Shader_Float);
    source += DeclareUniform("scatterCoeffSum", Shader_Vector3);
    source += DeclareUniform("invScatterCoeffSum", Shader_Vector3);
    source += DeclareUniform("extinctionCoeff", Shader_Vector3);

    return source;
}


std::string
TextureSamplerDeclarations(const ShaderProperties& props)
{
    std::string source;

    // Declare texture samplers
    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        source += DeclareUniform("diffTex", Shader_Sampler2D);
    }

    if (props.texUsage & ShaderProperties::NormalTexture)
    {
        source += DeclareUniform("normTex", Shader_Sampler2D);
    }

    if (props.texUsage & ShaderProperties::SpecularTexture)
    {
        source += DeclareUniform("specTex", Shader_Sampler2D);
    }

    if (props.texUsage & ShaderProperties::NightTexture)
    {
        source += DeclareUniform("nightTex", Shader_Sampler2D);
    }

    if (props.texUsage & ShaderProperties::EmissiveTexture)
    {
        source += DeclareUniform("emissiveTex", Shader_Sampler2D);
    }

    if (props.texUsage & ShaderProperties::OverlayTexture)
    {
        source += DeclareUniform("overlayTex", Shader_Sampler2D);
    }

    if (props.texUsage & ShaderProperties::ShadowMapTexture)
    {
#if GL_ONLY_SHADOWS
        source += DeclareUniform("shadowMapTex0", Shader_Sampler2DShadow);
#else
        source += DeclareUniform("shadowMapTex0", Shader_Sampler2D);
#endif
    }

    return source;
}


std::string
TextureCoordDeclarations(const ShaderProperties& props)
{
    std::string source;

    if (props.hasSharedTextureCoords())
    {
        // If the shared texture coords flag is set, use the diffuse texture
        // coordinate for sampling all the texture maps.
        if (props.texUsage & (ShaderProperties::DiffuseTexture  |
                              ShaderProperties::NormalTexture   |
                              ShaderProperties::SpecularTexture |
                              ShaderProperties::NightTexture    |
                              ShaderProperties::EmissiveTexture |
                              ShaderProperties::OverlayTexture))
        {
            source += DeclareVarying("diffTexCoord", Shader_Vector2);
        }
    }
    else
    {
        if (props.texUsage & ShaderProperties::DiffuseTexture)
            source += DeclareVarying("diffTexCoord", Shader_Vector2);
        if (props.texUsage & ShaderProperties::NormalTexture)
            source += DeclareVarying("normTexCoord", Shader_Vector2);
        if (props.texUsage & ShaderProperties::SpecularTexture)
            source += DeclareVarying("specTexCoord", Shader_Vector2);
        if (props.texUsage & ShaderProperties::NightTexture)
            source += DeclareVarying("nightTexCoord", Shader_Vector2);
        if (props.texUsage & ShaderProperties::EmissiveTexture)
            source += DeclareVarying("emissiveTexCoord", Shader_Vector2);
        if (props.texUsage & ShaderProperties::OverlayTexture)
            source += DeclareVarying("overlayTexCoord", Shader_Vector2);
    }

    if (props.texUsage & ShaderProperties::ShadowMapTexture)
    {
        source += DeclareVarying("shadowTexCoord0", Shader_Vector4);
        source += DeclareVarying("cosNormalLightDir", Shader_Float);
    }

    return source;
}

std::string
PointSizeDeclaration()
{
    std::string source;
    source += DeclareVarying("pointFade", Shader_Float);
    source += DeclareUniform("pointScale", Shader_Float);
    source += DeclareAttribute("in_PointSize", Shader_Float);
    return source;
}

std::string
PointSizeCalculation()
{
    std::string source;
    source += "float ptSize = pointScale * in_PointSize / length(vec3(ModelViewMatrix * in_Position));\n";
    source += "pointFade = min(1.0, ptSize * ptSize);\n";
    source += "gl_PointSize = ptSize;\n";

    return source;
}

std::string
StaticPointSize()
{
    std::string source;
    source += "pointFade = 1.0;\n";
    source += "gl_PointSize = in_PointSize * pointScale;\n";
    return source;
}

static std::string
LineDeclaration()
{
    std::string source;
    source += DeclareAttribute("in_PositionNext", Shader_Vector4);
    source += DeclareAttribute("in_ScaleFactor", Shader_Float);
    source += DeclareUniform("lineWidthX", Shader_Float);
    source += DeclareUniform("lineWidthY", Shader_Float);
    return source;
}

std::string
CalculateShadow()
{
    std::string source;
#if GL_ONLY_SHADOWS
    source += R"glsl(
float calculateShadow()
{
    float texelSize = 1.0 / shadowMapSize;
    float s = 0.0;
    float bias = max(0.005 * (1.0 - cosNormalLightDir), 0.0005);
)glsl"sv;
    float boxFilterWidth = (float) ShadowSampleKernelWidth - 1.0f;
    float firstSample = -boxFilterWidth / 2.0f;
    float lastSample = firstSample + boxFilterWidth;
    float sampleWeight = 1.0f / (float) (ShadowSampleKernelWidth * ShadowSampleKernelWidth);
    source += fmt::format("    for (float y = {:f}; y <= {:f}; y += 1.0)\n", firstSample, lastSample);
    source += fmt::format("        for (float x = {:f}; x <= {:f}; x += 1.0)\n", firstSample, lastSample);
    source += "            s += shadow2D(shadowMapTex0, shadowTexCoord0.xyz + vec3(x * texelSize, y * texelSize, bias)).z;\n";
    source += fmt::format("    return s * {:f};\n", sampleWeight);
    source += "}\n";
#else
    source += R"glsl(
float calculateShadow()
{
    float texelSize = 1.0 / shadowMapSize;
    float s = 0.0;
    float bias = max(0.005 * (1.0 - cosNormalLightDir), 0.0005);
    for(float x = -1.0; x <= 1.0; x += 1.0)
    {
        for(float y = -1.0; y <= 1.0; y += 1.0)
        {
            float pcfDepth = texture2D(shadowMapTex0, shadowTexCoord0.xy + vec2(x * texelSize, y * texelSize)).r;
            s += shadowTexCoord0.z - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    return 1.0 - s / 9.0;
}
)glsl"sv;
#endif
    return source;
}

void
BindAttribLocations(const GLProgram *prog)
{
    glBindAttribLocation(prog->getID(), CelestiaGLProgram::VertexCoordAttributeIndex,   "in_Position");
    glBindAttribLocation(prog->getID(), CelestiaGLProgram::NormalAttributeIndex,        "in_Normal");
    glBindAttribLocation(prog->getID(), CelestiaGLProgram::TextureCoord0AttributeIndex, "in_TexCoord0");
    glBindAttribLocation(prog->getID(), CelestiaGLProgram::TextureCoord1AttributeIndex, "in_TexCoord1");
    glBindAttribLocation(prog->getID(), CelestiaGLProgram::TextureCoord2AttributeIndex, "in_TexCoord2");
    glBindAttribLocation(prog->getID(), CelestiaGLProgram::TextureCoord3AttributeIndex, "in_TexCoord3");
    glBindAttribLocation(prog->getID(), CelestiaGLProgram::ColorAttributeIndex,         "in_Color");
    glBindAttribLocation(prog->getID(), CelestiaGLProgram::IntensityAttributeIndex,     "in_Intensity");
    glBindAttribLocation(prog->getID(), CelestiaGLProgram::NextVCoordAttributeIndex,    "in_PositionNext");
    glBindAttribLocation(prog->getID(), CelestiaGLProgram::ScaleFactorAttributeIndex,   "in_ScaleFactor");
    glBindAttribLocation(prog->getID(), CelestiaGLProgram::TangentAttributeIndex,       "in_Tangent");
    glBindAttribLocation(prog->getID(), CelestiaGLProgram::PointSizeAttributeIndex,     "in_PointSize");
}

std::optional<std::string>
ReadShaderFile(const fs::path &path)
{
    std::error_code ec;
    std::uintmax_t size = fs::file_size(path, ec);
    if (ec)
    {
        GetLogger()->error("Failed to get file size of {}.\n", path);
        return {};
    }

    std::ifstream in(path);
    if (!in.good())
    {
        GetLogger()->error("Failed to open {}.\n", path);
        return {};
    }

    std::string s(size, '\0');
    in.read(&s[0], size); /* Flawfinder: ignore */

    if (!in.good() && !in.eof())
        return {};

    return s;
}

GLShaderStatus
CreateErrorShader(GLProgram **prog, bool fisheyeEnabled)
{
    std::string _vs = fmt::format("{}{}{}{}{}\n", VersionHeader, CommonHeader, VertexHeader, VPFunction(fisheyeEnabled), errorVertexShaderSource);
    std::string _fs = fmt::format("{}{}{}{}\n", VersionHeader, CommonHeader, FragmentHeader, errorFragmentShaderSource);

    auto status = GLShaderLoader::CreateProgram(_vs, _fs, prog);
    if (status == ShaderStatus_OK)
    {
        BindAttribLocations(*prog);
        status = (*prog)->link();
    }

    if (status != ShaderStatus_OK)
    {
        if (g_shaderLogFile != nullptr)
            *g_shaderLogFile << "Failed to create error shader!\n";

        return ShaderStatus_LinkError;
    }

    return ShaderStatus_OK;
}

constexpr std::string_view
InPrimitive(GLint prim)
{
    switch (prim)
    {
    case GL_POINTS:
        return "points"sv;
    case GL_LINES:
        return "lines"sv;
    case GL_LINES_ADJACENCY:
        return "lines_adjacency"sv;
    case GL_TRIANGLES:
        return "triangles"sv;
    case GL_TRIANGLES_ADJACENCY:
        return "triangles_adjacency"sv;
    default:
        return "invalid"sv;
    }
}

constexpr std::string_view
OutPrimitive(GLint prim)
{
    switch (prim)
    {
    case GL_POINTS:
        return "points"sv;
    case GL_LINE_STRIP:
        return "line_strip"sv;
    case GL_TRIANGLE_STRIP:
        return "triangle_strip"sv;
    default:
        return "invalid"sv;
    }
}

} // end unnamed namespace

bool
ShaderProperties::usesShadows() const
{
    return shadowCounts != 0;
}


bool
ShaderProperties::usesFragmentLighting() const
{
    return (texUsage & NormalTexture) != 0 || (lightModel & PerPixelSpecularModel) != 0;
}


unsigned int
ShaderProperties::getEclipseShadowCountForLight(unsigned int lightIndex) const
{
    return (shadowCounts >> lightIndex * ShadowBitsPerLight) & EclipseShadowMask;
}


bool
ShaderProperties::hasEclipseShadows() const
{
    return (shadowCounts & AnyEclipseShadowMask) != 0;
}


void
ShaderProperties::setEclipseShadowCountForLight(unsigned int lightIndex, unsigned int shadowCount)
{
    assert(shadowCount <= MaxShaderEclipseShadows);
    assert(lightIndex < MaxShaderLights);
    if (shadowCount <= MaxShaderEclipseShadows && lightIndex < MaxShaderLights)
    {
        shadowCounts &= ~(EclipseShadowMask << lightIndex * ShadowBitsPerLight);
        shadowCounts |= shadowCount << lightIndex * ShadowBitsPerLight;
    }
}


bool
ShaderProperties::hasRingShadowForLight(unsigned int lightIndex) const
{
    return ((shadowCounts >> lightIndex * ShadowBitsPerLight) & RingShadowMask) != 0;
}


bool
ShaderProperties::hasRingShadows() const
{
    return (shadowCounts & AnyRingShadowMask) != 0;
}


void
ShaderProperties::setRingShadowForLight(unsigned int lightIndex, bool enabled)
{
    assert(lightIndex < MaxShaderLights);
    if (lightIndex < MaxShaderLights)
    {
        shadowCounts &= ~(RingShadowMask << lightIndex * ShadowBitsPerLight);
        shadowCounts |= (enabled ? RingShadowMask : 0) << lightIndex * ShadowBitsPerLight;
    }
}


bool
ShaderProperties::hasSelfShadowForLight(unsigned int lightIndex) const
{
    return ((shadowCounts >> lightIndex * ShadowBitsPerLight) & SelfShadowMask) != 0;
}


bool
ShaderProperties::hasSelfShadows() const
{
    return (shadowCounts & AnySelfShadowMask) != 0;
}


void
ShaderProperties::setSelfShadowForLight(unsigned int lightIndex, bool enabled)
{
    assert(lightIndex < MaxShaderLights);
    if (lightIndex < MaxShaderLights)
    {
        shadowCounts &= ~(SelfShadowMask << lightIndex * ShadowBitsPerLight);
        shadowCounts |= (enabled ? SelfShadowMask : 0) << lightIndex * ShadowBitsPerLight;
    }
}


bool
ShaderProperties::hasCloudShadowForLight(unsigned int lightIndex) const
{
    return ((shadowCounts >> lightIndex * ShadowBitsPerLight) & CloudShadowMask) != 0;
}


bool
ShaderProperties::hasCloudShadows() const
{
    return (shadowCounts & AnyCloudShadowMask) != 0;
}


bool
ShaderProperties::hasShadowMap() const
{
    return (texUsage & ShadowMapTexture) != 0;
}

void
ShaderProperties::setCloudShadowForLight(unsigned int lightIndex, bool enabled)
{
    assert(lightIndex < MaxShaderLights);
    if (lightIndex < MaxShaderLights)
    {
        shadowCounts &= ~(SelfShadowMask << lightIndex * ShadowBitsPerLight);
        shadowCounts |= (enabled ? CloudShadowMask : 0) << lightIndex * ShadowBitsPerLight;
    }
}


bool
ShaderProperties::hasShadowsForLight(unsigned int light) const
{
    assert(light < MaxShaderLights);
    return getEclipseShadowCountForLight(light) != 0 ||
           hasRingShadowForLight(light)    ||
           hasSelfShadowForLight(light)    ||
           hasCloudShadowForLight(light);
}


// Returns true if diffuse, specular, bump, and night textures all use the
// same texture coordinate set.
bool
ShaderProperties::hasSharedTextureCoords() const
{
    return (texUsage & SharedTextureCoords) != 0;
}


bool
ShaderProperties::hasSpecular() const
{
    return lightModel == SpecularModel || (lightModel & PerPixelSpecularModel) != 0;
}


bool
ShaderProperties::hasScattering() const
{
    return (texUsage & Scattering) != 0;
}


bool
ShaderProperties::isViewDependent() const
{
    switch (lightModel)
    {
    case DiffuseModel:
    case ParticleDiffuseModel:
    case EmissiveModel:
    case UnlitModel:
        return false;
    default:
        return true;
    }
}


bool
ShaderProperties::usesTangentSpaceLighting() const
{
    return (texUsage & NormalTexture) != 0;
}


bool ShaderProperties::usePointSize() const
{
    return (texUsage & (PointSprite | StaticPointSize)) != 0;
}


bool operator<(const ShaderProperties& p0, const ShaderProperties& p1)
{
    return std::tie(p0.texUsage, p0.nLights, p0.shadowCounts, p0.effects, p0.fishEyeOverride, p0.lightModel)
         < std::tie(p1.texUsage, p1.nLights, p1.shadowCounts, p1.effects, p1.fishEyeOverride, p1.lightModel);
}


ShaderManager::ShaderManager()
{
#if defined(_DEBUG) || defined(DEBUG) || 1
    // Only write to shader log file if this is a debug build

    if (g_shaderLogFile == nullptr)
#ifdef _WIN32
        g_shaderLogFile = new std::ofstream("shaders.log");
#else
        g_shaderLogFile = new std::ofstream("/tmp/celestia-shaders.log");
#endif

#endif
}


ShaderManager::~ShaderManager()
{
    for(const auto& shader : dynamicShaders)
        delete shader.second;

    dynamicShaders.clear();

    for(const auto& shader : staticShaders)
        delete shader.second;

    staticShaders.clear();
}

CelestiaGLProgram*
ShaderManager::getShader(const ShaderProperties& props)
{
    auto iter = dynamicShaders.find(props);
    if (iter != dynamicShaders.end())
    {
        // Shader already exists
        return iter->second;
    }
    else
    {
        // Create a new shader and add it to the table of created shaders
        CelestiaGLProgram* prog = buildProgram(props);
        dynamicShaders[props] = prog;

        return prog;
    }
}

CelestiaGLProgram*
ShaderManager::getShader(std::string_view name, std::string_view vs, std::string_view fs)
{
    if (auto iter = staticShaders.find(name); iter != staticShaders.end())
    {
        // Shader already exists
        return iter->second;
    }

    // Create a new shader and add it to the table of created shaders
    auto *prog = buildProgram(vs, fs);
    staticShaders[name] = prog;

    return prog;
}

CelestiaGLProgram*
ShaderManager::getShader(std::string_view name)
{
    if (auto iter = staticShaders.find(name); iter != staticShaders.end())
    {
        // Shader already exists
        return iter->second;
    }

    fs::path dir("shaders");
    auto vsName = dir / fmt::format("{}_vert.glsl", name);
    auto fsName = dir / fmt::format("{}_frag.glsl", name);

    auto vs = ReadShaderFile(vsName);
    if (!vs.has_value())
        return getShader(name, errorVertexShaderSource, errorFragmentShaderSource);
    auto fs = ReadShaderFile(fsName);
    if (!fs.has_value())
        return getShader(name, errorVertexShaderSource, errorFragmentShaderSource);

    // Create a new shader and add it to the table of created shaders
    auto *prog = buildProgram(*vs, *fs);
    staticShaders[name] = prog;

    return prog;
}

CelestiaGLProgram*
ShaderManager::getShaderGL3(std::string_view name, const GeomShaderParams* params)
{
    if (auto iter = staticShaders.find(name); iter != staticShaders.end())
    {
        // Shader already exists
        return iter->second;
    }

    fs::path dir("shaders");
    auto vsName = dir / fmt::format("{}_vert.glsl", name);
    auto fsName = dir / fmt::format("{}_frag.glsl", name);

    auto vs = ReadShaderFile(vsName);
    if (!vs.has_value())
        return getShader(name, errorVertexShaderSource, errorFragmentShaderSource);
    auto fs = ReadShaderFile(fsName);
    if (!fs.has_value())
        return getShader(name, errorVertexShaderSource, errorFragmentShaderSource);

    CelestiaGLProgram *prog = nullptr;

    // Geometric shader is optional
    if (auto gsName = dir / fmt::format("{}_geom.glsl", name); fs::exists(gsName))
    {
        if (auto gs = ReadShaderFile(gsName); gs.has_value())
            prog = buildProgramGL3(*vs, *gs, *fs, params);
        else
            return getShader(name, errorVertexShaderSource, errorFragmentShaderSource);
    }
    else
    {
        prog = buildProgramGL3(*vs, *fs);
    }

    // Create a new shader and add it to the table of created shaders
    staticShaders[name] = prog;

    return prog;
}


CelestiaGLProgram*
ShaderManager::getShaderGL3(std::string_view name, std::string_view vs, std::string_view gs, std::string_view fs)
{
    if (auto iter = staticShaders.find(name); iter != staticShaders.end())
    {
        // Shader already exists
        return iter->second;
    }

    // Create a new shader and add it to the table of created shaders
    auto *prog = buildProgramGL3(vs, gs, fs);
    staticShaders[name] = prog;

    return prog;
}


GLVertexShader*
ShaderManager::buildVertexShader(const ShaderProperties& props)
{
    std::string source(VersionHeader);
    source += CommonHeader;
    source += VertexHeader;
    source += CommonAttribs;

    source += DeclareLights(props);
    if (props.lightModel == ShaderProperties::SpecularModel)
        source += DeclareUniform("shininess", Shader_Float);

    source += DeclareUniform("eyePosition", Shader_Vector3);

    source += TextureCoordDeclarations(props);
    source += DeclareUniform("textureOffset", Shader_Float);

    if (props.hasScattering())
    {
        source += ScatteringConstantDeclarations(props);
    }

    if (props.usePointSize())
        source += PointSizeDeclaration();

    if (props.usesTangentSpaceLighting())
    {
        source += DeclareAttribute("in_Tangent", Shader_Vector3);
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            source += DeclareVarying(LightDir_tan(i), Shader_Vector3);
        }

        if (props.isViewDependent() &&
            props.lightModel != ShaderProperties::SpecularModel)
        {
            source += DeclareVarying("eyeDir_tan", Shader_Vector3);
        }
    }
    else if ((props.lightModel & ShaderProperties::PerPixelSpecularModel) != 0)
    {
        source += DeclareVarying("diffFactors", Shader_Vector4);
        source += DeclareVarying("normal", Shader_Vector3);
    }
    else if (props.usesShadows())
    {
        source += DeclareVarying("diffFactors", Shader_Vector4);
        if (props.lightModel == ShaderProperties::SpecularModel)
        {
            source += DeclareVarying("specFactors", Shader_Vector4);
        }
    }
    else
    {
        if (props.lightModel != ShaderProperties::UnlitModel)
        {
            source += DeclareUniform("ambientColor", Shader_Vector3);
            source += DeclareUniform("opacity", Shader_Float);
        }
        source += DeclareVarying("diff", Shader_Vector4);
        if (props.lightModel == ShaderProperties::SpecularModel)
        {
            source += DeclareVarying("spec", Shader_Vector4);
        }
    }

    // If this shader uses tangent space lighting, the diffuse term
    // will be calculated in the fragment shader and we won't need
    // the lunar-Lambert term here in the vertex shader.
    if (!props.usesTangentSpaceLighting())
    {
        if ((props.lightModel & ShaderProperties::LunarLambertModel) != 0)
            source += DeclareUniform("lunarLambert", Shader_Float);
    }

    // Miscellaneous lighting values
    if ((props.texUsage & ShaderProperties::NightTexture) && VSComputesColorSum(props))
    {
        source += DeclareVarying("totalLight", Shader_Float);
    }

    if (props.hasScattering())
    {
        //source += DeclareVarying("scatterIn", Shader_Vector3);
        source += DeclareVarying("scatterEx", Shader_Vector3);
        source += DeclareVarying("scatterColor", Shader_Vector3);
    }

    // Shadow parameters
    if (props.hasEclipseShadows())
    {
        source += DeclareVarying("position_obj", Shader_Vector3);
    }

    if (props.hasRingShadows())
    {
        source += DeclareUniform("ringWidth", Shader_Float);
        source += DeclareUniform("ringRadius", Shader_Float);
        source += DeclareUniform("ringPlane", Shader_Vector4);
        source += DeclareUniform("ringCenter", Shader_Vector3);
        source += DeclareVarying("ringShadowTexCoord", Shader_Vector4);
    }

    if (props.hasCloudShadows())
    {
        source += DeclareUniform("cloudShadowTexOffset", Shader_Float);
        source += DeclareUniform("cloudHeight", Shader_Float);
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            if (props.hasCloudShadowForLight(i))
            {
                source += DeclareVarying(CloudShadowTexCoord(i), Shader_Vector2);
            }
        }
    }

    if (props.hasShadowMap())
        source += DeclareUniform("ShadowMatrix0", Shader_Matrix4);

    if (props.texUsage & ShaderProperties::LineAsTriangles)
        source += LineDeclaration();

    source += VPFunction(props.fishEyeOverride != ShaderProperties::FisheyeOverrideModeDisabled && fisheyeEnabled);

    // Begin main() function
    source += "\nvoid main(void)\n{\n";
    if (props.isViewDependent() || props.hasScattering())
    {
        source += "vec3 eyeDir = normalize(eyePosition - in_Position.xyz);\n";
        if (!props.usesTangentSpaceLighting())
        {
            source += "float NV = dot(in_Normal, eyeDir);\n";
        }
    }
    else if (props.lightModel == ShaderProperties::SpecularModel)
    {
        source += "vec3 eyeDir = normalize(eyePosition - in_Position.xyz);\n";
    }

    source += "float NL;\n";
    if (props.lightModel == ShaderProperties::SpecularModel)
    {
        source += "float NH;\n";
        source += "vec3 H;\n";
    }

    if ((props.texUsage & ShaderProperties::NightTexture) && VSComputesColorSum(props))
    {
        source += "totalLight = 0.0;\n";
    }

    if (props.usesTangentSpaceLighting())
    {
        source += "vec3 bitangent = cross(in_Normal, in_Tangent);\n";
        if (props.isViewDependent() &&
            props.lightModel != ShaderProperties::SpecularModel)
        {
            source += TangentSpaceTransform("eyeDir_tan", "eyeDir");
        }
    }
    else if ((props.lightModel & ShaderProperties::PerPixelSpecularModel) != 0)
    {
        source += "normal = in_Normal;\n";
    }
    else if (props.usesShadows())
    {
    }
    else
    {
        if (props.lightModel == ShaderProperties::UnlitModel)
        {
            if ((props.texUsage & ShaderProperties::VertexColors) != 0)
                source += "diff = in_Color;\n";
            else
                source += "diff = vec4(1.0);\n";
        }
        else
        {
            source += "diff = vec4(ambientColor, opacity);\n";
        }
        if (props.hasSpecular())
            source += "spec = vec4(0.0);\n";
    }

    if (props.hasShadowMap())
    {
        source += "cosNormalLightDir = dot(in_Normal, " + LightProperty(0, "direction") + ");\n";
    }

    for (unsigned int i = 0; i < props.nLights; i++)
    {
        source += AddDirectionalLightContrib(i, props);
    }

    if ((props.texUsage & ShaderProperties::NightTexture) && VSComputesColorSum(props))
    {
        source += NightTextureBlend();
    }

    unsigned int nTexCoords = 0;

    // Output the texture coordinates. Use just a single texture coordinate if all textures are mapped
    // identically. The texture offset is added for cloud maps; specular and night texture are not offset
    // because cloud layers never have these textures.
    if (props.hasSharedTextureCoords())
    {
        if (props.texUsage & (ShaderProperties::DiffuseTexture  |
                              ShaderProperties::NormalTexture   |
                              ShaderProperties::SpecularTexture |
                              ShaderProperties::NightTexture    |
                              ShaderProperties::EmissiveTexture |
                              ShaderProperties::OverlayTexture))
        {
            source += "diffTexCoord = " + TexCoord2D(nTexCoords) + ";\n";
            source += "diffTexCoord.x += textureOffset;\n";
        }
    }
    else
    {
        if (props.texUsage & ShaderProperties::DiffuseTexture)
        {
            source += "diffTexCoord = " + TexCoord2D(nTexCoords) + " + vec2(textureOffset, 0.0);\n";
            nTexCoords++;
        }

        if (!props.hasSharedTextureCoords())
        {
            if (props.texUsage & ShaderProperties::NormalTexture)
            {
                source += "normTexCoord = " + TexCoord2D(nTexCoords) + " + vec2(textureOffset, 0.0);\n";
                nTexCoords++;
            }

            if (props.texUsage & ShaderProperties::SpecularTexture)
            {
                source += "specTexCoord = " + TexCoord2D(nTexCoords) + ";\n";
                nTexCoords++;
            }

            if (props.texUsage & ShaderProperties::NightTexture)
            {
                source += "nightTexCoord = " + TexCoord2D(nTexCoords) + ";\n";
                nTexCoords++;
            }

            if (props.texUsage & ShaderProperties::EmissiveTexture)
            {
                source += "emissiveTexCoord = " + TexCoord2D(nTexCoords) + ";\n";
                nTexCoords++;
            }
        }
    }

    // Shadow texture coordinates are generated in the shader
    if (props.hasRingShadows())
    {
        source += "vec3 ringShadowProj;\n";
        source += "float t = -(dot(in_Position.xyz, ringPlane.xyz) + ringPlane.w);\n";
        for (unsigned int j = 0; j < props.nLights; j++)
        {
            if (props.hasRingShadowForLight(j))
            {
                source += "ringShadowProj = in_Position.xyz + " +
                  LightProperty(j, "direction") +
                  " * max(0.0, t / dot(" +
                  LightProperty(j, "direction") + ", ringPlane.xyz));\n";

                source += RingShadowTexCoord(j) +
                  " = (length(ringShadowProj - ringCenter) - ringRadius) * ringWidth;\n";
            }
        }
    }

    if (props.hasCloudShadows())
    {
        for (unsigned int j = 0; j < props.nLights; j++)
        {
            if (props.hasCloudShadowForLight(j))
            {
                source += "{\n";

                // A cheap way to calculate cloud shadow texture coordinates that doesn't correctly account
                // for sun angle.
                source += "    " + CloudShadowTexCoord(j) + " = vec2(diffTexCoord.x + cloudShadowTexOffset, diffTexCoord.y);\n";

                // Disabled: there are too many problems with this approach,
                // though it should theoretically work. The inverse trig
                // approximations produced by the shader compiler are crude
                // enough that visual anomalies are apparent. And in the current
                // GeForce 8800 driver, this shader produces an internal compiler
                // error. Cloud shadows are trivial if the cloud texture is a cube
                // map. Also, DX10 capable hardware could efficiently perform
                // the rect-to-spherical conversion in the pixel shader with an
                // fp32 texture serving as a lookup table.
#if 0
                // Compute the intersection of the sun direction and the cloud layer (currently assumed to be a sphere)
                source += "    float rq = dot(" + LightProperty(j, "direction") + ", in_Position.xyz);\n";
                source += "    float qq = dot(in_Position.xyz, in_Position.xyz) - cloudHeight * cloudHeight;\n";
                source += "    float d = sqrt(max(rq * rq - qq, 0.0));\n";
                source += "    vec3 cloudSpherePos = (in_Position.xyz + (-rq + d) * " + LightProperty(j, "direction") + ");\n";
                //source += "    vec3 cloudSpherePos = in_Position.xyz;\n";

                // Find the texture coordinates at this point on the sphere by converting from rectangular to spherical; this is an
                // expensive calculation to perform per vertex.
                source += "    float invPi = 1.0 / 3.1415927;\n";
                source += "    " + CloudShadowTexCoord(j) + ".y = 0.5 - asin(cloudSpherePos.y) * invPi;\n";
                source += "    float u = fract(atan(cloudSpherePos.x, cloudSpherePos.z) * (invPi * 0.5) + 0.75);\n";
                source += "    if (diffTexCoord.x < 0.25 && u > 0.5) u -= 1.0;\n";
                source += "    else if (diffTexCoord.x > 0.75 && u < 0.5) u += 1.0;\n";
                source += "    " + CloudShadowTexCoord(j) + ".x = u + cloudShadowTexOffset;\n";
#endif

                source += "}\n";
            }
        }
    }

    if (props.hasScattering())
    {
        source += AtmosphericEffects(props);
    }

    if ((props.texUsage & ShaderProperties::OverlayTexture) && !props.hasSharedTextureCoords())
    {
        source += "overlayTexCoord = " + TexCoord2D(nTexCoords) + ";\n";
        nTexCoords++;
    }

    if (props.hasEclipseShadows())
    {
        source += "position_obj = in_Position.xyz;\n";
    }

    if (props.texUsage & ShaderProperties::PointSprite)
        source += PointSizeCalculation();
    else if (props.texUsage & ShaderProperties::StaticPointSize)
        source += StaticPointSize();

    if (props.hasShadowMap())
        source += "shadowTexCoord0 = ShadowMatrix0 * vec4(in_Position.xyz, 1.0);\n";

    source += VertexPosition(props);
    source += "}\n";

    DumpVSSource(source);

    GLVertexShader* vs = nullptr;
    GLShaderStatus status = GLShaderLoader::CreateVertexShader(source, &vs);
    return status == ShaderStatus_OK ? vs : nullptr;
}


GLFragmentShader*
ShaderManager::buildFragmentShader(const ShaderProperties& props)
{
    std::string source(VersionHeader);
    // Without GL_ARB_shader_texture_lod enabled one can use texture2DLod
    // in vertext shaders only
    if (gl::ARB_shader_texture_lod)
        source += "#extension GL_ARB_shader_texture_lod : enable\n";
    source += CommonHeader;

    std::string diffTexCoord("diffTexCoord");
    std::string specTexCoord("specTexCoord");
    std::string nightTexCoord("nightTexCoord");
    std::string emissiveTexCoord("emissiveTexCoord");
    std::string normTexCoord("normTexCoord");
    if (props.hasSharedTextureCoords())
    {
        specTexCoord     = diffTexCoord;
        nightTexCoord    = diffTexCoord;
        normTexCoord     = diffTexCoord;
        emissiveTexCoord = diffTexCoord;
    }

    source += TextureSamplerDeclarations(props);
    source += TextureCoordDeclarations(props);

    // Declare lighting parameters
    if (props.usesTangentSpaceLighting())
    {
        source += DeclareUniform("ambientColor", Shader_Vector3);
        source += DeclareUniform("opacity", Shader_Float);
        if (props.isViewDependent())
        {
            if (props.lightModel == ShaderProperties::SpecularModel)
            {
                // Specular model is sort of a hybrid: all the view-dependent lighting is
                // handled in the vertex shader, and the fragment shader is view-independent
                source += DeclareVarying("specFactors", Shader_Vector4);
                source += "vec4 spec = vec4(0.0);\n";
            }
            else
            {
                source += DeclareVarying("eyeDir_tan", Shader_Vector3);  // tangent space eye vector
                source += "vec4 spec = vec4(0.0);\n";
                source += DeclareUniform("shininess", Shader_Float);
            }
        }

        if ((props.lightModel & ShaderProperties::LunarLambertModel) != 0)
            source += DeclareUniform("lunarLambert", Shader_Float);

        for (unsigned int i = 0; i < props.nLights; i++)
        {
            source += DeclareVarying(LightDir_tan(i), Shader_Vector3);
            source += DeclareUniform(FragLightProperty(i, "color"), Shader_Vector3);
            if (props.hasSpecular())
            {
                source += DeclareUniform(FragLightProperty(i, "specColor"), Shader_Vector3);
            }
            if (props.texUsage & ShaderProperties::NightTexture)
            {
                source += DeclareUniform(FragLightProperty(i, "brightness"), Shader_Float);
            }
        }
    }
    else if ((props.lightModel & ShaderProperties::PerPixelSpecularModel) != 0)
    {
        source += DeclareUniform("ambientColor", Shader_Vector3);
        source += DeclareUniform("opacity", Shader_Float);
        source += DeclareVarying("diffFactors", Shader_Vector4);
        source += DeclareVarying("normal", Shader_Vector3);
        source += "vec4 spec = vec4(0.0);\n";
        source += DeclareUniform("shininess", Shader_Float);

        for (unsigned int i = 0; i < props.nLights; i++)
        {
            source += DeclareUniform(FragLightProperty(i, "color"), Shader_Vector3);
            source += DeclareUniform(FragLightProperty(i, "specColor"), Shader_Vector3);
        }
    }
    else if (props.usesShadows())
    {
        source += DeclareUniform("ambientColor", Shader_Vector3);
        source += DeclareUniform("opacity", Shader_Float);
        source += DeclareVarying("diffFactors", Shader_Vector4);
        if (props.lightModel == ShaderProperties::SpecularModel)
        {
            source += DeclareVarying("specFactors", Shader_Vector4);
            source += "vec4 spec = vec4(0.0);\n";
        }
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            source += DeclareUniform(FragLightProperty(i, "color"), Shader_Vector3);
            if (props.lightModel == ShaderProperties::SpecularModel)
                source += DeclareUniform(FragLightProperty(i, "specColor"), Shader_Vector3);
        }
    }
    else
    {
        source += DeclareVarying("diff", Shader_Vector4);
        if (props.lightModel == ShaderProperties::SpecularModel)
        {
            source += DeclareVarying("spec", Shader_Vector4);
        }
    }

    if (props.hasScattering())
    {
        //source += DeclareVarying("scatterIn", Shader_Vector3);
        source += DeclareVarying("scatterEx", Shader_Vector3);
        source += DeclareVarying("scatterColor", Shader_Vector3);
    }

    if ((props.texUsage & ShaderProperties::NightTexture))
    {
        if (VSComputesColorSum(props))
        {
            source += DeclareVarying("totalLight", Shader_Float);
        }
    }

    // Declare shadow parameters
    if (props.shadowCounts != 0)
    {
        source += DeclareVarying("position_obj", Shader_Vector3);
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            for (unsigned int j = 0; j < props.getEclipseShadowCountForLight(i); j++)
            {
                source += DeclareUniform(IndexedParameter("shadowTexGenS", i, j), Shader_Vector4);
                source += DeclareUniform(IndexedParameter("shadowTexGenT", i, j), Shader_Vector4);
                source += DeclareUniform(IndexedParameter("shadowFalloff", i, j), Shader_Float);
                source += DeclareUniform(IndexedParameter("shadowMaxDepth", i, j), Shader_Float);
            }
        }
    }

    if (props.hasRingShadows())
    {
        source += DeclareUniform("ringTex", Shader_Sampler2D);
        source += DeclareVarying("ringShadowTexCoord", Shader_Vector4);
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            if (props.hasRingShadowForLight(i))
            {
                source += DeclareUniform(IndexedParameter("ringShadowLOD", i), Shader_Float);
            }
        }
    }

    if (props.hasCloudShadows())
    {
        source += DeclareUniform("cloudShadowTex", Shader_Sampler2D);
        for (unsigned int i = 0; i < props.nLights; i++)
            source += DeclareVarying(CloudShadowTexCoord(i), Shader_Vector2);
    }

    if (props.usePointSize())
        source += DeclareVarying("pointFade", Shader_Float);

    if (props.hasShadowMap())
    {
        source += DeclareUniform("shadowMapSize", Shader_Float);
        source += CalculateShadow();
    }

    source += DeclareLights(props);

    source += "\nvoid main(void)\n{\n";
    source += "vec4 color;\n";
    if (props.usesTangentSpaceLighting() ||
        (props.lightModel & ShaderProperties::PerPixelSpecularModel) != 0 ||
        props.usesShadows())
    {
        source += "vec4 diff = vec4(ambientColor, opacity);\n";
    }

    if (props.usesShadows())
    {
        // Temporaries required for shadows
        source += "float shadow;\n";
        if (props.hasEclipseShadows())
        {
            source += "vec2 shadowCenter;\n";
            source += "float shadowR;\n";
        }
    }

    // Sum the illumination from each light source, computing a total diffuse and specular
    // contributions from all sources.
    if (props.usesTangentSpaceLighting())
    {
        // Get the normal in tangent space. Ordinarily it comes from the normal texture, but if one
        // isn't provided, we'll simulate a smooth surface by using a constant (in tangent space)
        // normal of [ 0 0 1 ]
        if (props.texUsage & ShaderProperties::NormalTexture)
        {
            if (props.texUsage & ShaderProperties::CompressedNormalTexture)
            {
                source += "vec3 n;\n";
                source += "n.xy = texture2D(normTex, " + normTexCoord + ".st).ag * 2.0 - vec2(1.0);\n";
                source += "n.z = sqrt(1.0 - n.x * n.x - n.y * n.y);\n";
            }
            else
            {
                // TODO: normalizing the filtered normal texture value noticeably improves the appearance; add
                // an option for this.
                //source += "vec3 n = normalize(texture2D(normTex, " + normTexCoord + ".st).xyz * 2.0 - vec3(1.0));\n";
                source += "vec3 n = texture2D(normTex, " + normTexCoord + ".st).xyz * 2.0 - vec3(1.0);\n";
            }
        }
        else
        {
            source += "vec3 n = vec3(0.0, 0.0, 1.0);\n";
        }

        source += "float l;\n";

        if (props.isViewDependent())
        {
            source += "vec3 V = normalize(eyeDir_tan);\n";

            if ((props.lightModel & ShaderProperties::PerPixelSpecularModel) != 0)
            {
                source += "vec3 H;\n";
                source += "float NH;\n";
            }
            if ((props.lightModel & ShaderProperties::LunarLambertModel) != 0)
            {
                source += "float NV = dot(n, V);\n";
            }
        }

        source += "float NL;\n";

        for (unsigned i = 0; i < props.nLights; i++)
        {
            // Bump mapping with self shadowing
            // TODO: normalize the light direction (optionally--not as important for finely tesselated
            // geometry like planet spheres.)
            // source += LightDir_tan(i) + " = normalize(" + LightDir(i)_tan + ");\n";
            source += "NL = dot(" + LightDir_tan(i) + ", n);\n";
            if ((props.lightModel & ShaderProperties::LunarLambertModel) != 0)
            {
                source += "NL = max(0.0, NL);\n";
                source += "l = mix(NL, (NL / (max(NV, 0.001) + NL)), lunarLambert) * clamp(" + LightDir_tan(i) + ".z * 8.0, 0.0, 1.0);\n";
            }
            else
            {
                source += "l = max(0.0, dot(" + LightDir_tan(i) + ", n)) * clamp(" + LightDir_tan(i) + ".z * 8.0, 0.0, 1.0);\n";
            }

            if ((props.texUsage & ShaderProperties::NightTexture) &&
                !VSComputesColorSum(props))
            {
                if (i == 0)
                    source += "float totalLight = ";
                else
                    source += "totalLight += ";
                source += "l * " + FragLightProperty(i, "brightness") + ";\n";
            }

            std::string illum;
            if (props.hasShadowsForLight(i))
                illum = std::string("l * shadow");
            else
                illum = std::string("l");

            if (props.hasShadowsForLight(i))
                source += ShadowsForLightSource(props, i);

            source += "diff.rgb += " + illum + " * " +
                FragLightProperty(i, "color") + ";\n";

            if (props.lightModel == ShaderProperties::SpecularModel && props.usesShadows())
            {
                source += "spec.rgb += " + illum + " * " + SeparateSpecular(i) +
                    " * " + FragLightProperty(i, "specColor") +
                    ";\n";
            }
            else if ((props.lightModel & ShaderProperties::PerPixelSpecularModel) != 0)
            {
                source += "H = normalize(eyeDir_tan + " + LightDir_tan(i) + ");\n";
                source += "NH = max(0.0, dot(n, H));\n";
                source += "spec.rgb += " + illum + " * pow(NH, shininess) * " + FragLightProperty(i, "specColor") + ";\n";
            }
        }
    }
    else if ((props.lightModel & ShaderProperties::PerPixelSpecularModel) != 0)
    {
        source += "float NH;\n";
        source += "vec3 n = normalize(normal);\n";
        source += "float shadowMapCoeff = 1.0;\n";

        // Sum the contributions from each light source
        for (unsigned i = 0; i < props.nLights; i++)
        {
            std::string illum;

            if (props.hasShadowsForLight(i))
                illum = std::string("shadow");
            else
                illum = SeparateDiffuse(i);

            if (props.hasShadowsForLight(i))
                source += ShadowsForLightSource(props, i);

            source += "diff.rgb += " + illum + " * " + FragLightProperty(i, "color") + ";\n";
            source += "NH = max(0.0, dot(n, normalize(" + LightProperty(i, "halfVector") + ")));\n";
            source += "spec.rgb += " + illum + " * pow(NH, shininess) * " + FragLightProperty(i, "specColor") + ";\n";
            if (props.hasShadowMap() && i == 0)
                source += ApplyShadow(true);
        }
    }
    else if (props.usesShadows())
    {
        source += "float shadowMapCoeff = 1.0;\n";

        // Sum the contributions from each light source
        for (unsigned i = 0; i < props.nLights; i++)
        {
            source += ShadowsForLightSource(props, i);
            source += "diff.rgb += shadow * " +
                FragLightProperty(i, "color") + ";\n";
            if (props.lightModel == ShaderProperties::SpecularModel)
            {
                source += "spec.rgb += shadow * " + SeparateSpecular(i) +
                    " * " +
                    FragLightProperty(i, "specColor") + ";\n";
            }
            if (props.hasShadowMap() && i == 0)
                ApplyShadow(props.lightModel == ShaderProperties::SpecularModel);
        }
    }

    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        if (props.usePointSize())
            source += "color = texture2D(diffTex, gl_PointCoord);\n";
        else
            source += "color = texture2D(diffTex, " + diffTexCoord + ".st);\n";
    }
    else
    {
        source += "color = vec4(1.0);\n";
    }

#if POINT_FADE
    if (props.usePointSize())
    {
        source += "color.a *= pointFade;\n";
    }
#endif

    // Mix in the overlay color with the base color
    if (props.texUsage & ShaderProperties::OverlayTexture)
    {
        source += "vec4 overlayColor = texture2D(overlayTex, overlayTexCoord.st);\n";
        source += "color.rgb = mix(color.rgb, overlayColor.rgb, overlayColor.a);\n";
    }

    if (props.hasSpecular())
    {
        // Add in the specular color
        if (props.texUsage & ShaderProperties::SpecularInDiffuseAlpha)
            source += "gl_FragColor = vec4(color.rgb, 1.0) * diff + float(color.a) * spec;\n";
        else if (props.texUsage & ShaderProperties::SpecularTexture)
            source += "gl_FragColor = color * diff + texture2D(specTex, " + specTexCoord + ".st) * spec;\n";
        else
            source += "gl_FragColor = color * diff + spec;\n";
    }
    else
    {
        source += "gl_FragColor = color * diff;\n";
    }

    // Add in the emissive color
    // TODO: support a constant emissive color, not just an emissive texture
    if (props.texUsage & ShaderProperties::NightTexture)
    {
        // If the night texture blend factor wasn't computed in the vertex
        // shader, we need to do so now.
        if (!VSComputesColorSum(props))
        {
            if (!props.usesTangentSpaceLighting())
            {
                source += "float totalLight = ";

                if (props.nLights == 0)
                {
                    source += "0.0f;\n";
                }
                else
                {
                    int k;
                    for (k = 0; k < props.nLights - 1; k++)
                        source += SeparateDiffuse(k) + " + ";
                    source += SeparateDiffuse(k) + ";\n";
                }
            }

            source += NightTextureBlend();
        }

        source += "gl_FragColor += texture2D(nightTex, " + nightTexCoord + ".st) * totalLight;\n";
    }

    if (props.texUsage & ShaderProperties::EmissiveTexture)
    {
        source += "gl_FragColor += texture2D(emissiveTex, " + emissiveTexCoord + ".st);\n";
    }

    // Include the effect of atmospheric scattering.
    if (props.hasScattering())
    {
        source += "gl_FragColor.rgb = gl_FragColor.rgb * scatterEx + scatterColor;\n";
    }

    source += "}\n";

    DumpFSSource(source);

    GLFragmentShader* fs = nullptr;
    GLShaderStatus status = GLShaderLoader::CreateFragmentShader(source, &fs);
    return status == ShaderStatus_OK ? fs : nullptr;
}


#if 0
GLVertexShader*
ShaderManager::buildRingsVertexShader(const ShaderProperties& props)
{
    std::string source(VersionHeader);
    source += CommonHeader;
    source += VertexHeader;
    source += CommonAttribs;

    source += DeclareLights(props);
    source += DeclareUniform("eyePosition", Shader_Vector3);

    source += DeclareVarying("diffFactors", Shader_Vector4);

    if (props.texUsage & ShaderProperties::DiffuseTexture)
        source += DeclareVarying("diffTexCoord", Shader_Vector2);

    if (props.shadowCounts != 0)
    {
        source += DeclareVarying("position_obj", Shader_Vector3);
        source += DeclareVarying("shadowDepths", Shader_Vector4);
    }

    source += "\nvoid main(void)\n{\n";

    // Get the normalized direction from the eye to the vertex
    source += "vec3 eyeDir = normalize(eyePosition - in_Position.xyz);\n";

    for (unsigned int i = 0; i < props.nLights; i++)
    {
        source += SeparateDiffuse(i) + " = (dot(" +
            LightProperty(i, "direction") + ", eyeDir) + 1.0) * 0.5;\n";
    }

    if (props.texUsage & ShaderProperties::DiffuseTexture)
        source += "diffTexCoord = " + TexCoord2D(0) + ";\n";

    if (props.hasEclipseShadows() != 0)
    {
        source += "position_obj = in_Position.xyz;\n";
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            source += ShadowDepth(i) + " = dot(in_Position.xyz, " +
                       LightProperty(i, "direction") + ");\n";
        }
    }

    source += VertexPosition;
    source += "}\n";

    DumpVSSource(source);

    GLVertexShader* vs = nullptr;
    GLShaderStatus status = GLShaderLoader::CreateVertexShader(source, &vs);
    return status == ShaderStatus_OK ? vs : nullptr;
}


GLFragmentShader*
ShaderManager::buildRingsFragmentShader(const ShaderProperties& props)
{
    std::string source(VersionHeader);
    source += CommonHeader;

    source += DeclareUniform("ambientColor", Shader_Vector3);
    source += "vec4 diff = vec4(ambientColor, 1.0);\n";
    for (unsigned int i = 0; i < props.nLights; i++)
        source += DeclareUniform(FragLightProperty(i, "color"), Shader_Vector3);

    source += DeclareVarying("diffFactors", Shader_Vector4);

    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        source += DeclareVarying("diffTexCoord", Shader_Vector2);
        source += DeclareUniform("diffTex", Shader_Sampler2D);
    }

    if (props.hasEclipseShadows())
    {
        source += DeclareVarying("position_obj", Shader_Vector3);
        source += DeclareVarying("shadowDepths", Shader_Vector4);

        for (unsigned int i = 0; i < props.nLights; i++)
        {
            for (unsigned int j = 0; j < props.getEclipseShadowCountForLight(i); j++)
            {
                source += DeclareUniform(IndexedParameter("shadowTexGenS", i, j), Shader_Vector4);
                source += DeclareUniform(IndexedParameter("shadowTexGenT", i, j), Shader_Vector4);
                source += DeclareUniform(IndexedParameter("shadowFalloff", i, j), Shader_Float);
                source += DeclareUniform(IndexedParameter("shadowMaxDepth", i, j), Shader_Float);
            }
        }
    }

    source += "\nvoid main(void)\n{\n";
    source += "vec4 color;\n";

    if (props.hasEclipseShadows())
    {
        // Temporaries required for shadows
        source += "float shadow;\n";
        source += "vec2 shadowCenter;\n";
        source += "float shadowR;\n";
    }

    // Sum the contributions from each light source
    for (unsigned i = 0; i < props.nLights; i++)
    {
        if (props.getEclipseShadowCountForLight(i) > 0)
        {
            source += "shadow = 1.0;\n";
            source += Shadow(i, 0);
            source += "shadow = min(1.0, shadow + step(0.0, " + ShadowDepth(i) + "));\n";
            source += "diff.rgb += (shadow * " + SeparateDiffuse(i) + ") * " +
                FragLightProperty(i, "color") + ";\n";
        }
        else
        {
            source += "diff.rgb += " + SeparateDiffuse(i) + " * " +
                FragLightProperty(i, "color") + ";\n";
        }
    }

    if (props.texUsage & ShaderProperties::DiffuseTexture)
        source += "color = texture2D(diffTex, diffTexCoord.st);\n";
    else
        source += "color = vec4(1.0);\n";

    source += "gl_FragColor = color * diff;\n";

    source += "}\n";

    DumpFSSource(source);

    GLFragmentShader* fs = nullptr;
    GLShaderStatus status = GLShaderLoader::CreateFragmentShader(source, &fs);
    return status == ShaderStatus_OK ? fs : nullptr;
}
#endif


GLVertexShader*
ShaderManager::buildRingsVertexShader(const ShaderProperties& props)
{
    std::string source(VersionHeader);
    source += CommonHeader;
    source += VertexHeader;
    source += CommonAttribs;

    source += DeclareLights(props);

    source += DeclareVarying("position_obj", Shader_Vector3);
    if (props.hasEclipseShadows())
    {
        source += DeclareVarying("shadowDepths", Shader_Vector4);
    }

    if (props.texUsage & ShaderProperties::DiffuseTexture)
        source += DeclareVarying("diffTexCoord", Shader_Vector2);

    if (props.texUsage & ShaderProperties::LineAsTriangles)
        source += LineDeclaration();

    source += VPFunction(props.fishEyeOverride != ShaderProperties::FisheyeOverrideModeDisabled && fisheyeEnabled);

    source += "\nvoid main(void)\n{\n";

    if (props.texUsage & ShaderProperties::DiffuseTexture)
        source += "diffTexCoord = " + TexCoord2D(0) + ";\n";

    source += "position_obj = in_Position.xyz;\n";
    if (props.hasEclipseShadows())
    {
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            source += ShadowDepth(i) + " = dot(in_Position.xyz, " +
                       LightProperty(i, "direction") + ");\n";
        }
    }

    source += VertexPosition(props);
    source += "}\n";

    DumpVSSource(source);

    GLVertexShader* vs = nullptr;
    GLShaderStatus status = GLShaderLoader::CreateVertexShader(source, &vs);
    return status == ShaderStatus_OK ? vs : nullptr;
}


GLFragmentShader*
ShaderManager::buildRingsFragmentShader(const ShaderProperties& props)
{
    std::string source(VersionHeader);
    source += CommonHeader;

    source += DeclareUniform("ambientColor", Shader_Vector3);
    for (unsigned int i = 0; i < props.nLights; i++)
        source += DeclareUniform(FragLightProperty(i, "color"), Shader_Vector3);

    source += DeclareLights(props);

    source += DeclareUniform("eyePosition", Shader_Vector3);
    source += DeclareVarying("position_obj", Shader_Vector3);

    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        source += DeclareVarying("diffTexCoord", Shader_Vector2);
        source += DeclareUniform("diffTex", Shader_Sampler2D);
    }

    if (props.hasEclipseShadows())
    {
        source += DeclareVarying("shadowDepths", Shader_Vector4);

        for (unsigned int i = 0; i < props.nLights; i++)
        {
            for (unsigned int j = 0; j < props.getEclipseShadowCountForLight(i); j++)
            {
                source += DeclareUniform(IndexedParameter("shadowTexGenS", i, j), Shader_Vector4);
                source += DeclareUniform(IndexedParameter("shadowTexGenT", i, j), Shader_Vector4);
                source += DeclareUniform(IndexedParameter("shadowFalloff", i, j), Shader_Float);
                source += DeclareUniform(IndexedParameter("shadowMaxDepth", i, j), Shader_Float);
            }
        }
    }

    source += "\nvoid main(void)\n{\n";

    source += "vec4 diff = vec4(ambientColor, 1.0);\n";

    // Get the normalized direction from the eye to the vertex
    source += "vec3 eyeDir = normalize(eyePosition - position_obj);\n";

    source += DeclareLocal("color", Shader_Vector4);
    if (props.texUsage & ShaderProperties::DiffuseTexture)
        source += "color = texture2D(diffTex, diffTexCoord.st);\n";
    else
        source += "color = vec4(1.0);\n";
    source += DeclareLocal("opticalDepth", Shader_Float, sh_vec4("color")["a"]);
    if (props.hasEclipseShadows())
    {
        // Temporaries required for shadows
        source += DeclareLocal("shadow",       Shader_Float);
        source += DeclareLocal("shadowCenter", Shader_Vector2);
        source += DeclareLocal("shadowR",      Shader_Float);
    }

    // Sum the contributions from each light source
    source += DeclareLocal("intensity", Shader_Float);
    source += DeclareLocal("litSide", Shader_Float);

    for (unsigned i = 0; i < props.nLights; i++)
    {
        // litSide is 1 when viewer and light are on the same side of the rings, 0 otherwise
        source += assign("litSide", 1.0f - step(0.0f, sh_vec3(LightProperty(i, "direction"))["y"] * sh_vec3("eyeDir")["y"]));
        //source += assign("litSide", 1.0f - step(0.0f, sh_vec3("eyePosition")["y"]));

        source += assign("intensity", (dot(sh_vec3(LightProperty(i, "direction")), sh_vec3("eyeDir")) + 1.0f) * 0.5f);
        source += assign("intensity", mix(sh_float("intensity"), sh_float("intensity") * (1.0f - sh_float("opticalDepth")), sh_float("litSide")));
        if (props.getEclipseShadowCountForLight(i) > 0)
        {
            source += "shadow = 1.0;\n";
            source += Shadow(i, 0);
            source += "shadow = min(1.0, shadow + step(0.0, " + ShadowDepth(i) + "));\n";
#if 0
            source += "diff.rgb += (shadow * " + SeparateDiffuse(i) + ") * " +
                FragLightProperty(i, "color") + ";\n";
#endif
            source += "diff.rgb += (shadow * intensity) * " + FragLightProperty(i, "color") + ";\n";
        }
        else
        {
            source += "diff.rgb += intensity * " + FragLightProperty(i, "color") + ";\n";
#if 0
            source += SeparateDiffuse(i) + " = (dot(" +
                LightProperty(i, "direction") + ", eyeDir) + 1.0) * 0.5;\n";
#endif
#if 0
            source += "diff.rgb += " + SeparateDiffuse(i) + " * " +
                FragLightProperty(i, "color") + ";\n";
#endif
        }
    }

    source += "gl_FragColor = vec4(color.rgb * diff.rgb, opticalDepth);\n";

    source += "}\n";

    DumpFSSource(source);

    GLFragmentShader* fs = nullptr;
    GLShaderStatus status = GLShaderLoader::CreateFragmentShader(source, &fs);
    return status == ShaderStatus_OK ? fs : nullptr;
}


GLVertexShader*
ShaderManager::buildAtmosphereVertexShader(const ShaderProperties& props)
{
    std::string source(VersionHeader);
    source += CommonHeader;
    source += VertexHeader;
    source += CommonAttribs;

    source += DeclareLights(props);
    source += DeclareUniform("eyePosition", Shader_Vector3);
    source += ScatteringConstantDeclarations(props);
    for (unsigned int i = 0; i < props.nLights; i++)
    {
        source += DeclareVarying(ScatteredColor(i), Shader_Vector3);
    }

    source += DeclareVarying("scatterEx", Shader_Vector3);
    source += DeclareVarying("eyeDir_obj", Shader_Vector3);

    if (props.texUsage & ShaderProperties::LineAsTriangles)
        source += LineDeclaration();

    source += VPFunction(props.fishEyeOverride != ShaderProperties::FisheyeOverrideModeDisabled && fisheyeEnabled);

    // Begin main() function
    source += "\nvoid main(void)\n{\n";
    source += "float NL;\n";
    source += "vec3 eyeDir = normalize(eyePosition - in_Position.xyz);\n";
    source += "float NV = dot(in_Normal, eyeDir);\n";

    source += AtmosphericEffects(props);

    source += "eyeDir_obj = eyeDir;\n";
    source += VertexPosition(props);
    source += "}\n";

    DumpVSSource(source);

    GLVertexShader* vs = nullptr;
    GLShaderStatus status = GLShaderLoader::CreateVertexShader(source, &vs);
    return status == ShaderStatus_OK ? vs : nullptr;
}


GLFragmentShader*
ShaderManager::buildAtmosphereFragmentShader(const ShaderProperties& props)
{
    std::string source(VersionHeader);
    source += CommonHeader;

    source += DeclareVarying("scatterEx", Shader_Vector3);
    source += DeclareVarying("eyeDir_obj", Shader_Vector3);

    // Scattering constants
    source += DeclareUniform("mieK", Shader_Float);
    source += DeclareUniform("mieCoeff", Shader_Float);
    source += DeclareUniform("rayleighCoeff", Shader_Vector3);
    source += DeclareUniform("invScatterCoeffSum", Shader_Vector3);

#ifdef USE_GLSL_STRUCTS
    source += DeclareLights(props);
#endif
    unsigned int i;
    for (i = 0; i < props.nLights; i++)
    {
#ifndef USE_GLSL_STRUCTS
        source += DeclareUniform(LightProperty(i, "direction"), Shader_Vector3);
#endif
        source += DeclareVarying(ScatteredColor(i), Shader_Vector3);
    }

    source += "\nvoid main(void)\n{\n";

    // Sum the contributions from each light source
    source += "vec3 color = vec3(0.0);\n";
    source += "vec3 V = normalize(eyeDir_obj);\n";

    // Only do scattering calculations for the primary light source
    // TODO: Eventually handle multiple light sources, and removed the 'min'
    // from the line below.
    for (i = 0; i < std::min(static_cast<unsigned int>(props.nLights), 1u); i++)
    {
        source += "    float cosTheta = dot(V, " + LightProperty(i, "direction") + ");\n";
        source += ScatteringPhaseFunctions(props);

        // TODO: Consider premultiplying by invScatterCoeffSum
        source += "    color += (phRayleigh * rayleighCoeff + phMie * mieCoeff) * invScatterCoeffSum * " + ScatteredColor(i) + ";\n";
    }

    source += "    gl_FragColor = vec4(color, dot(scatterEx, vec3(0.333)));\n";
    source += "}\n";

    DumpFSSource(source);

    GLFragmentShader* fs = nullptr;
    GLShaderStatus status = GLShaderLoader::CreateFragmentShader(source, &fs);
    return status == ShaderStatus_OK ? fs : nullptr;
}


// The emissive shader ignores all lighting and uses the diffuse color
// as the final fragment color.
GLVertexShader*
ShaderManager::buildEmissiveVertexShader(const ShaderProperties& props)
{
    std::string source(VersionHeader);
    source += CommonHeader;
    source += VertexHeader;
    source += CommonAttribs;

    source += DeclareUniform("opacity", Shader_Float);

    // There are no light sources used for the emissive light model, but
    // we still need the diffuse property of light 0. For other lighting
    // models, the material color is premultiplied with the light color.
    // Emissive shaders interoperate better with other shaders if they also
    // take the color from light source 0.
#ifndef USE_GLSL_STRUCTS
    source += DeclareUniform("light0_diffuse", Shader_Vector3);
#else
    source += std::string("uniform struct {\n   vec3 diffuse;\n} lights[1];\n");
#endif

    if (props.usePointSize())
        source += PointSizeDeclaration();

    source += DeclareVarying("v_Color", Shader_Vector4);
    source += DeclareVarying("v_TexCoord0", Shader_Vector2);

    if (props.texUsage & ShaderProperties::LineAsTriangles)
        source += LineDeclaration();

    source += VPFunction(props.fishEyeOverride != ShaderProperties::FisheyeOverrideModeDisabled && fisheyeEnabled);

    // Begin main() function
    source += "\nvoid main(void)\n{\n";

    // Optional texture coordinates (generated automatically for point
    // sprites.)
    if ((props.texUsage & ShaderProperties::DiffuseTexture) &&
        !props.usePointSize())
    {
        source += "    v_TexCoord0.st = " + TexCoord2D(0) + ";\n";
    }

    // Set the color.
    std::string colorSource;
    if (props.texUsage & ShaderProperties::VertexColors)
        colorSource = "in_Color.rgb";
    else
        colorSource = LightProperty(0, "diffuse");

    source += "    v_Color = vec4(" + colorSource + ", opacity);\n";

    // Optional point size
    if (props.texUsage & ShaderProperties::PointSprite)
        source += PointSizeCalculation();
    else if (props.texUsage & ShaderProperties::StaticPointSize)
        source += StaticPointSize();

    source += VertexPosition(props);
    source += "}\n";
    // End of main()

    DumpVSSource(source);

    GLVertexShader* vs = nullptr;
    GLShaderStatus status = GLShaderLoader::CreateVertexShader(source, &vs);
    return status == ShaderStatus_OK ? vs : nullptr;
}


GLFragmentShader*
ShaderManager::buildEmissiveFragmentShader(const ShaderProperties& props)
{
    std::string source(VersionHeader);
    source += CommonHeader;

    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        source += DeclareUniform("diffTex", Shader_Sampler2D);
    }

    if (props.usePointSize())
    {
        source += DeclareVarying("pointFade", Shader_Float);
    }

    source += DeclareVarying("v_Color", Shader_Vector4);
    source += DeclareVarying("v_TexCoord0", Shader_Vector2);

    // Begin main()
    source += "\nvoid main(void)\n{\n";

    std::string colorSource = "v_Color";
    if (props.usePointSize())
    {
        source += "    vec4 color = v_Color;\n";
#if POINT_FADE
        source += "    color.a *= pointFade;\n";
#endif
        colorSource = "color";
    }

    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        if (props.usePointSize())
            source += "    gl_FragColor = " + colorSource + " * texture2D(diffTex, gl_PointCoord);\n";
        else
            source += "    gl_FragColor = " + colorSource + " * texture2D(diffTex, v_TexCoord0.st);\n";
    }
    else
    {
        source += "    gl_FragColor = " + colorSource + " ;\n";
    }

    source += "}\n";
    // End of main()

    DumpFSSource(source);

    GLFragmentShader* fs = nullptr;
    GLShaderStatus status = GLShaderLoader::CreateFragmentShader(source, &fs);
    return status == ShaderStatus_OK ? fs : nullptr;
}


// Build the vertex shader used for rendering particle systems.
GLVertexShader*
ShaderManager::buildParticleVertexShader(const ShaderProperties& props)
{
    std::ostringstream source;
    source << VersionHeader;
    source << CommonHeader;
    source << VertexHeader;
    source << CommonAttribs;

    source << "// PARTICLE SHADER\n";
    source << "// shadow count: " << props.shadowCounts << std::endl;

    source << DeclareLights(props);

    source << DeclareUniform("eyePosition", Shader_Vector3);

    // TODO: scattering constants

    if (props.usePointSize())
        source << PointSizeDeclaration();

     source << DeclareVarying("v_Color", Shader_Vector4);

    // Shadow parameters
    if (props.shadowCounts != 0)
    {
        source << DeclareVarying("position_obj", Shader_Vector3);
    }

    if (props.texUsage & ShaderProperties::LineAsTriangles)
        source << LineDeclaration();

    source << VPFunction(props.fishEyeOverride != ShaderProperties::FisheyeOverrideModeDisabled && fisheyeEnabled);

    // Begin main() function
    source << "\nvoid main(void)\n{\n";

#define PARTICLE_PHASE_PARAMETER 0
#if PARTICLE_PHASE_PARAMETER
    float g = -0.4f;
    float miePhaseAsymmetry = 1.55f * g - 0.55f * g * g * g;
    source << "    float mieK = " << miePhaseAsymmetry << ";\n";

    source << "    vec3 eyeDir = normalize(eyePosition - in_Position.xyz);\n";
    source << "    float brightness = 0.0;\n";
    for (unsigned int i = 0; i < std::min(1u, props.nLights); i++)
    {
        source << "    {\n";
        source << "         float cosTheta = dot(" << LightProperty(i, "direction") << ", eyeDir);\n";
        source << "         float phMie = (1.0 - mieK * mieK) / ((1.0 - mieK * cosTheta) * (1.0 - mieK * cosTheta));\n";
        source << "         brightness += phMie;\n";
        source << "    }\n";
    }
#else
    source << "    float brightness = 1.0;\n";
#endif

    // Set the color. Should *always* use vertex colors for color and opacity.
    source << "    v_Color = in_Color * brightness;\n";

    // Optional point size
    if (props.texUsage & ShaderProperties::PointSprite)
        source << PointSizeCalculation();
    else if (props.texUsage & ShaderProperties::StaticPointSize)
        source << StaticPointSize();

    source << VertexPosition(props);
    source << "}\n";
    // End of main()

    DumpVSSource(source);

    GLVertexShader* vs = nullptr;
    GLShaderStatus status = GLShaderLoader::CreateVertexShader(source.str(), &vs);
    return status == ShaderStatus_OK ? vs : nullptr;
}


GLFragmentShader*
ShaderManager::buildParticleFragmentShader(const ShaderProperties& props)
{
    std::ostringstream source;

    source << VersionHeader << CommonHeader;

    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        source << DeclareUniform("diffTex", Shader_Sampler2D);
    }

    if (props.usePointSize())
    {
        source << DeclareVarying("pointFade", Shader_Float);
    }

    if (props.usesShadows())
    {
        source << DeclareUniform("ambientColor", Shader_Vector3);
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            source << DeclareUniform(FragLightProperty(i, "color"), Shader_Vector3);
        }
    }

    // Declare shadow parameters
    if (props.shadowCounts != 0)
    {
        source << DeclareVarying("position_obj", Shader_Vector3);
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            for (unsigned int j = 0; j < props.getEclipseShadowCountForLight(i); j++)
            {
                source << DeclareUniform(IndexedParameter("shadowTexGenS", i, j), Shader_Vector4);
                source << DeclareUniform(IndexedParameter("shadowTexGenT", i, j), Shader_Vector4);
                source << DeclareUniform(IndexedParameter("shadowFalloff", i, j), Shader_Float);
                source << DeclareUniform(IndexedParameter("shadowMaxDepth", i, j), Shader_Float);
            }
        }
    }

    source << DeclareVarying("v_Color", Shader_Vector4);

    // Begin main()
    source << "\nvoid main(void)\n{\n";

    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        source << "    gl_FragColor = v_Color * texture2D(diffTex, gl_PointCoord);\n";
    }
    else
    {
        source << "    gl_FragColor = v_Color;\n";
    }

    source << "}\n";
    // End of main()

    DumpFSSource(source);

    GLFragmentShader* fs = nullptr;
    GLShaderStatus status = GLShaderLoader::CreateFragmentShader(source.str(), &fs);
    return status == ShaderStatus_OK ? fs : nullptr;
}

CelestiaGLProgram*
ShaderManager::buildProgram(const ShaderProperties& props)
{
    GLProgram* prog = nullptr;
    GLShaderStatus status;

    GLVertexShader* vs = nullptr;
    GLFragmentShader* fs = nullptr;

    if (props.lightModel == ShaderProperties::RingIllumModel)
    {
        vs = buildRingsVertexShader(props);
        fs = buildRingsFragmentShader(props);
    }
    else if (props.lightModel == ShaderProperties::AtmosphereModel)
    {
        vs = buildAtmosphereVertexShader(props);
        fs = buildAtmosphereFragmentShader(props);
    }
    else if (props.lightModel == ShaderProperties::EmissiveModel)
    {
        vs = buildEmissiveVertexShader(props);
        fs = buildEmissiveFragmentShader(props);
    }
    else if (props.lightModel == ShaderProperties::ParticleModel)
    {
        vs = buildParticleVertexShader(props);
        fs = buildParticleFragmentShader(props);
    }
    else
    {
        vs = buildVertexShader(props);
        fs = buildFragmentShader(props);
    }

    if (vs != nullptr && fs != nullptr)
    {
        status = GLShaderLoader::CreateProgram(*vs, *fs, &prog);
        if (status == ShaderStatus_OK)
        {
            BindAttribLocations(prog);
            status = prog->link();
        }
    }
    else
    {
        status = ShaderStatus_CompileError;
    }

    delete vs;
    delete fs;

    if (status != ShaderStatus_OK)
    {
        // If the shader creation failed for some reason, substitute the
        // error shader.
        if (CreateErrorShader(&prog, fisheyeEnabled) != ShaderStatus_OK)
            return nullptr;
    }

    return new CelestiaGLProgram(*prog, props);
}

CelestiaGLProgram*
ShaderManager::buildProgram(std::string_view vs, std::string_view fs)
{
    GLProgram* prog = nullptr;
    GLShaderStatus status;
    std::string _vs = fmt::format("{}{}{}{}{}\n", VersionHeader, CommonHeader, VertexHeader, VPFunction(fisheyeEnabled), vs);
    std::string _fs = fmt::format("{}{}{}{}\n", VersionHeader, CommonHeader, FragmentHeader, fs);

    DumpVSSource(_vs);
    DumpFSSource(_fs);

    status = GLShaderLoader::CreateProgram(_vs, _fs, &prog);
    if (status == ShaderStatus_OK)
    {
        BindAttribLocations(prog);
        status = prog->link();
    }

    if (status != ShaderStatus_OK)
    {
        // If the shader creation failed for some reason, substitute the
        // error shader.
        if (CreateErrorShader(&prog, fisheyeEnabled) != ShaderStatus_OK)
            return nullptr;
    }

    return new CelestiaGLProgram(*prog);
}

CelestiaGLProgram*
ShaderManager::buildProgramGL3(std::string_view vs, std::string_view fs)
{
    GLProgram* prog = nullptr;
    GLShaderStatus status;
    std::string _vs = fmt::format("{}{}{}{}{}\n", VersionHeaderGL3, CommonHeader, VertexHeader, VPFunction(fisheyeEnabled), vs);
    std::string _fs = fmt::format("{}{}{}{}\n", VersionHeaderGL3, CommonHeader, FragmentHeader, fs);

    DumpVSSource(_vs);
    DumpFSSource(_fs);

    status = GLShaderLoader::CreateProgram(_vs, _fs, &prog);
    if (status == ShaderStatus_OK)
    {
        BindAttribLocations(prog);
        status = prog->link();
    }

    if (status != ShaderStatus_OK)
    {
        // If the shader creation failed for some reason, substitute the
        // error shader.
        if (CreateErrorShader(&prog, fisheyeEnabled) != ShaderStatus_OK)
            return nullptr;
    }

    return new CelestiaGLProgram(*prog);
}

CelestiaGLProgram*
ShaderManager::buildProgramGL3(std::string_view vs, std::string_view gs, std::string_view fs, const GeomShaderParams* params)
{
    std::string layout;
    if (params != nullptr)
    {
        layout = fmt::format("layout({}) in;\nlayout({}, max_vertices = {}) out;\n"sv,
                             InPrimitive(params->inputType),
                             OutPrimitive(params->outType),
                             params->nOutVertices
        );
    }

    GLProgram* prog = nullptr;
    GLShaderStatus status;
    auto _vs = fmt::format("{}{}{}{}\n", VersionHeaderGL3, CommonHeader, VertexHeader, vs);
    auto _gs = fmt::format("{}{}{}{}{}{}\n", VersionHeaderGL3, CommonHeader, layout, GeomHeaderGL3, VPFunction(fisheyeEnabled), gs);
    auto _fs = fmt::format("{}{}{}{}\n", VersionHeaderGL3, CommonHeader, FragmentHeader, fs);

    DumpVSSource(_vs);
    DumpGSSource(_gs);
    DumpFSSource(_fs);

    status = GLShaderLoader::CreateProgram(_vs, _gs, _fs, &prog);
    if (status == ShaderStatus_OK)
    {
        BindAttribLocations(prog);
        status = prog->link();
    }

    if (status != ShaderStatus_OK)
    {
        // If the shader creation failed for some reason, substitute the
        // error shader.
        if (CreateErrorShader(&prog, fisheyeEnabled) != ShaderStatus_OK)
            return nullptr;
    }

    return new CelestiaGLProgram(*prog);
}

void ShaderManager::setFisheyeEnabled(bool enabled)
{
    fisheyeEnabled = enabled;
}

CelestiaGLProgram::CelestiaGLProgram(GLProgram& _program,
                                     const ShaderProperties& _props) :
    program(&_program),
    props(_props)
{
    initParameters();
    initSamplers();
}


CelestiaGLProgram::CelestiaGLProgram(GLProgram& _program) :
    program(&_program)
{
    initCommonParameters();
}

CelestiaGLProgram::~CelestiaGLProgram()
{
    delete program;
}


FloatShaderParameter
CelestiaGLProgram::floatParam(const std::string& paramName)
{
    return FloatShaderParameter(program->getID(), paramName.c_str());
}


IntegerShaderParameter
CelestiaGLProgram::intParam(const std::string& paramName)
{
    return IntegerShaderParameter(program->getID(), paramName.c_str());
}

IntegerShaderParameter
CelestiaGLProgram::samplerParam(const std::string& paramName)
{
    return IntegerShaderParameter(program->getID(), paramName.c_str());
}


Vec3ShaderParameter
CelestiaGLProgram::vec3Param(const std::string& paramName)
{
    return Vec3ShaderParameter(program->getID(), paramName.c_str());
}


Vec4ShaderParameter
CelestiaGLProgram::vec4Param(const std::string& paramName)
{
    return Vec4ShaderParameter(program->getID(), paramName.c_str());
}


Mat3ShaderParameter
CelestiaGLProgram::mat3Param(const std::string& paramName)
{
    return Mat3ShaderParameter(program->getID(), paramName.c_str());
}


Mat4ShaderParameter
CelestiaGLProgram::mat4Param(const std::string& paramName)
{
    return Mat4ShaderParameter(program->getID(), paramName.c_str());
}


int
CelestiaGLProgram::attribIndex(const std::string& paramName) const
{
    return glGetAttribLocation(program->getID(), paramName.c_str());
}

void
CelestiaGLProgram::initCommonParameters()
{
    ModelViewMatrix = mat4Param("ModelViewMatrix");
    ProjectionMatrix = mat4Param("ProjectionMatrix");
    MVPMatrix = mat4Param("MVPMatrix");
    lineWidthX = floatParam("lineWidthX");
    lineWidthY = floatParam("lineWidthY");
}

void
CelestiaGLProgram::initParameters()
{
    initCommonParameters();
    for (unsigned int i = 0; i < props.nLights; i++)
    {
        lights[i].direction  = vec3Param(LightProperty(i, "direction"));
        lights[i].diffuse    = vec3Param(LightProperty(i, "diffuse"));
        lights[i].specular   = vec3Param(LightProperty(i, "specular"));
        lights[i].halfVector = vec3Param(LightProperty(i, "halfVector"));
        if (props.texUsage & ShaderProperties::NightTexture)
            lights[i].brightness = floatParam(LightProperty(i, "brightness"));

        fragLightColor[i] = vec3Param(FragLightProperty(i, "color"));
        fragLightSpecColor[i] = vec3Param(FragLightProperty(i, "specColor"));
        if (props.texUsage & ShaderProperties::NightTexture)
            fragLightBrightness[i] = floatParam(FragLightProperty(i, "brightness"));
        if (props.hasRingShadowForLight(i))
            ringShadowLOD[i] = floatParam(IndexedParameter("ringShadowLOD", i));
        for (unsigned int j = 0; j < props.getEclipseShadowCountForLight(i); j++)
        {
            shadows[i][j].texGenS =
                vec4Param(IndexedParameter("shadowTexGenS", i, j));
            shadows[i][j].texGenT =
                vec4Param(IndexedParameter("shadowTexGenT", i, j));
            shadows[i][j].falloff =
                floatParam(IndexedParameter("shadowFalloff", i, j));
            shadows[i][j].maxDepth =
                floatParam(IndexedParameter("shadowMaxDepth", i, j));
        }
    }

    if (props.hasSpecular())
    {
        shininess            = floatParam("shininess");
    }

    if (props.isViewDependent() || props.hasScattering())
    {
        eyePosition          = vec3Param("eyePosition");
    }

    opacity      = floatParam("opacity");
    ambientColor = vec3Param("ambientColor");

    if (props.texUsage & ShaderProperties::RingShadowTexture)
    {
        ringWidth            = floatParam("ringWidth");
        ringRadius           = floatParam("ringRadius");
        ringPlane            = vec4Param("ringPlane");
        ringCenter           = vec3Param("ringCenter");
    }

    textureOffset = floatParam("textureOffset");

    if (props.texUsage & ShaderProperties::CloudShadowTexture)
    {
        cloudHeight         = floatParam("cloudHeight");
        shadowTextureOffset = floatParam("cloudShadowTexOffset");
    }

    if (props.texUsage & ShaderProperties::ShadowMapTexture)
    {
        ShadowMatrix0       = mat4Param("ShadowMatrix0");
    }

    if (props.hasScattering())
    {
        mieCoeff             = floatParam("mieCoeff");
        mieScaleHeight       = floatParam("mieH");
        miePhaseAsymmetry    = floatParam("mieK");
        rayleighCoeff        = vec3Param("rayleighCoeff");
        rayleighScaleHeight  = floatParam("rayleighH");
        atmosphereRadius     = vec3Param("atmosphereRadius");
        scatterCoeffSum      = vec3Param("scatterCoeffSum");
        invScatterCoeffSum   = vec3Param("invScatterCoeffSum");
        extinctionCoeff      = vec3Param("extinctionCoeff");
    }

    if ((props.lightModel & ShaderProperties::LunarLambertModel) != 0)
    {
        lunarLambert         = floatParam("lunarLambert");
    }

    if (props.usePointSize())
    {
        pointScale           = floatParam("pointScale");
    }

    if (props.texUsage & ShaderProperties::LineAsTriangles)
    {
        lineWidthX           = floatParam("lineWidthX");
        lineWidthY           = floatParam("lineWidthY");
    }
}


void
CelestiaGLProgram::initSamplers()
{
    program->use();

    unsigned int nSamplers = 0;

    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        int slot = glGetUniformLocation(program->getID(), "diffTex");
        if (slot != -1)
            glUniform1i(slot, nSamplers++);
    }

    if (props.texUsage & ShaderProperties::NormalTexture)
    {
        int slot = glGetUniformLocation(program->getID(), "normTex");
        if (slot != -1)
            glUniform1i(slot, nSamplers++);
    }

    if (props.texUsage & ShaderProperties::SpecularTexture)
    {
        int slot = glGetUniformLocation(program->getID(), "specTex");
        if (slot != -1)
            glUniform1i(slot, nSamplers++);
    }

    if (props.texUsage & ShaderProperties::NightTexture)
    {
        int slot = glGetUniformLocation(program->getID(), "nightTex");
        if (slot != -1)
            glUniform1i(slot, nSamplers++);
    }

    if (props.texUsage & ShaderProperties::EmissiveTexture)
    {
        int slot = glGetUniformLocation(program->getID(), "emissiveTex");
        if (slot != -1)
            glUniform1i(slot, nSamplers++);
    }

    if (props.texUsage & ShaderProperties::OverlayTexture)
    {
        int slot = glGetUniformLocation(program->getID(), "overlayTex");
        if (slot != -1)
            glUniform1i(slot, nSamplers++);
    }

    if (props.texUsage & ShaderProperties::RingShadowTexture)
    {
        int slot = glGetUniformLocation(program->getID(), "ringTex");
        if (slot != -1)
            glUniform1i(slot, nSamplers++);
    }

    if (props.texUsage & ShaderProperties::CloudShadowTexture)
    {
        int slot = glGetUniformLocation(program->getID(), "cloudShadowTex");
        if (slot != -1)
            glUniform1i(slot, nSamplers++);
    }

    if (props.texUsage & ShaderProperties::ShadowMapTexture)
    {
        int slot = glGetUniformLocation(program->getID(), "shadowMapTex0");
        if (slot != -1)
            glUniform1i(slot, nSamplers++);
    }
}


void
CelestiaGLProgram::setLightParameters(const LightingState& ls,
                                      Color materialDiffuse,
                                      Color materialSpecular,
                                      Color materialEmissive)
{
    unsigned int nLights = std::min(MaxShaderLights, ls.nLights);

    Eigen::Vector3f diffuseColor(materialDiffuse.red(),
                                 materialDiffuse.green(),
                                 materialDiffuse.blue());
    Eigen::Vector3f specularColor(materialSpecular.red(),
                                  materialSpecular.green(),
                                  materialSpecular.blue());

    for (unsigned int i = 0; i < nLights; i++)
    {
        const DirectionalLight& light = ls.lights[i];

        Eigen::Vector3f lightColor = Eigen::Vector3f(light.color.red(),
                                                     light.color.green(),
                                                     light.color.blue()) * light.irradiance;
        lights[i].direction = light.direction_obj;

        // Include a phase-based normalization factor to prevent planets from appearing
        // too dim when rendered with non-Lambertian photometric functions.
        float cosPhaseAngle = light.direction_obj.dot(ls.eyeDir_obj);
        if ((props.lightModel & ShaderProperties::LunarLambertModel) != 0)
        {
            float photometricNormFactor = std::max(1.0f, 1.0f + cosPhaseAngle * 0.5f);
            lightColor *= photometricNormFactor;
        }

        if (props.usesShadows() ||
            props.usesFragmentLighting() ||
            props.lightModel == ShaderProperties::RingIllumModel)
        {
            fragLightColor[i] = lightColor.cwiseProduct(diffuseColor);
            if (props.hasSpecular())
            {
                fragLightSpecColor[i] = lightColor.cwiseProduct(specularColor);
            }
            fragLightBrightness[i] = lightColor.maxCoeff();
        }
        else
        {
            lights[i].diffuse = lightColor.cwiseProduct(diffuseColor);
        }

        lights[i].brightness = lightColor.maxCoeff();
        lights[i].specular = lightColor.cwiseProduct(specularColor);

        Eigen::Vector3f halfAngle_obj = ls.eyeDir_obj + light.direction_obj;
        if (halfAngle_obj.norm() != 0.0f)
            halfAngle_obj.normalize();
        lights[i].halfVector = halfAngle_obj;
    }

    eyePosition = ls.eyePos_obj;
    ambientColor = ls.ambientColor.cwiseProduct(diffuseColor) +
        Eigen::Vector3f(materialEmissive.red(), materialEmissive.green(), materialEmissive.blue());
    opacity = materialDiffuse.alpha();
}


/** Set GLSL shader constants for shadows from ellipsoid occluders; shadows from
 *  irregular objects are not handled yet.
 *  \param scaleFactors the scale factors of the object being shadowed
 *  \param orientation orientation of the object being shadowed
 */
void
CelestiaGLProgram::setEclipseShadowParameters(const LightingState& ls,
                                              const Eigen::Vector3f& scaleFactors,
                                              const Eigen::Quaternionf& orientation)
{
    // Compute the transformation from model to world coordinates
    Eigen::Affine3f rotation(orientation.conjugate());
    Eigen::Matrix4f modelToWorld = (rotation * Eigen::Scaling(scaleFactors)).matrix();

    // The shadow bias matrix maps from
    Eigen::Matrix4f shadowBias;
    shadowBias << 0.5f, 0.0f, 0.0f, 0.5f,
                  0.0f, 0.5f, 0.0f, 0.5f,
                  0.0f, 0.0f, 0.5f, 0.5f,
                  0.0f, 0.0f, 0.0f, 1.0f;

    for (unsigned int li = 0;
         li < std::min(ls.nLights, MaxShaderLights);
         li++)
    {
        if (ls.shadows[li] != nullptr)
        {
            auto nShadows = std::min(MaxShaderEclipseShadows, static_cast<unsigned int>(ls.shadows[li]->size()));

            for (unsigned int i = 0; i < nShadows; i++)
            {
                EclipseShadow& shadow = ls.shadows[li]->at(i);
                CelestiaGLProgramShadow& shadowParams = shadows[li][i];

                // Compute shadow parameters: max depth of at the center of the shadow
                // (always 1 if an eclipse is total) and the linear falloff
                // rate from the center to the outer endge of the penumbra.
                float umbra = shadow.umbraRadius / shadow.penumbraRadius;
                shadowParams.falloff = -shadow.maxDepth / std::max(0.001f, 1.0f - std::fabs(umbra));
                shadowParams.maxDepth = shadow.maxDepth;

                // Compute a transformation that will rotate points in world space to shadow space
                Eigen::Vector3f u = shadow.direction.unitOrthogonal();
                Eigen::Vector3f v = u.cross(shadow.direction);
                Eigen::Matrix4f shadowRotation;
                shadowRotation << u.transpose(),                0.0f,
                                  v.transpose(),                0.0f,
                                  shadow.direction.transpose(), 0.0f,
                                  0.0f, 0.0f, 0.0f,             1.0f;

                // Compose the world-to-shadow matrix
                Eigen::Matrix4f worldToShadow = shadowRotation *
                                                Eigen::Affine3f(Eigen::Scaling(1.0f / shadow.penumbraRadius)).matrix() *
                                                Eigen::Affine3f(Eigen::Translation3f(-shadow.origin)).matrix();

                // Finally, multiply all the matrices together to get the mapping from
                // object space to shadow map space.
                Eigen::Matrix4f m = shadowBias * worldToShadow * modelToWorld;

                shadowParams.texGenS = m.row(0);
                shadowParams.texGenT = m.row(1);
            }
        }
    }
}


// Set the scattering and absorption shader parameters for atmosphere simulation.
// They are from standard units to the normalized system used by the shaders.
// atmPlanetRadius - the radius in km of the planet with the atmosphere
// objRadius - the radius in km of the object we're rendering
void
CelestiaGLProgram::setAtmosphereParameters(const Atmosphere& atmosphere,
                                           float atmPlanetRadius,
                                           float objRadius)
{
    // Compute the radius of the sky sphere to render; the density of the atmosphere
    // falls off exponentially with height above the planet's surface, so the actual
    // radius is infinite. That's a bit impractical, so well just render the portion
    // out to the point where the density is some fraction of the surface density.
    float skySphereRadius = atmPlanetRadius + -atmosphere.mieScaleHeight * std::log(AtmosphereExtinctionThreshold);

    float tMieCoeff                  = atmosphere.mieCoeff * objRadius;
    Eigen::Vector3f tRayleighCoeff   = atmosphere.rayleighCoeff * objRadius;
    Eigen::Vector3f tAbsorptionCoeff = atmosphere.absorptionCoeff * objRadius;

    float r = skySphereRadius / objRadius;
    atmosphereRadius = Eigen::Vector3f(r, r * r, atmPlanetRadius / objRadius);

    mieCoeff = tMieCoeff;
    mieScaleHeight = objRadius / atmosphere.mieScaleHeight;

    // The scattering shaders use the Schlick approximation to the
    // Henyey-Greenstein phase function because it's slightly faster
    // to compute. Convert the HG asymmetry parameter to the Schlick
    // parameter.
    float g = atmosphere.miePhaseAsymmetry;
    miePhaseAsymmetry = 1.55f * g - 0.55f * g * g * g;

    rayleighCoeff = tRayleighCoeff;
    rayleighScaleHeight = 0.0f; // TODO

    // Precompute sum and inverse sum of scattering coefficients to save work
    // in the vertex shader.
    Eigen::Vector3f tScatterCoeffSum = tRayleighCoeff.array() + tMieCoeff;
    scatterCoeffSum = tScatterCoeffSum;
    invScatterCoeffSum = tScatterCoeffSum.cwiseInverse();
    extinctionCoeff = tScatterCoeffSum + tAbsorptionCoeff;
}

void
CelestiaGLProgram::setMVPMatrices(const Eigen::Matrix4f& p, const Eigen::Matrix4f& m)
{
    ProjectionMatrix = p;
    ModelViewMatrix = m;
    MVPMatrix = p * m;
}
