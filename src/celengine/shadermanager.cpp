// shadermanager.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <Eigen/Geometry>
#include <celcompat/filesystem.h>
#include <celmath/geomutil.h>
#include <celutil/debug.h>
#include "glsupport.h"
#include "vecgl.h"
#include "shadermanager.h"
#include "shadowmap.h"

using namespace celestia;
using namespace Eigen;
using namespace std;

// GLSL on Mac OS X appears to have a bug that precludes us from using structs
#define USE_GLSL_STRUCTS
#define POINT_FADE 1

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

static const char* errorVertexShaderSource =
    "attribute vec4 in_Position;\n"
    "void main(void) {\n"
    "   gl_Position = MVPMatrix * in_Position;\n"
    "}\n";
static const char* errorFragmentShaderSource =
    "void main(void) {\n"
    "   gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
    "}\n";


#ifndef GL_ES
static const char* CommonHeader = "#version 120\n";
#else
static const char* CommonHeader = "#version 100\nprecision highp float;\n";
#endif
static const char* VertexHeader = R"glsl(
uniform mat4 ModelViewMatrix;
uniform mat4 ProjectionMatrix;
uniform mat4 MVPMatrix;

invariant gl_Position;
)glsl";

static const char* FragmentHeader = "";

static const char* CommonAttribs = R"glsl(
attribute vec4 in_Position;
attribute vec3 in_Normal;
attribute vec4 in_TexCoord0;
attribute vec4 in_TexCoord1;
attribute vec4 in_TexCoord2;
attribute vec4 in_TexCoord3;
attribute vec4 in_Color;
)glsl";

bool
ShaderProperties::usesShadows() const
{
    return shadowCounts != 0;
}


bool
ShaderProperties::usesFragmentLighting() const
{
    return (texUsage & NormalTexture) != 0 || lightModel == PerPixelSpecularModel;
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
    switch (lightModel)
    {
    case SpecularModel:
    case PerPixelSpecularModel:
        return true;
    default:
        return false;
    }
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


bool operator<(const ShaderProperties& p0, const ShaderProperties& p1)
{
    if (p0.texUsage < p1.texUsage)
        return true;
    if (p1.texUsage < p0.texUsage)
        return false;

    if (p0.nLights < p1.nLights)
        return true;
    if (p1.nLights < p0.nLights)
        return false;

    if (p0.shadowCounts < p1.shadowCounts)
        return true;
    if (p1.shadowCounts < p0.shadowCounts)
        return false;

    if (p0.effects < p1.effects)
        return true;
    if (p1.effects < p0.effects)
        return false;

    return (p0.lightModel < p1.lightModel);
}


ShaderManager::ShaderManager()
{
#if defined(_DEBUG) || defined(DEBUG) || 1
    // Only write to shader log file if this is a debug build

    if (g_shaderLogFile == nullptr)
#ifdef _WIN32
        g_shaderLogFile = new ofstream("shaders.log");
#else
        g_shaderLogFile = new ofstream("/tmp/celestia-shaders.log");
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
ShaderManager::getShader(const string& name, const string& vs, const string& fs)
{
    auto iter = staticShaders.find(name);
    if (iter != staticShaders.end())
    {
        // Shader already exists
        return iter->second;
    }

    // Create a new shader and add it to the table of created shaders
    CelestiaGLProgram* prog = buildProgram(vs, fs);
    staticShaders[name] = prog;

    return prog;
}

CelestiaGLProgram*
ShaderManager::getShader(const string& name)
{
    auto iter = staticShaders.find(name);
    if (iter != staticShaders.end())
    {
        // Shader already exists
        return iter->second;
    }

    fs::path dir("shaders");
    auto vsName = dir / fmt::sprintf("%s_vert.glsl", name);
    auto fsName = dir / fmt::sprintf("%s_frag.glsl", name);

    std::error_code ecv, ecf;
    uintmax_t vsSize = fs::file_size(vsName, ecv);
    uintmax_t fsSize = fs::file_size(fsName, ecf);
    if (ecv || ecf)
    {
        fmt::fprintf(cerr, "Failed to get file size of %s or %s\n", vsName, fsName);
        return getShader(name, errorVertexShaderSource, errorFragmentShaderSource);
    }

    ifstream vsf(vsName.string());
    if (!vsf.good())
    {
        fmt::fprintf(cerr, "Failed to open %s\n", vsName);
        return getShader(name, errorVertexShaderSource, errorFragmentShaderSource);
    }

    ifstream fsf(fsName.string());
    if (!fsf.good())
    {
        fmt::fprintf(cerr, "Failed to open %s\n", fsName);
        return getShader(name, errorVertexShaderSource, errorFragmentShaderSource);
    }

    string vs(vsSize, '\0'), fs(fsSize, '\0');

    vsf.read(&vs[0], vsSize);
    fsf.read(&fs[0], fsSize);

    // Create a new shader and add it to the table of created shaders
    CelestiaGLProgram* prog = buildProgram(vs, fs);
    staticShaders[name] = prog;

    return prog;
}

static string
LightProperty(unsigned int i, const char* property)
{
#ifndef USE_GLSL_STRUCTS
    return fmt::sprintf("light%d_%s", i, property);
#else
    return fmt::sprintf("lights[%d].%s", i, property);
#endif
}


static string
FragLightProperty(unsigned int i, const char* property)
{
    return fmt::sprintf("light%s%d", property, i);
}

#if 0
static string
IndexedParameter(const char* name, unsigned int index)
{
    return fmt::sprintf("%s%d", name, index);
}
#endif

static const string
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

static string
IndexedParameter(const char* name, unsigned int index0)
{
    return fmt::sprintf("%s%d", name, index0);
}

static string
IndexedParameter(const char* name, unsigned int index0, unsigned int index1)
{
    return fmt::sprintf("%s%d_%d", name, index0, index1);
}


class Sh_ExpressionContents
{
protected:
    Sh_ExpressionContents() = default;
    virtual ~Sh_ExpressionContents() = default;

public:
    virtual string toString() const = 0;
    virtual int precedence() const = 0;
    string toStringPrecedence(int parentPrecedence) const
    {
        if (parentPrecedence >= precedence())
            return string("(") + toString() + ")";
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
    string toString() const override
    {
        return fmt::sprintf("%f", m_value);
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

    string toString() const
    {
        return m_contents->toString();
    }

    string toStringPrecedence(int parentPrecedence) const
    {
        return m_contents->toStringPrecedence(parentPrecedence);
    }

    int precedence() const
    {
        return m_contents->precedence();
    }

    // [] operator acts like swizzle
    Sh_Expression operator[](const string& components) const;

private:
    const Sh_ExpressionContents* m_contents;
};


class Sh_VariableExpression : public Sh_ExpressionContents
{
public:
    Sh_VariableExpression(const string& name) : m_name(name) {}
    string toString() const override
    {
        return m_name;
    }

    int precedence() const override { return 100; }

private:
    const string m_name;
};

class Sh_SwizzleExpression : public Sh_ExpressionContents
{
public:
    Sh_SwizzleExpression(const Sh_Expression& expr, const string& components) :
        m_expr(expr),
        m_components(components)
    {
    }

    string toString() const override
    {
        return m_expr.toStringPrecedence(precedence()) + "." + m_components;
    }

    int precedence() const override { return 99; }

private:
    const Sh_Expression m_expr;
    const string m_components;
};

class Sh_BinaryExpression : public Sh_ExpressionContents
{
public:
    Sh_BinaryExpression(const string& op, int precedence, const Sh_Expression& left, const Sh_Expression& right) :
        m_op(op),
        m_precedence(precedence),
        m_left(left),
        m_right(right) {};

    string toString() const override
    {
        return left().toStringPrecedence(precedence()) + op() + right().toStringPrecedence(precedence());
    }

    const Sh_Expression& left() const { return m_left; }
    const Sh_Expression& right() const { return m_right; }
    string op() const { return m_op; }
    int precedence() const override { return m_precedence; }

private:
    string m_op;
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

Sh_Expression Sh_Expression::operator[](const string& components) const
{
    return new Sh_SwizzleExpression(*this, components);
}

class Sh_UnaryFunctionExpression : public Sh_ExpressionContents
{
public:
    Sh_UnaryFunctionExpression(const string& name, const Sh_Expression& arg0) :
        m_name(name), m_arg0(arg0) {}

    string toString() const override
    {
        return m_name + "(" + m_arg0.toString() + ")";
    }

    int precedence() const override { return 100; }

private:
    string m_name;
    const Sh_Expression m_arg0;
};

class Sh_BinaryFunctionExpression : public Sh_ExpressionContents
{
public:
    Sh_BinaryFunctionExpression(const string& name, const Sh_Expression& arg0, const Sh_Expression& arg1) :
        m_name(name), m_arg0(arg0), m_arg1(arg1) {}

    string toString() const override
    {
        return m_name + "(" + m_arg0.toString() + ", " + m_arg1.toString() + ")";
    }

    int precedence() const override { return 100; }

private:
    string m_name;
    const Sh_Expression m_arg0;
    const Sh_Expression m_arg1;
};

class Sh_TernaryFunctionExpression : public Sh_ExpressionContents
{
public:
    Sh_TernaryFunctionExpression(const string& name, const Sh_Expression& arg0, const Sh_Expression& arg1, const Sh_Expression& arg2) :
        m_name(name), m_arg0(arg0), m_arg1(arg1), m_arg2(arg2) {}

    string toString() const override
    {
        return m_name + "(" + m_arg0.toString() + ", " + m_arg1.toString() + ", " + m_arg2.toString() + ")";
    }

    int precedence() const override { return 100; }

private:
    string m_name;
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

Sh_Expression sampler2D(const string& name)
{
    return new Sh_VariableExpression(name);
}

Sh_Expression sh_vec3(const string& name)
{
    return new Sh_VariableExpression(name);
}

Sh_Expression sh_vec4(const string& name)
{
    return new Sh_VariableExpression(name);
}

Sh_Expression sh_float(const string& name)
{
    return new Sh_VariableExpression(name);
}

Sh_Expression indexedUniform(const string& name, unsigned int index0)
{
    string buf;
    buf = fmt::sprintf("%s%u", name.c_str(), index0);
    return new Sh_VariableExpression(buf);
}

Sh_Expression ringShadowTexCoord(unsigned int index)
{
    string buf;
    buf = fmt::sprintf("ringShadowTexCoord.%c", "xyzw"[index]);
    return new Sh_VariableExpression(buf);
}

Sh_Expression cloudShadowTexCoord(unsigned int index)
{
    string buf;
    buf = fmt::sprintf("cloudShadowTexCoord%d", index);
    return new Sh_VariableExpression(string(buf));
}

static string
DeclareUniform(const std::string& name, ShaderVariableType type)
{
    return string("uniform ") + ShaderTypeString(type) + " " + name + ";\n";
}

static string
DeclareVarying(const std::string& name, ShaderVariableType type)
{
    return string("varying ") + ShaderTypeString(type) + " " + name + ";\n";
}

static string
DeclareAttribute(const std::string& name, ShaderVariableType type)
{
    return string("attribute ") + ShaderTypeString(type) + " " + name + ";\n";
}

static string
DeclareLocal(const std::string& name, ShaderVariableType type)
{
    return ShaderTypeString(type) + " " + name + ";\n";
}

static string
DeclareLocal(const std::string& name, ShaderVariableType type, const Sh_Expression& expr)
{
    return ShaderTypeString(type) + " " + name + " = " + expr.toString() + ";\n";
}

string assign(const string& variableName, const Sh_Expression& expr)
{
    return variableName + " = " + expr.toString() + ";\n";
}

string addAssign(const string& variableName, const Sh_Expression& expr)
{
    return variableName + " += " + expr.toString() + ";\n";
}

string mulAssign(const string& variableName, const Sh_Expression& expr)
{
    return variableName + " *= " + expr.toString() + ";\n";
}



static string
RingShadowTexCoord(unsigned int index)
{
    return fmt::sprintf("ringShadowTexCoord.%c", "xyzw"[index]);
}


static string
CloudShadowTexCoord(unsigned int index)
{
    return fmt::sprintf("cloudShadowTexCoord%d", index);
}


static string
VarScatterInVS()
{
    return "v_ScatterColor";
}

static string
VarScatterInFS()
{
    return "v_ScatterColor";
}


static void
DumpShaderSource(ostream& out, const std::string& source)
{
    bool newline = true;
    unsigned int lineNumber = 0;

    for (unsigned int i = 0; i < source.length(); i++)
    {
        if (newline)
        {
            lineNumber++;
            out << setw(3) << lineNumber << ": ";
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

static string
DeclareLights(const ShaderProperties& props)
{
    if (props.nLights == 0)
        return {};

    ostringstream stream;
#ifndef USE_GLSL_STRUCTS
    for (unsigned int i = 0; i < props.nLights; i++)
    {
        stream << "uniform vec3 light" << i << "_direction;\n";
        stream << "uniform vec3 light" << i << "_diffuse;\n";
        if (props.hasSpecular())
        {
            stream << "uniform vec3 light" << i << "_specular;\n";
            stream << "uniform vec3 light" << i << "_halfVector;\n";
        }
        if (props.texUsage & ShaderProperties::NightTexture)
            stream << "uniform float light" << i << "_brightness;\n";
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


static string
SeparateDiffuse(unsigned int i)
{
    // Used for packing multiple diffuse factors into the diffuse color.
    return fmt::sprintf("diffFactors.%c", "xyzw"[i & 3]);
}


static string
SeparateSpecular(unsigned int i)
{
    // Used for packing multiple specular factors into the specular color.
    return fmt::sprintf("specFactors.%c", "xyzw"[i & 3]);
}


// Used by rings shader
static string
ShadowDepth(unsigned int i)
{
    return fmt::sprintf("shadowDepths.%c", "xyzw"[i & 3]);
}


static string
TexCoord2D(unsigned int i)
{
    return fmt::sprintf("in_TexCoord%d.st", i);
}


// Tangent space light direction
static string
LightDir_tan(unsigned int i)
{
    return fmt::sprintf("lightDir_tan_%d", i);
}


static string
LightHalfVector(unsigned int i)
{
    return fmt::sprintf("lightHalfVec%d", i);
}


static string
ScatteredColor(unsigned int i)
{
    return fmt::sprintf("scatteredColor%d", i);
}


static string
TangentSpaceTransform(const string& dst, const string& src)
{
    string source;

    source += dst + ".x = dot(in_Tangent, " + src + ");\n";
    source += dst + ".y = dot(-bitangent, " + src + ");\n";
    source += dst + ".z = dot(in_Normal, " + src + ");\n";

    return source;
}


static string
NightTextureBlend()
{
#if 1
    // Output the blend factor for night lights textures
    return string("totalLight = 1.0 - totalLight;\n"
                  "totalLight = totalLight * totalLight * totalLight * totalLight;\n");
#else
    // Alternate night light blend function; much sharper falloff near terminator.
    return string("totalLight = clamp(10.0 * (0.1 - totalLight), 0.0, 1.0);\n");
#endif
}


// Return true if the color sum from all light sources should be computed in
// the vertex shader, and false if it will be done by the pixel shader.
static bool
VSComputesColorSum(const ShaderProperties& props)
{
    return !props.usesShadows() && props.lightModel != ShaderProperties::PerPixelSpecularModel;
}


static string
AssignDiffuse(unsigned int lightIndex, const ShaderProperties& props)
{
    if (VSComputesColorSum(props))
        return string("diff.rgb += ") + LightProperty(lightIndex, "diffuse") + " * ";
    else
        return SeparateDiffuse(lightIndex)  + " = ";
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

static string
AddDirectionalLightContrib(unsigned int i, const ShaderProperties& props)
{
    string source;

    if (props.lightModel == ShaderProperties::ParticleDiffuseModel)
    {
        // The ParticleDiffuse model doesn't use a surface normal; vertices
        // are lit as if they were infinitesimal spherical particles,
        // unaffected by light direction except when considering shadows.
        source += "NL = 1.0;\n";
    }
    else
    {
        source += "NL = max(0.0, dot(in_Normal, " +
            LightProperty(i, "direction") + "));\n";
    }

    if (props.lightModel == ShaderProperties::SpecularModel)
    {
        source += "H = normalize(" + LightProperty(i, "direction") + " + normalize(eyePosition - in_Position.xyz));\n";
        source += "NH = max(0.0, dot(in_Normal, H));\n";

        // The calculation below uses the infinite viewer approximation. It's slightly faster,
        // but results in less accurate rendering.
        // source += "NH = max(0.0, dot(in_Normal, " + LightProperty(i, "halfVector") + "));\n";
    }

    if (props.usesTangentSpaceLighting())
    {
        source += TangentSpaceTransform(LightDir_tan(i), LightProperty(i, "direction"));
        // Diffuse color is computed in the fragment shader
    }
    else if (props.lightModel == ShaderProperties::PerPixelSpecularModel)
    {
        source += SeparateDiffuse(i) + " = NL;\n";
        // Specular is computed in the fragment shader; half vectors are required
        // for the calculation
        source += LightHalfVector(i) + " = " + LightProperty(i, "direction") + " + eyeDir;\n";
    }
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
    else if (props.lightModel == ShaderProperties::LunarLambertModel)
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


static string
BeginLightSourceShadows(const ShaderProperties& props, unsigned int light)
{
    string source;

    if (props.usesTangentSpaceLighting())
    {
        if (props.hasShadowsForLight(light))
            source += "shadow = 1.0;\n";
    }
    else
    {
        source += "shadow = " + SeparateDiffuse(light) + ";\n";
    }

    if (props.hasRingShadowForLight(light))
    {
        if (gl::ARB_shader_texture_lod)
        {
            source += mulAssign("shadow",
                      (1.0f - texture2DLod(sampler2D("ringTex"), vec2(ringShadowTexCoord(light), 0.0f), indexedUniform("ringShadowLOD", light))["a"]));
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
                      (1.0f - texture2DLodBias(sampler2D("ringTex"), vec2(ringShadowTexCoord(light), 0.0f), indexedUniform("ringShadowLOD", light))["a"]));
        }
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
static string
Shadow(unsigned int light, unsigned int shadow)
{
    string source;

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


static string
ShadowsForLightSource(const ShaderProperties& props, unsigned int light)
{
    string source = BeginLightSourceShadows(props, light);

    for (unsigned int i = 0; i < props.getEclipseShadowCountForLight(light); i++)
        source += Shadow(light, i);

    return source;
}


static string
ScatteringPhaseFunctions(const ShaderProperties& /*unused*/)
{
    string source;

    // Evaluate the Mie and Rayleigh phase functions; both are functions of the cosine
    // of the angle between the view vector and light vector
    source += "    float phMie = (1.0 - mieK * mieK) / ((1.0 - mieK * cosTheta) * (1.0 - mieK * cosTheta));\n";

    // Ignore Rayleigh phase function and treat Rayleigh scattering as isotropic
    // source += "    float phRayleigh = (1.0 + cosTheta * cosTheta);\n";
    source += "    float phRayleigh = 1.0;\n";

    return source;
}


static string
AtmosphericEffects(const ShaderProperties& props)
{
    string source;

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

    string scatter;
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

        source += "    " + VarScatterInVS() + " = (phRayleigh * rayleighCoeff + phMie * mieCoeff) * invScatterCoeffSum * sunColor * " + scatter + ";\n";
    }


    // Optional exposure control
    //source += "    1.0 - (scatterIn * exp(-5.0 * max(scatterIn.x, max(scatterIn.y, scatterIn.z))));\n";

    source += "}\n";

    return source;
}


#if 0
// Integrate the atmosphere by summation--slow, but higher quality
static string
AtmosphericEffects(const ShaderProperties& props, unsigned int nSamples)
{
    string source;

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
    source += "    vec3 scatter = vec3(0.0, 0.0, 0.0);\n";
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

        source += "    " + VarScatterInVS() + " = (phRayleigh * rayleighCoeff + phMie * mieCoeff) * invScatterCoeffSum * scatter;\n";
    }
    // Optional exposure control
    //source += "    1.0 - (" + VarScatterInVS() + " * exp(-5.0 * max(scatterIn.x, max(scatterIn.y, scatterIn.z))));\n";

    source += "}\n";

    return source;
}
#endif


string
ScatteringConstantDeclarations(const ShaderProperties& /*props*/)
{
    string source;

    source += "uniform vec3 atmosphereRadius;\n";
    source += "uniform float mieCoeff;\n";
    source += "uniform float mieH;\n";
    source += "uniform float mieK;\n";
    source += "uniform vec3 rayleighCoeff;\n";
    source += "uniform float rayleighH;\n";
    source += "uniform vec3 scatterCoeffSum;\n";
    source += "uniform vec3 invScatterCoeffSum;\n";
    source += "uniform vec3 extinctionCoeff;\n";

    return source;
}


string
TextureSamplerDeclarations(const ShaderProperties& props)
{
    string source;

    // Declare texture samplers
    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        source += "uniform sampler2D diffTex;\n";
    }

    if (props.texUsage & ShaderProperties::NormalTexture)
    {
        source += "uniform sampler2D normTex;\n";
    }

    if (props.texUsage & ShaderProperties::SpecularTexture)
    {
        source += "uniform sampler2D specTex;\n";
    }

    if (props.texUsage & ShaderProperties::NightTexture)
    {
        source += "uniform sampler2D nightTex;\n";
    }

    if (props.texUsage & ShaderProperties::EmissiveTexture)
    {
        source += "uniform sampler2D emissiveTex;\n";
    }

    if (props.texUsage & ShaderProperties::OverlayTexture)
    {
        source += "uniform sampler2D overlayTex;\n";
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


string
TextureCoordDeclarations(const ShaderProperties& props)
{
    string source;

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
            source += "varying vec2 diffTexCoord;\n";
        }
    }
    else
    {
        if (props.texUsage & ShaderProperties::DiffuseTexture)
            source += "varying vec2 diffTexCoord;\n";
        if (props.texUsage & ShaderProperties::NormalTexture)
            source += "varying vec2 normTexCoord;\n";
        if (props.texUsage & ShaderProperties::SpecularTexture)
            source += "varying vec2 specTexCoord;\n";
        if (props.texUsage & ShaderProperties::NightTexture)
            source += "varying vec2 nightTexCoord;\n";
        if (props.texUsage & ShaderProperties::EmissiveTexture)
            source += "varying vec2 emissiveTexCoord;\n";
        if (props.texUsage & ShaderProperties::OverlayTexture)
            source += "varying vec2 overlayTexCoord;\n";
    }

    if (props.texUsage & ShaderProperties::ShadowMapTexture)
    {
        source += DeclareVarying("shadowTexCoord0", Shader_Vector4);
        source += DeclareVarying("cosNormalLightDir", Shader_Float);
    }

    return source;
}


string
PointSizeCalculation()
{
    string source;
    source += "float ptSize = pointScale * in_PointSize / length(vec3(ModelViewMatrix * in_Position));\n";
    source += "pointFade = min(1.0, ptSize * ptSize);\n";
    source += "gl_PointSize = ptSize;\n";

    return source;
}

static string
CalculateShadow()
{
    string source;
#if GL_ONLY_SHADOWS
    source += R"glsl(
float calculateShadow()
{
    float texelSize = 1.0 / shadowMapSize;
    float s = 0.0;
    float bias = max(0.005 * (1.0 - cosNormalLightDir), 0.0005);
)glsl";
    float boxFilterWidth = (float) ShadowSampleKernelWidth - 1.0f;
    float firstSample = -boxFilterWidth / 2.0f;
    float lastSample = firstSample + boxFilterWidth;
    float sampleWeight = 1.0f / (float) (ShadowSampleKernelWidth * ShadowSampleKernelWidth);
    source += fmt::sprintf("    for (float y = %f; y <= %f; y += 1.0)\n", firstSample, lastSample);
    source += fmt::sprintf("        for (float x = %f; x <= %f; x += 1.0)\n", firstSample, lastSample);
    source += "            s += shadow2D(shadowMapTex0, shadowTexCoord0.xyz + vec3(x * texelSize, y * texelSize, bias)).z;\n";
    source += fmt::sprintf("    return s * %f;\n", sampleWeight);
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
)glsl";
#endif
    return source;
}

GLVertexShader*
ShaderManager::buildVertexShader(const ShaderProperties& props)
{
    string source(CommonHeader);
    source += VertexHeader;
    source += CommonAttribs;

    source += DeclareLights(props);
    if (props.lightModel == ShaderProperties::SpecularModel)
        source += "uniform float shininess;\n";

    source += "uniform vec3 eyePosition;\n";

    source += TextureCoordDeclarations(props);
    source += "uniform float textureOffset;\n";

    if (props.hasScattering())
    {
        source += ScatteringConstantDeclarations(props);
    }

    if (props.texUsage & ShaderProperties::PointSprite)
    {
        source += DeclareUniform("pointScale", Shader_Float);
        source += DeclareAttribute("in_PointSize", Shader_Float);
        source += DeclareVarying("pointFade", Shader_Float);
    }

    if (props.usesTangentSpaceLighting())
    {
        source += "attribute vec3 in_Tangent;\n";
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            source += "varying vec3 " + LightDir_tan(i) + ";\n";
        }

        if (props.isViewDependent() &&
            props.lightModel != ShaderProperties::SpecularModel)
        {
            source += "varying vec3 eyeDir_tan;\n";
        }
    }
    else if (props.lightModel == ShaderProperties::PerPixelSpecularModel)
    {
        source += "varying vec4 diffFactors;\n";
        source += "varying vec3 normal;\n";
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            source += "varying vec3 " + LightHalfVector(i) + ";\n";
        }
    }
    else if (props.usesShadows())
    {
        source += "varying vec4 diffFactors;\n";
        if (props.lightModel == ShaderProperties::SpecularModel)
        {
            source += "varying vec4 specFactors;\n";
        }
    }
    else
    {
        if (props.lightModel != ShaderProperties::UnlitModel)
        {
            source += "uniform vec3 ambientColor;\n";
            source += "uniform float opacity;\n";
        }
        source += "varying vec4 diff;\n";
        if (props.lightModel == ShaderProperties::SpecularModel)
        {
            source += "varying vec4 spec;\n";
        }
    }

    // If this shader uses tangent space lighting, the diffuse term
    // will be calculated in the fragment shader and we won't need
    // the lunar-Lambert term here in the vertex shader.
    if (!props.usesTangentSpaceLighting())
    {
        if (props.lightModel == ShaderProperties::LunarLambertModel)
            source += "uniform float lunarLambert;\n";
    }

    // Miscellaneous lighting values
    if ((props.texUsage & ShaderProperties::NightTexture) && VSComputesColorSum(props))
    {
        source += "varying float totalLight;\n";
    }

    if (props.hasScattering())
    {
        //source += "varying vec3 scatterIn;\n";
        source += "varying vec3 scatterEx;\n";
        source += DeclareVarying(VarScatterInVS(), Shader_Vector3);
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
        source += "uniform float cloudShadowTexOffset;\n";
        source += "uniform float cloudHeight;\n";
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            if (props.hasCloudShadowForLight(i))
            {
                source += "varying vec2 " + CloudShadowTexCoord(i) + ";\n";
            }
        }
    }

    if (props.hasShadowMap())
        source += "uniform mat4 ShadowMatrix0;\n";

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
    else if (props.lightModel == ShaderProperties::PerPixelSpecularModel)
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
            source += "spec = vec4(0.0, 0.0, 0.0, 0.0);\n";
    }

    if (props.hasShadowMap())
    {
        source += "cosNormalLightDir = dot(in_Normal, lights[0].direction);\n";
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

    if ((props.texUsage & ShaderProperties::PointSprite) != 0)
        source += PointSizeCalculation();

    if (props.hasShadowMap())
        source += "shadowTexCoord0 = ShadowMatrix0 * vec4(in_Position.xyz, 1.0);\n";

    source += "gl_Position = MVPMatrix * in_Position;\n";
    source += "}\n";

    DumpVSSource(source);

    GLVertexShader* vs = nullptr;
    GLShaderStatus status = GLShaderLoader::CreateVertexShader(source, &vs);
    return status == ShaderStatus_OK ? vs : nullptr;
}


GLFragmentShader*
ShaderManager::buildFragmentShader(const ShaderProperties& props)
{
    string source(CommonHeader);

    // Without GL_ARB_shader_texture_lod enabled one can use texture2DLod
    // in vertext shaders only
    if (gl::ARB_shader_texture_lod)
        source += "#extension GL_ARB_shader_texture_lod : enable\n";

    string diffTexCoord("diffTexCoord");
    string specTexCoord("specTexCoord");
    string nightTexCoord("nightTexCoord");
    string emissiveTexCoord("emissiveTexCoord");
    string normTexCoord("normTexCoord");
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
        source += "uniform vec3 ambientColor;\n";
        source += "uniform float opacity;\n";
        if (props.isViewDependent())
        {
            if (props.lightModel == ShaderProperties::SpecularModel)
            {
                // Specular model is sort of a hybrid: all the view-dependent lighting is
                // handled in the vertex shader, and the fragment shader is view-independent
                source += "varying vec4 specFactors;\n";
                source += "vec4 spec = vec4(0.0);\n";
            }
            else
            {
                source += "varying vec3 eyeDir_tan;\n";  // tangent space eye vector
                source += "vec4 spec = vec4(0.0);\n";
                source += "uniform float shininess;\n";
            }
        }

        if (props.lightModel == ShaderProperties::LunarLambertModel)
            source += "uniform float lunarLambert;\n";

        for (unsigned int i = 0; i < props.nLights; i++)
        {
            source += "varying vec3 " + LightDir_tan(i) + ";\n";
            source += "uniform vec3 " + FragLightProperty(i, "color") + ";\n";
            if (props.hasSpecular())
            {
                source += "uniform vec3 " + FragLightProperty(i, "specColor") + ";\n";
            }
            if (props.texUsage & ShaderProperties::NightTexture)
            {
                source += "uniform float " + FragLightProperty(i, "brightness") + ";\n";
            }
        }
    }
    else if (props.lightModel == ShaderProperties::PerPixelSpecularModel)
    {
        source += "uniform vec3 ambientColor;\n";
        source += "uniform float opacity;\n";
        source += "varying vec4 diffFactors;\n";
        source += "varying vec3 normal;\n";
        source += "vec4 spec = vec4(0.0);\n";
        source += "uniform float shininess;\n";

        for (unsigned int i = 0; i < props.nLights; i++)
        {
            source += "varying vec3 " + LightHalfVector(i) + ";\n";
            source += "uniform vec3 " + FragLightProperty(i, "color") + ";\n";
            source += "uniform vec3 " + FragLightProperty(i, "specColor") + ";\n";
        }
    }
    else if (props.usesShadows())
    {
        source += "uniform vec3 ambientColor;\n";
        source += "uniform float opacity;\n";
        source += "varying vec4 diffFactors;\n";
        if (props.lightModel == ShaderProperties::SpecularModel)
        {
            source += "varying vec4 specFactors;\n";
            source += "vec4 spec = vec4(0.0);\n";
        }
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            source += "uniform vec3 " + FragLightProperty(i, "color") + ";\n";
            if (props.lightModel == ShaderProperties::SpecularModel)
                source += "uniform vec3 " + FragLightProperty(i, "specColor") + ";\n";
        }
    }
    else
    {
        source += "varying vec4 diff;\n";
        if (props.lightModel == ShaderProperties::SpecularModel)
        {
            source += "varying vec4 spec;\n";
        }
    }

    if (props.hasScattering())
    {
        //source += "varying vec3 scatterIn;\n";
        source += "varying vec3 scatterEx;\n";
        source += DeclareVarying(VarScatterInFS(), Shader_Vector3);
    }

    if ((props.texUsage & ShaderProperties::NightTexture))
    {
#ifdef USE_HDR
        source += "uniform float nightLightScale;\n";
#endif
        if (VSComputesColorSum(props))
        {
            source += "varying float totalLight;\n";
        }
    }

    // Declare shadow parameters
    if (props.shadowCounts != 0)
    {
        source += "varying vec3 position_obj;\n";
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            for (unsigned int j = 0; j < props.getEclipseShadowCountForLight(i); j++)
            {
                source += "uniform vec4 " +
                    IndexedParameter("shadowTexGenS", i, j) + ";\n";
                source += "uniform vec4 " +
                    IndexedParameter("shadowTexGenT", i, j) + ";\n";
                source += "uniform float " +
                    IndexedParameter("shadowFalloff", i, j) + ";\n";
                source += "uniform float " +
                    IndexedParameter("shadowMaxDepth", i, j) + ";\n";
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
        source += "uniform sampler2D cloudShadowTex;\n";
        for (unsigned int i = 0; i < props.nLights; i++)
            source += "varying vec2 " + CloudShadowTexCoord(i) + ";\n";
    }

    if (props.texUsage & ShaderProperties::PointSprite)
    {
        source += DeclareVarying("pointFade", Shader_Float);
    }

    if (props.hasShadowMap())
    {
        source += DeclareUniform("shadowMapSize", Shader_Float);
        source += CalculateShadow();
    }

    source += "\nvoid main(void)\n{\n";
    source += "vec4 color;\n";
    if (props.usesTangentSpaceLighting() ||
        props.lightModel == ShaderProperties::PerPixelSpecularModel ||
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
                source += "n.xy = texture2D(normTex, " + normTexCoord + ".st).ag * 2.0 - vec2(1.0, 1.0);\n";
                source += "n.z = sqrt(1.0 - n.x * n.x - n.y * n.y);\n";
            }
            else
            {
                // TODO: normalizing the filtered normal texture value noticeably improves the appearance; add
                // an option for this.
                //source += "vec3 n = normalize(texture2D(normTex, " + normTexCoord + ".st).xyz * 2.0 - vec3(1.0, 1.0, 1.0));\n";
                source += "vec3 n = texture2D(normTex, " + normTexCoord + ".st).xyz * 2.0 - vec3(1.0, 1.0, 1.0);\n";
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

            if (props.lightModel == ShaderProperties::PerPixelSpecularModel)
            {
                source += "vec3 H;\n";
                source += "float NH;\n";
            }
            else if (props.lightModel == ShaderProperties::LunarLambertModel)
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
            if (props.lightModel == ShaderProperties::LunarLambertModel)
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

            string illum;
            if (props.hasShadowsForLight(i))
                illum = string("l * shadow");
            else
                illum = string("l");

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
            else if (props.lightModel == ShaderProperties::PerPixelSpecularModel)
            {
                source += "H = normalize(eyeDir_tan + " + LightDir_tan(i) + ");\n";
                source += "NH = max(0.0, dot(n, H));\n";
                source += "spec.rgb += " + illum + " * pow(NH, shininess) * " + FragLightProperty(i, "specColor") + ";\n";
            }
        }
    }
    else if (props.lightModel == ShaderProperties::PerPixelSpecularModel)
    {
        source += "float NH;\n";
        source += "vec3 n = normalize(normal);\n";
        source += "float shadowMapCoeff = 1.0;\n";

        // Sum the contributions from each light source
        for (unsigned i = 0; i < props.nLights; i++)
        {
            string illum;

            if (props.hasShadowsForLight(i))
                illum = string("shadow");
            else
                illum = SeparateDiffuse(i);

            if (props.hasShadowsForLight(i))
                source += ShadowsForLightSource(props, i);

            source += "diff.rgb += " + illum + " * " + FragLightProperty(i, "color") + ";\n";
            source += "NH = max(0.0, dot(n, normalize(" + LightHalfVector(i) + ")));\n";
            source += "spec.rgb += " + illum + " * pow(NH, shininess) * " + FragLightProperty(i, "specColor") + ";\n";
            if (props.hasShadowMap() && i == 0)
            {
                source += "if (cosNormalLightDir > 0.0)\n{\n";
                source += "    shadowMapCoeff = calculateShadow();\n";
                source += "    diff.rgb *= shadowMapCoeff;\n";
                source += "    spec.rgb *= shadowMapCoeff;\n";
                source += "}\n";
            }
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
            {
                source += "if (cosNormalLightDir > 0.0)\n{\n";
                source += "    shadowMapCoeff = calculateShadow();\n";
                source += "    diff.rgb *= shadowMapCoeff;\n";
                if (props.lightModel == ShaderProperties::SpecularModel)
                    source += "    spec.rgb *= shadowMapCoeff;\n";
                source += "}\n";
            }
        }
    }

    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        if (props.texUsage & ShaderProperties::PointSprite)
            source += "color = texture2D(diffTex, gl_PointCoord);\n";
        else
            source += "color = texture2D(diffTex, " + diffTexCoord + ".st);\n";
    }
    else
    {
        source += "color = vec4(1.0, 1.0, 1.0, 1.0);\n";
    }

#if POINT_FADE
    if (props.texUsage & ShaderProperties::PointSprite)
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
            source += "gl_FragColor = color * diff + float(color.a) * spec;\n";
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

#ifdef USE_HDR
        source += "gl_FragColor += texture2D(nightTex, " + nightTexCoord + ".st) * totalLight * nightLightScale;\n";
#else
        source += "gl_FragColor += texture2D(nightTex, " + nightTexCoord + ".st) * totalLight;\n";
#endif
    }

    if (props.texUsage & ShaderProperties::EmissiveTexture)
    {
        source += "gl_FragColor += texture2D(emissiveTex, " + emissiveTexCoord + ".st);\n";
    }

    // Include the effect of atmospheric scattering.
    if (props.hasScattering())
    {
        source += "gl_FragColor.rgb = gl_FragColor.rgb * scatterEx + " + VarScatterInFS() + ";\n";
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
    string source(CommonHeader);
    source += VertexHeader;
    source += CommonAttribs;

    source += DeclareLights(props);
    source += "uniform vec3 eyePosition;\n";

    source += "varying vec4 diffFactors;\n";

    if (props.texUsage & ShaderProperties::DiffuseTexture)
        source += "varying vec2 diffTexCoord;\n";

    if (props.shadowCounts != 0)
    {
        source += "varying vec3 position_obj;\n";
        source += "varying vec4 shadowDepths;\n";
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

    source += "gl_Position = MVPMatrix * in_Position;\n";
    source += "}\n";

    DumpVSSource(source);

    GLVertexShader* vs = nullptr;
    GLShaderStatus status = GLShaderLoader::CreateVertexShader(source, &vs);
    return status == ShaderStatus_OK ? vs : nullptr;
}


GLFragmentShader*
ShaderManager::buildRingsFragmentShader(const ShaderProperties& props)
{
    string source(CommonHeader);

    source += "uniform vec3 ambientColor;\n";
    source += "vec4 diff = vec4(ambientColor, 1.0);\n";
    for (unsigned int i = 0; i < props.nLights; i++)
        source += "uniform vec3 " + FragLightProperty(i, "color") + ";\n";

    source += "varying vec4 diffFactors;\n";

    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        source += "varying vec2 diffTexCoord;\n";
        source += "uniform sampler2D diffTex;\n";
    }

    if (props.hasEclipseShadows())
    {
        source += "varying vec3 position_obj;\n";
        source += "varying vec4 shadowDepths;\n";

        for (unsigned int i = 0; i < props.nLights; i++)
        {
            for (unsigned int j = 0; j < props.getEclipseShadowCountForLight(i); j++)
            {
                source += "uniform vec4 " +
                    IndexedParameter("shadowTexGenS", i, j) + ";\n";
                source += "uniform vec4 " +
                    IndexedParameter("shadowTexGenT", i, j) + ";\n";
                source += "uniform float " +
                    IndexedParameter("shadowFalloff", i, j) + ";\n";
                source += "uniform float " +
                    IndexedParameter("shadowMaxDepth", i, j) + ";\n";
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
        source += "color = vec4(1.0, 1.0, 1.0, 1.0);\n";

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
    string source(CommonHeader);
    source += VertexHeader;
    source += CommonAttribs;

    source += DeclareLights(props);

    source += DeclareVarying("position_obj", Shader_Vector3);
    if (props.hasEclipseShadows())
    {
        source += "varying vec4 shadowDepths;\n";
    }

    if (props.texUsage & ShaderProperties::DiffuseTexture)
        source += "varying vec2 diffTexCoord;\n";

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

    source += "gl_Position = MVPMatrix * in_Position;\n";
    source += "}\n";

    DumpVSSource(source);

    GLVertexShader* vs = nullptr;
    GLShaderStatus status = GLShaderLoader::CreateVertexShader(source, &vs);
    return status == ShaderStatus_OK ? vs : nullptr;
}


GLFragmentShader*
ShaderManager::buildRingsFragmentShader(const ShaderProperties& props)
{
    string source(CommonHeader);

    source += "uniform vec3 ambientColor;\n";
    for (unsigned int i = 0; i < props.nLights; i++)
        source += "uniform vec3 " + FragLightProperty(i, "color") + ";\n";

    source += DeclareLights(props);

    source += DeclareUniform("eyePosition", Shader_Vector3);
    source += DeclareVarying("position_obj", Shader_Vector3);

    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        source += "varying vec2 diffTexCoord;\n";
        source += "uniform sampler2D diffTex;\n";
    }

    if (props.hasEclipseShadows())
    {
        source += "varying vec4 shadowDepths;\n";

        for (unsigned int i = 0; i < props.nLights; i++)
        {
            for (unsigned int j = 0; j < props.getEclipseShadowCountForLight(i); j++)
            {
                source += "uniform vec4 " +
                    IndexedParameter("shadowTexGenS", i, j) + ";\n";
                source += "uniform vec4 " +
                    IndexedParameter("shadowTexGenT", i, j) + ";\n";
                source += "uniform float " +
                    IndexedParameter("shadowFalloff", i, j) + ";\n";
                source += "uniform float " +
                    IndexedParameter("shadowMaxDepth", i, j) + ";\n";
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
        source += "color = vec4(1.0, 1.0, 1.0, 1.0);\n";
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
    string source(CommonHeader);
    source += VertexHeader;
    source += CommonAttribs;

    source += DeclareLights(props);
    source += "uniform vec3 eyePosition;\n";
    source += ScatteringConstantDeclarations(props);
    for (unsigned int i = 0; i < props.nLights; i++)
    {
        source += "varying vec3 " + ScatteredColor(i) + ";\n";
    }

    source += "varying vec3 scatterEx;\n";
    source += "varying vec3 eyeDir_obj;\n";

    // Begin main() function
    source += "\nvoid main(void)\n{\n";
    source += "float NL;\n";
    source += "vec3 eyeDir = normalize(eyePosition - in_Position.xyz);\n";
    source += "float NV = dot(in_Normal, eyeDir);\n";

    source += AtmosphericEffects(props);

    source += "eyeDir_obj = eyeDir;\n";
    source += "gl_Position = MVPMatrix * in_Position;\n";
    source += "}\n";

    DumpVSSource(source);

    GLVertexShader* vs = nullptr;
    GLShaderStatus status = GLShaderLoader::CreateVertexShader(source, &vs);
    return status == ShaderStatus_OK ? vs : nullptr;
}


GLFragmentShader*
ShaderManager::buildAtmosphereFragmentShader(const ShaderProperties& props)
{
    string source(CommonHeader);

    source += "varying vec3 scatterEx;\n";
    source += "varying vec3 eyeDir_obj;\n";

    // Scattering constants
    source += "uniform float mieK;\n";
    source += "uniform float mieCoeff;\n";
    source += "uniform vec3  rayleighCoeff;\n";
    source += "uniform vec3  invScatterCoeffSum;\n";

#ifdef USE_GLSL_STRUCTS
    source += DeclareLights(props);
#endif
    unsigned int i;
    for (i = 0; i < props.nLights; i++)
    {
#ifndef USE_GLSL_STRUCTS
        source += "uniform vec3 " + LightProperty(i, "direction") + ";\n";
#endif
        source += "varying vec3 " + ScatteredColor(i) + ";\n";
    }

    source += "\nvoid main(void)\n";
    source += "{\n";

    // Sum the contributions from each light source
    source += "vec3 color = vec3(0.0, 0.0, 0.0);\n";
    source += "vec3 V = normalize(eyeDir_obj);\n";

    // Only do scattering calculations for the primary light source
    // TODO: Eventually handle multiple light sources, and removed the 'min'
    // from the line below.
    for (i = 0; i < min((unsigned int) props.nLights, 1u); i++)
    {
        source += "    float cosTheta = dot(V, " + LightProperty(i, "direction") + ");\n";
        source += ScatteringPhaseFunctions(props);

        // TODO: Consider premultiplying by invScatterCoeffSum
        source += "    color += (phRayleigh * rayleighCoeff + phMie * mieCoeff) * invScatterCoeffSum * " + ScatteredColor(i) + ";\n";
    }

    source += "    gl_FragColor = vec4(color, dot(scatterEx, vec3(0.333, 0.333, 0.333)));\n";
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
    string source(CommonHeader);
    source += VertexHeader;
    source += CommonAttribs;

    source += "uniform float opacity;\n";

    // There are no light sources used for the emissive light model, but
    // we still need the diffuse property of light 0. For other lighting
    // models, the material color is premultiplied with the light color.
    // Emissive shaders interoperate better with other shaders if they also
    // take the color from light source 0.
#ifndef USE_GLSL_STRUCTS
    source += string("uniform vec3 light0_diffuse;\n");
#else
    source += string("uniform struct {\n   vec3 diffuse;\n} lights[1];\n");
#endif

    if (props.texUsage & ShaderProperties::PointSprite)
    {
        source += "uniform float pointScale;\n";
        source += "attribute float in_PointSize;\n";
        source += "varying float pointFade;\n";
    }

    source += DeclareVarying("v_Color", Shader_Vector4);
    source += DeclareVarying("v_TexCoord0", Shader_Vector2);

    // Begin main() function
    source += "\nvoid main(void)\n{\n";

    // Optional texture coordinates (generated automatically for point
    // sprites.)
    if ((props.texUsage & ShaderProperties::DiffuseTexture) &&
        !(props.texUsage & ShaderProperties::PointSprite))
    {
        source += "    v_TexCoord0.st = " + TexCoord2D(0) + ";\n";
    }

    // Set the color.
    string colorSource;
    if (props.texUsage & ShaderProperties::VertexColors)
        colorSource = "in_Color.rgb";
    else
        colorSource = LightProperty(0, "diffuse");

    source += "    v_Color = vec4(" + colorSource + ", opacity);\n";

    // Optional point size
    if ((props.texUsage & ShaderProperties::PointSprite) != 0)
        source += PointSizeCalculation();

    source += "    gl_Position = MVPMatrix * in_Position;\n";

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
    string source(CommonHeader);

    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        source += "uniform sampler2D diffTex;\n";
    }

    if (props.texUsage & ShaderProperties::PointSprite)
    {
        source += "varying float pointFade;\n";
    }

    source += DeclareVarying("v_Color", Shader_Vector4);
    source += DeclareVarying("v_TexCoord0", Shader_Vector2);

    // Begin main()
    source += "\nvoid main(void)\n";
    source += "{\n";

    string colorSource = "v_Color";
    if (props.texUsage & ShaderProperties::PointSprite)
    {
        source += "    vec4 color = v_Color;\n";
#if POINT_FADE
        source += "    color.a *= pointFade;\n";
#endif
        colorSource = "color";
    }

    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        if (props.texUsage & ShaderProperties::PointSprite)
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
    ostringstream source;

    source << CommonHeader;
    source << VertexHeader;
    source << CommonAttribs;

    source << "// PARTICLE SHADER\n";
    source << "// shadow count: " << props.shadowCounts << endl;

    source << DeclareLights(props);

    source << "uniform vec3 eyePosition;\n";

    // TODO: scattering constants

    if (props.texUsage & ShaderProperties::PointSprite)
    {
        source << "uniform float pointScale;\n";
        source << "attribute float in_PointSize;\n";
        source << DeclareVarying("pointFade", Shader_Float);
    }

     source << DeclareVarying("v_Color", Shader_Vector4);

    // Shadow parameters
    if (props.shadowCounts != 0)
    {
        source << "varying vec3 position_obj;\n";
    }

    // Begin main() function
    source << "\nvoid main(void)\n{\n";

#define PARTICLE_PHASE_PARAMETER 0
#if PARTICLE_PHASE_PARAMETER
    float g = -0.4f;
    float miePhaseAsymmetry = 1.55f * g - 0.55f * g * g * g;
    source << "    float mieK = " << miePhaseAsymmetry << ";\n";

    source << "    vec3 eyeDir = normalize(eyePosition - in_Position.xyz);\n";
    source << "    float brightness = 0.0;\n";
    for (unsigned int i = 0; i < min(1u, props.nLights); i++)
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
    if ((props.texUsage & ShaderProperties::PointSprite) != 0)
        source << PointSizeCalculation();

    source << "    gl_Position = MVPMatrix * in_Position;\n";

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
    ostringstream source;

    source << CommonHeader;

    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        source << "uniform sampler2D diffTex;\n";
    }

    if (props.texUsage & ShaderProperties::PointSprite)
    {
        source << DeclareVarying("pointFade", Shader_Float);
    }

    if (props.usesShadows())
    {
        source << "uniform vec3 ambientColor;\n";
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            source << "uniform vec3 " << FragLightProperty(i, "color") << ";\n";
        }
    }

    // Declare shadow parameters
    if (props.shadowCounts != 0)
    {
        source << "varying vec3 position_obj;\n";
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            for (unsigned int j = 0; j < props.getEclipseShadowCountForLight(i); j++)
            {
                source << "uniform vec4 " << IndexedParameter("shadowTexGenS", i, j) << ";\n";
                source << "uniform vec4 " << IndexedParameter("shadowTexGenT", i, j) << ";\n";
                source << "uniform float " << IndexedParameter("shadowFalloff", i, j) << ";\n";
                source << "uniform float " << IndexedParameter("shadowMaxDepth", i, j) << ";\n";
            }
        }
    }

    source << DeclareVarying("v_Color", Shader_Vector4);

    // Begin main()
    source << "\nvoid main(void)\n";
    source << "{\n";

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
            glBindAttribLocation(prog->getID(),
                                 CelestiaGLProgram::VertexCoordAttributeIndex,
                                 "in_Position");

            glBindAttribLocation(prog->getID(),
                                 CelestiaGLProgram::NormalAttributeIndex,
                                 "in_Normal");

            glBindAttribLocation(prog->getID(),
                                 CelestiaGLProgram::TextureCoord0AttributeIndex,
                                 "in_TexCoord0");

            glBindAttribLocation(prog->getID(),
                                 CelestiaGLProgram::TextureCoord1AttributeIndex,
                                 "in_TexCoord1");

            glBindAttribLocation(prog->getID(),
                                 CelestiaGLProgram::TextureCoord2AttributeIndex,
                                 "in_TexCoord2");

            glBindAttribLocation(prog->getID(),
                                 CelestiaGLProgram::TextureCoord3AttributeIndex,
                                 "in_TexCoord3");

            glBindAttribLocation(prog->getID(),
                                 CelestiaGLProgram::ColorAttributeIndex,
                                 "in_Color");

            if (props.texUsage & ShaderProperties::NormalTexture)
            {
                glBindAttribLocation(prog->getID(),
                                     CelestiaGLProgram::TangentAttributeIndex,
                                     "in_Tangent");
            }

            if (props.texUsage & ShaderProperties::PointSprite)
            {
                glBindAttribLocation(prog->getID(),
                                     CelestiaGLProgram::PointSizeAttributeIndex,
                                     "in_PointSize");
            }

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
        status = GLShaderLoader::CreateProgram(errorVertexShaderSource,
                                               errorFragmentShaderSource,
                                               &prog);
        if (status != ShaderStatus_OK)
        {
            if (g_shaderLogFile != nullptr)
                *g_shaderLogFile << "Failed to create error shader!\n";
        }
        else
        {
            status = prog->link();
        }
    }

    if (prog == nullptr)
        return nullptr;

    return new CelestiaGLProgram(*prog, props);
}

CelestiaGLProgram*
ShaderManager::buildProgram(const std::string& vs, const std::string& fs)
{
    GLProgram* prog = nullptr;
    GLShaderStatus status;

    string _vs = fmt::sprintf("%s%s%s\n", CommonHeader, VertexHeader, vs);
    string _fs = fmt::sprintf("%s%s%s\n", CommonHeader, FragmentHeader, fs);

    DumpVSSource(_vs);
    DumpFSSource(_fs);

    status = GLShaderLoader::CreateProgram(_vs, _fs, &prog);
    if (status == ShaderStatus_OK)
    {
        glBindAttribLocation(prog->getID(),
                             CelestiaGLProgram::VertexCoordAttributeIndex,
                             "in_Position");

        glBindAttribLocation(prog->getID(),
                             CelestiaGLProgram::NormalAttributeIndex,
                             "in_Normal");

        glBindAttribLocation(prog->getID(),
                             CelestiaGLProgram::TextureCoord0AttributeIndex,
                             "in_TexCoord0");

        glBindAttribLocation(prog->getID(),
                             CelestiaGLProgram::TextureCoord1AttributeIndex,
                             "in_TexCoord1");

        glBindAttribLocation(prog->getID(),
                             CelestiaGLProgram::TextureCoord2AttributeIndex,
                             "in_TexCoord2");

        glBindAttribLocation(prog->getID(),
                             CelestiaGLProgram::TextureCoord3AttributeIndex,
                             "in_TexCoord3");

        glBindAttribLocation(prog->getID(),
                             CelestiaGLProgram::ColorAttributeIndex,
                             "in_Color");

        glBindAttribLocation(prog->getID(),
                             CelestiaGLProgram::TangentAttributeIndex,
                             "in_Tangent");

        glBindAttribLocation(prog->getID(),
                             CelestiaGLProgram::PointSizeAttributeIndex,
                             "in_PointSize");

        status = prog->link();
    }

    if (status != ShaderStatus_OK)
    {
        // If the shader creation failed for some reason, substitute the
        // error shader.
        status = GLShaderLoader::CreateProgram(errorVertexShaderSource,
                                               errorFragmentShaderSource,
                                               &prog);
        if (status != ShaderStatus_OK)
        {
            if (g_shaderLogFile != nullptr)
                *g_shaderLogFile << "Failed to create error shader!\n";
        }
        else
        {
            status = prog->link();
        }
    }

    if (prog == nullptr)
        return nullptr;

    return new CelestiaGLProgram(*prog);
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
CelestiaGLProgram::floatParam(const string& paramName)
{
    return FloatShaderParameter(program->getID(), paramName.c_str());
}


IntegerShaderParameter
CelestiaGLProgram::intParam(const string& paramName)
{
    return IntegerShaderParameter(program->getID(), paramName.c_str());
}


IntegerShaderParameter
CelestiaGLProgram::samplerParam(const string& paramName)
{
    return IntegerShaderParameter(program->getID(), paramName.c_str());
}


Vec3ShaderParameter
CelestiaGLProgram::vec3Param(const string& paramName)
{
    return Vec3ShaderParameter(program->getID(), paramName.c_str());
}


Vec4ShaderParameter
CelestiaGLProgram::vec4Param(const string& paramName)
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
#ifdef USE_HDR
    nightLightScale          = floatParam("nightLightScale");
#endif

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

    if (props.lightModel == ShaderProperties::LunarLambertModel)
    {
        lunarLambert         = floatParam("lunarLambert");
    }

    if ((props.texUsage & ShaderProperties::PointSprite) != 0)
    {
        pointScale           = floatParam("pointScale");
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
                                      Color materialEmissive
#ifdef USE_HDR
                                     ,float _nightLightScale
#endif
                                      )
{
    unsigned int nLights = min(MaxShaderLights, ls.nLights);

    Vector3f diffuseColor(materialDiffuse.red(),
                          materialDiffuse.green(),
                          materialDiffuse.blue());
    Vector3f specularColor(materialSpecular.red(),
                           materialSpecular.green(),
                           materialSpecular.blue());

    for (unsigned int i = 0; i < nLights; i++)
    {
        const DirectionalLight& light = ls.lights[i];

        Vector3f lightColor = Vector3f(light.color.red(),
                                       light.color.green(),
                                       light.color.blue()) * light.irradiance;
        lights[i].direction = light.direction_obj;

        // Include a phase-based normalization factor to prevent planets from appearing
        // too dim when rendered with non-Lambertian photometric functions.
        float cosPhaseAngle = light.direction_obj.dot(ls.eyeDir_obj);
        if (props.lightModel == ShaderProperties::LunarLambertModel)
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

        Vector3f halfAngle_obj = ls.eyeDir_obj + light.direction_obj;
        if (halfAngle_obj.norm() != 0.0f)
            halfAngle_obj.normalize();
        lights[i].halfVector = halfAngle_obj;
    }

    eyePosition = ls.eyePos_obj;
    ambientColor = ls.ambientColor.cwiseProduct(diffuseColor) +
        Vector3f(materialEmissive.red(), materialEmissive.green(), materialEmissive.blue());
    opacity = materialDiffuse.alpha();
#ifdef USE_HDR
    nightLightScale = _nightLightScale;
#endif
}


/** Set GLSL shader constants for shadows from ellipsoid occluders; shadows from
 *  irregular objects are not handled yet.
 *  \param scaleFactors the scale factors of the object being shadowed
 *  \param orientation orientation of the object being shadowed
 */
void
CelestiaGLProgram::setEclipseShadowParameters(const LightingState& ls,
                                              const Vector3f& scaleFactors,
                                              const Eigen::Quaternionf& orientation)
{
    // Compute the transformation from model to world coordinates
    Affine3f rotation(orientation.conjugate());
    Matrix4f modelToWorld = (rotation *  Scaling(scaleFactors)).matrix();

    for (unsigned int li = 0;
         li < min(ls.nLights, MaxShaderLights);
         li++)
    {
        if (ls.shadows[li] != nullptr)
        {
            unsigned int nShadows = min((size_t) MaxShaderEclipseShadows, ls.shadows[li]->size());

            // The shadow bias matrix maps from
            Matrix4f shadowBias;
            shadowBias << 0.5f, 0.0f, 0.0f, 0.5f,
                          0.0f, 0.5f, 0.0f, 0.5f,
                          0.0f, 0.0f, 0.5f, 0.5f,
                          0.0f, 0.0f, 0.0f, 1.0f;

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
                Vector3f u = shadow.direction.unitOrthogonal();
                Vector3f v = u.cross(shadow.direction);
                Matrix4f shadowRotation;
                shadowRotation << u.transpose(),                0.0f,
                                  v.transpose(),                0.0f,
                                  shadow.direction.transpose(), 0.0f,
                                  0.0f, 0.0f, 0.0f,             1.0f;

                // Compose the world-to-shadow matrix
                Matrix4f worldToShadow = shadowRotation *
                                         Affine3f(Scaling(1.0f / shadow.penumbraRadius)).matrix() *
                                         Affine3f(Translation3f(-shadow.origin)).matrix();

                // Finally, multiply all the matrices together to get the mapping from
                // object space to shadow map space.
                Matrix4f m = shadowBias * worldToShadow * modelToWorld;

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
    // fallse off exponentially with height above the planet's surface, so the actual
    // radius is infinite. That's a bit impractical, so well just render the portion
    // out to the point where the density is some fraction of the surface density.
    float skySphereRadius = atmPlanetRadius + -atmosphere.mieScaleHeight * log(AtmosphereExtinctionThreshold);

    float tMieCoeff           = atmosphere.mieCoeff * objRadius;
    Vector3f tRayleighCoeff   = atmosphere.rayleighCoeff * objRadius;
    Vector3f tAbsorptionCoeff = atmosphere.absorptionCoeff * objRadius;

    float r = skySphereRadius / objRadius;
    atmosphereRadius = Vector3f(r, r * r, atmPlanetRadius / objRadius);

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
    Vector3f tScatterCoeffSum = tRayleighCoeff.array() + tMieCoeff;
    scatterCoeffSum = tScatterCoeffSum;
    invScatterCoeffSum = tScatterCoeffSum.cwiseInverse();
    extinctionCoeff = tScatterCoeffSum + tAbsorptionCoeff;
}

void
CelestiaGLProgram::setMVPMatrices(const Matrix4f& p, const Matrix4f& m)
{
    ProjectionMatrix = p;
    ModelViewMatrix = m;
    MVPMatrix = p * m;
}

