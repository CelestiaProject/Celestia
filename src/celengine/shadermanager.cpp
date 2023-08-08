// shadermanager.cpp
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "shadermanager.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <optional>
#include <ostream>
#include <sstream>
#include <string_view>
#include <system_error>
#include <tuple>
#include <utility>

#include <fmt/format.h>

#include <celcompat/filesystem.h>
#include <celutil/flag.h>
#include <celutil/logger.h>
#include "atmosphere.h"
#include "glsupport.h"
#include "lightenv.h"


using celestia::util::GetLogger;
using namespace std::string_view_literals;
namespace gl = celestia::gl;
namespace util = celestia::util;

#define POINT_FADE 1

namespace
{
#if GL_ONLY_SHADOWS
constexpr const int ShadowSampleKernelWidth = 2;
#endif

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

enum ShaderInOut
{
    Shader_In,
    Shader_Out
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

#ifdef GL_ES
constexpr std::string_view VersionHeader = "#version 100\n"sv;
constexpr std::string_view VersionHeaderGL3 = "#version 320 es\n"sv;
constexpr std::string_view CommonHeader = "precision highp float;\n"sv;
#else
constexpr std::string_view VersionHeader = "#version 120\n"sv;
constexpr std::string_view VersionHeaderGL3 = "#version 150\n"sv;
constexpr std::string_view CommonHeader = "\n"sv;
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
    thisPos.xy /= thisPos.w;
    nextPos.xy /= nextPos.w;
    vec2 transform = normalize(nextPos.xy - thisPos.xy);
    transform = vec2(transform.y * lineWidthX, -transform.x * lineWidthY) * in_ScaleFactor;
    gl_Position = vec4((thisPos.xy + transform) * thisPos.w, thisPos.zw);
)glsl"sv;

std::string_view
VertexPosition(const ShaderProperties& props)
{
    return util::is_set(props.texUsage, TexUsage::LineAsTriangles) ? LineVertexPosition : NormalVertexPosition;
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

constexpr std::string_view TextureTransformUniforms = R"glsl(
uniform vec2 texCoordBase0;
uniform vec2 texCoordBase1;
uniform vec2 texCoordBase2;
uniform vec2 texCoordBase3;
uniform vec2 texCoordDelta0;
uniform vec2 texCoordDelta1;
uniform vec2 texCoordDelta2;
uniform vec2 texCoordDelta3;
)glsl"sv;

std::string
LightProperty(unsigned int i, std::string_view property)
{
#ifdef USE_GLSL_STRUCTS
    return fmt::format("lights[{}].{}", i, property);
#else
    return fmt::format("light{}_{}", i, property);
#endif
}

std::string
ApplyShadow(bool hasSpecular)
{
    constexpr std::string_view code = R"glsl(
if (cosNormalLightDir > 0.0)
{{
    shadowMapCoeff = calculateShadow();
    diff.rgb *= shadowMapCoeff;
    {}
}}
)glsl"sv;

    return fmt::format(code, hasSpecular ? "spec.rgb *= shadowMapCoeff;"sv : ""sv);
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

std::string
DeclareUniform(std::string_view name, ShaderVariableType type)
{
    return fmt::format("uniform {} {};\n", ShaderTypeString(type), name);
}

std::string
DeclareInput(std::string_view name, ShaderVariableType type)
{
    return fmt::format("varying {} {};\n", ShaderTypeString(type), name);
}

std::string
DeclareOutput(std::string_view name, ShaderVariableType type)
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
DeclareLocal(std::string_view name, ShaderVariableType type, std::string_view value)
{
    return fmt::format("{} {} = {};\n", ShaderTypeString(type), name, value);
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

inline void DumpVSSource(const std::ostringstream& source)
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

inline void DumpFSSource(const std::ostringstream& source)
{
    DumpFSSource(source.str());
}

std::string
DeclareLights(const ShaderProperties& props)
{
    if (props.nLights == 0)
        return {};

    std::ostringstream stream;
#ifdef USE_GLSL_STRUCTS
    stream << "uniform struct {\n";
    stream << "   vec3 direction;\n";
    stream << "   vec3 diffuse;\n";
    stream << "   vec3 specular;\n";
    stream << "   vec3 halfVector;\n";
    if (props.lightModel == LightingModel::AtmosphereModel || props.hasScattering())
        stream << "   vec3 color;\n";
    if (util::is_set(props.texUsage, TexUsage::NightTexture))
        stream << "   float brightness;\n";
    stream << "} lights[" << props.nLights << "];\n";
#else
    for (unsigned int i = 0; i < props.nLights; i++)
    {
        stream << DeclareUniform(fmt::format("light{}_direction", i), Shader_Vector3);
        stream << DeclareUniform(fmt::format("light{}_diffuse", i), Shader_Vector3);
        if (props.hasSpecular())
        {
            stream << DeclareUniform(fmt::format("light{}_specular", i), Shader_Vector3);
            stream << DeclareUniform(fmt::format("light{}_halfVector", i), Shader_Vector3);
        }
        if (props.lightModel == LightingModel::AtmosphereModel || props.hasScattering())
            stream << DeclareUniform(fmt::format("light{}_color", i), Shader_Vector3);
        if (util::is_set(props.texUsage, TexUsage::NightTexture))
            stream << DeclareUniform(fmt::format("light{}_brightness", i), Shader_Float);
    }
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
TexCoord2D(unsigned int i, bool transform)
{
    return transform ? fmt::format("(texCoordBase{} + texCoordDelta{} * in_TexCoord{}.st)", i, i, i) : fmt::format("in_TexCoord{}.st", i);
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
    return fmt::format("{0}.x = dot(T, {1});\n"
                       "{0}.y = dot(-bitangent, {1});\n"
                       "{0}.z = dot(N, {1});\n",
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


std::string
AssignDiffuse(unsigned int lightIndex, const ShaderProperties& props)
{
    if (!props.usesShadows() && !props.hasSpecular())
        return fmt::format("diff.rgb += {} * ", LightProperty(lightIndex, "diffuse"));
    else
        return fmt::format("{} = ", SeparateDiffuse(lightIndex));
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

    if (props.lightModel == LightingModel::ParticleDiffuseModel)
    {
        // The ParticleDiffuse model doesn't use a surface normal; vertices
        // are lit as if they were infinitesimal spherical particles,
        // unaffected by light direction except when considering shadows.
        source += "NL = 1.0;\n";
    }
    else
    {
        source += "NL = max(0.0, dot(N, " + LightProperty(i, "direction") + "));\n";
    }

    if (props.usesTangentSpaceLighting())
    {
        source += TangentSpaceTransform(LightDir_tan(i), LightProperty(i, "direction"));
    }
    else if (props.hasSpecular())
    {
        if (util::is_set(props.lightModel, LightingModel::LunarLambertModel))
            source += AssignDiffuse(i, props) + " mix(NL, NL / (max(NV, 0.001) + NL), lunarLambert);\n";
        else
            source += SeparateDiffuse(i) + " = NL;\n";
    }
#if 0
    else if (props.lightModel == LightingModel::OrenNayarModel)
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
    else if (util::is_set(props.lightModel, LightingModel::LunarLambertModel))
    {
        source += AssignDiffuse(i, props) + " mix(NL, NL / (max(NV, 0.001) + NL), lunarLambert);\n";
    }
    else if (props.usesShadows())
    {
        // When there are shadows, we need to track the diffuse contributions
        // separately for each light.
        source += SeparateDiffuse(i) + " = NL;\n";
        if (props.hasSpecular())
            source += SeparateSpecular(i) + " = pow(NH, shininess);\n";
    }
    else
    {
        source += "diff.rgb += " + LightProperty(i, "diffuse") + " * NL;\n";
        if (props.hasSpecular())
            source += "spec.rgb += " + LightProperty(i, "specular") + " * (pow(NH, shininess) * NL);\n";
    }

    if (util::is_set(props.texUsage, TexUsage::NightTexture) && !props.usesTangentSpaceLighting())
        source += "totalLight += NL * " + LightProperty(i, "brightness") + ";\n";

    return source;
}


std::string
BeginLightSourceShadows(const ShaderProperties& props, unsigned int light)
{
    std::string source;

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
        if (light == 0)
            source += DeclareLocal("ringShadowTexCoordX", Shader_Float);
        source += "ringShadowTexCoordX = " +  RingShadowTexCoord(light) + ";\n";

#ifdef GL_ES
        if (!gl::OES_texture_border_clamp)
            source += "if (ringShadowTexCoordX >= 0.0 && ringShadowTexCoordX <= 1.0)\n{\n";
#endif
        if (gl::ARB_shader_texture_lod)
        {
            source += "shadow *= 1.0 - texture2DLod(ringTex, vec2(ringShadowTexCoordX, 0.0), " + IndexedParameter("ringShadowLOD", light) + ").a;\n";
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
            source += "shadow *= 1.0 - texture2D(ringTex, vec2(ringShadowTexCoordX, 0.0), " + IndexedParameter("ringShadowLOD", light) + ").a;\n";
        }
#ifdef GL_ES
        if (!gl::OES_texture_border_clamp)
            source += "}\n";
#endif
    }

    if (props.hasCloudShadowForLight(light))
    {
        source += "shadow *= 1.0 - texture2D(cloudShadowTex, " + CloudShadowTexCoord(light) + ").a * 0.75;\n";
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

    source += "shadowCenter.s = dot(vec4(position, 1.0), " +
        IndexedParameter("shadowTexGenS", light, shadow) + ") - 0.5;\n";
    source += "shadowCenter.t = dot(vec4(position, 1.0), " +
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
    source += "    vec3 atmLeave = nposition;\n";

    source += "    vec3 atmSamplePoint = (atmEnter + atmLeave) * 0.5;\n";
    //source += "    vec3 atmSamplePoint = atmEnter * 0.2 + atmLeave * 0.8;\n";

    // Compute the distance through the atmosphere from the sample point to the sun
    source += "    vec3 atmSamplePointSun = mix(atmEnter, atmLeave, 0.5);\n";
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
    source += "    atmSamplePoint = mix(atmEnter, atmLeave, 0.667);\n";
    //source += "    atmSamplePoint = atmEnter * 0.1 + atmLeave * 0.9;\n";
    source += "    float h = max(0.0, length(atmSamplePoint) - atmosphereRadius.z);\n";
    source += "    density += exp(-h * mieH);\n";
    source += "    atmSamplePoint = mix(atmEnter, atmLeave, 0.333);\n";
    //source += "    atmSamplePoint = atmEnter * 0.9 + atmLeave * 0.1;\n";
    source += "    h = max(0.0, length(atmSamplePoint) - atmosphereRadius.z);\n";
    source += "    density += exp(-h * mieH);\n";
#endif

    bool hasAbsorption = true;

    std::string scatter;
    if (hasAbsorption)
    {
        source += "    vec3 sunColor = " + LightProperty(0, "color") + " * exp(-extinctionCoeff * density * distSun);\n";
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
    if (props.lightModel == LightingModel::AtmosphereModel)
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
    source += "    vec3 atmLeave = nposition;\n";

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
    if (props.lightModel == LightingModel::AtmosphereModel)
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
    if (util::is_set(props.texUsage, TexUsage::DiffuseTexture))
    {
        source += DeclareUniform("diffTex", Shader_Sampler2D);
    }

    if (util::is_set(props.texUsage, TexUsage::NormalTexture))
    {
        source += DeclareUniform("normTex", Shader_Sampler2D);
    }

    if (util::is_set(props.texUsage, TexUsage::SpecularTexture))
    {
        source += DeclareUniform("specTex", Shader_Sampler2D);
    }

    if (util::is_set(props.texUsage, TexUsage::NightTexture))
    {
        source += DeclareUniform("nightTex", Shader_Sampler2D);
    }

    if (util::is_set(props.texUsage, TexUsage::EmissiveTexture))
    {
        source += DeclareUniform("emissiveTex", Shader_Sampler2D);
    }

    if (util::is_set(props.texUsage, TexUsage::OverlayTexture))
    {
        source += DeclareUniform("overlayTex", Shader_Sampler2D);
    }

    if (util::is_set(props.texUsage, TexUsage::ShadowMapTexture))
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
TextureCoordDeclarations(const ShaderProperties& props, ShaderInOut inOut)
{
    std::string source;

    auto declare = inOut == Shader_In ? DeclareInput : DeclareOutput;

    if (props.hasSharedTextureCoords())
    {
        // If the shared texture coords flag is set, use the diffuse texture
        // coordinate for sampling all the texture maps.
        if (util::is_set(props.texUsage, TexUsage::DiffuseTexture  |
                                         TexUsage::NormalTexture   |
                                         TexUsage::SpecularTexture |
                                         TexUsage::NightTexture    |
                                         TexUsage::EmissiveTexture |
                                         TexUsage::OverlayTexture))
        {
            source += declare("diffTexCoord", Shader_Vector2);
        }
    }
    else
    {
        if (util::is_set(props.texUsage, TexUsage::DiffuseTexture))
            source += declare("diffTexCoord", Shader_Vector2);
        if (util::is_set(props.texUsage, TexUsage::NormalTexture))
            source += declare("normTexCoord", Shader_Vector2);
        if (util::is_set(props.texUsage, TexUsage::SpecularTexture))
            source += declare("specTexCoord", Shader_Vector2);
        if (util::is_set(props.texUsage, TexUsage::NightTexture))
            source += declare("nightTexCoord", Shader_Vector2);
        if (util::is_set(props.texUsage, TexUsage::EmissiveTexture))
            source += declare("emissiveTexCoord", Shader_Vector2);
        if (util::is_set(props.texUsage, TexUsage::OverlayTexture))
            source += declare("overlayTexCoord", Shader_Vector2);
    }

    if (util::is_set(props.texUsage, TexUsage::ShadowMapTexture))
    {
        source += declare("shadowTexCoord0", Shader_Vector4);
        source += declare("cosNormalLightDir", Shader_Float);
    }

    return source;
}

std::string
PointSizeDeclaration()
{
    std::string source;
    source += DeclareOutput("pointFade", Shader_Float);
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
    if (status == GLShaderStatus::OK)
    {
        BindAttribLocations(*prog);
        status = (*prog)->link();
    }

    if (status != GLShaderStatus::OK)
    {
        if (g_shaderLogFile != nullptr)
            *g_shaderLogFile << "Failed to create error shader!\n";

        return GLShaderStatus::LinkError;
    }

    return GLShaderStatus::OK;
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
    return util::is_set(texUsage, TexUsage::ShadowMapTexture);
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
    return util::is_set(texUsage, TexUsage::SharedTextureCoords);
}

bool
ShaderProperties::hasTextureCoordTransform() const
{
    return util::is_set(texUsage, TexUsage::TextureCoordTransform);
}

bool
ShaderProperties::hasSpecular() const
{
    return util::is_set(lightModel, LightingModel::PerPixelSpecularModel);
}


bool
ShaderProperties::hasScattering() const
{
    return util::is_set(texUsage, TexUsage::Scattering);
}


bool
ShaderProperties::isViewDependent() const
{
    switch (lightModel)
    {
    case LightingModel::DiffuseModel:
    case LightingModel::ParticleDiffuseModel:
    case LightingModel::EmissiveModel:
    case LightingModel::UnlitModel:
        return false;
    default:
        return true;
    }
}


bool
ShaderProperties::usesTangentSpaceLighting() const
{
    return util::is_set(texUsage, TexUsage::NormalTexture);
}


bool ShaderProperties::usePointSize() const
{
    return util::is_set(texUsage, TexUsage::PointSprite | TexUsage::StaticPointSize);
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
    source += "// buildVertexShader\n";
    source += "/***************************************************\n";
    source += fmt::format(
R"glsl(
    usesShadows = {}
    usesTangentSpaceLighting = {}
    hasEclipseShadows = {}
    hasRingShadows = {}
    hasSelfShadows = {}
    hasCloudShadows = {}
    hasSpecular = {}
    hasScattering = {}
    isViewDependent = {}
    lightModel = {:x}
)glsl",
    props.usesShadows(),
    props.usesTangentSpaceLighting(),
    props.hasEclipseShadows(),
    props.hasRingShadows(),
    props.hasSelfShadows(),
    props.hasCloudShadows(),
    props.hasSpecular(),
    props.hasScattering(),
    props.isViewDependent(),
    static_cast<std::uint16_t>(props.lightModel));
    source += "***************************************************/\n";
    source += CommonHeader;
    source += VertexHeader;
    source += CommonAttribs;
    if (props.hasTextureCoordTransform())
        source += TextureTransformUniforms;

    source += DeclareLights(props);
    source += TextureCoordDeclarations(props, Shader_Out);
    source += DeclareUniform("textureOffset", Shader_Float);

    if (props.usePointSize())
        source += PointSizeDeclaration();

    if (props.lightModel != LightingModel::ParticleDiffuseModel)
        source += DeclareOutput("normal", Shader_Vector3);

    if (props.usesTangentSpaceLighting())
    {
        source += DeclareAttribute("in_Tangent", Shader_Vector3);
        source += DeclareOutput("tangent", Shader_Vector3);
    }

    if (props.lightModel == LightingModel::UnlitModel && util::is_set(props.texUsage, TexUsage::VertexColors))
        source += DeclareOutput("diff", Shader_Vector4);

    if (props.isViewDependent() || props.hasScattering() || props.hasEclipseShadows())
        source += DeclareOutput("position", Shader_Vector3);

    // Shadow parameters
    if (props.hasRingShadows())
    {
        source += DeclareUniform("ringWidth", Shader_Float);
        source += DeclareUniform("ringRadius", Shader_Float);
        source += DeclareUniform("ringPlane", Shader_Vector4);
        source += DeclareUniform("ringCenter", Shader_Vector3);
        source += DeclareOutput("ringShadowTexCoord", Shader_Vector4);
    }

    if (props.hasCloudShadows())
    {
        source += DeclareUniform("cloudShadowTexOffset", Shader_Float);
        source += DeclareUniform("cloudHeight", Shader_Float);
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            if (props.hasCloudShadowForLight(i))
                source += DeclareOutput(CloudShadowTexCoord(i), Shader_Vector2);
        }
    }

    if (props.hasShadowMap())
        source += DeclareUniform("ShadowMatrix0", Shader_Matrix4);

    if (util::is_set(props.texUsage, TexUsage::LineAsTriangles))
        source += LineDeclaration();

    source += VPFunction(props.fishEyeOverride != FisheyeOverrideMode::Disabled && fisheyeEnabled);

    // Begin main() function
    source += "\nvoid main(void)\n{\n";
    if (props.lightModel != LightingModel::ParticleDiffuseModel)
        source += "normal = in_Normal;\n";

    if (props.isViewDependent() || props.hasScattering() || props.hasEclipseShadows())
        source += "position = in_Position.xyz;\n";

    if (props.usesTangentSpaceLighting())
        source += "tangent = in_Tangent;\n";

    if (props.lightModel == LightingModel::UnlitModel && util::is_set(props.texUsage, TexUsage::VertexColors))
        source += "diff = in_Color;\n";

    if (props.hasShadowMap())
    {
        source += "cosNormalLightDir = dot(in_Normal, " + LightProperty(0, "direction") + ");\n";
        source += "shadowTexCoord0 = ShadowMatrix0 * vec4(in_Position.xyz, 1.0);\n";
    }

    unsigned int nTexCoords = 0;

    // Output the texture coordinates. Use just a single texture coordinate if all textures are mapped
    // identically. The texture offset is added for cloud maps; specular and night texture are not offset
    // because cloud layers never have these textures.
    bool hasTexCoordTransform = props.hasTextureCoordTransform();
    if (props.hasSharedTextureCoords())
    {
        if (util::is_set(props.texUsage, TexUsage::DiffuseTexture  |
                                         TexUsage::NormalTexture   |
                                         TexUsage::SpecularTexture |
                                         TexUsage::NightTexture    |
                                         TexUsage::EmissiveTexture |
                                         TexUsage::OverlayTexture))
        {
            source += "diffTexCoord = " + TexCoord2D(nTexCoords, hasTexCoordTransform) + ";\n";
            source += "diffTexCoord.x += textureOffset;\n";
        }
    }
    else
    {
        if (util::is_set(props.texUsage, TexUsage::DiffuseTexture))
        {
            source += "diffTexCoord = " + TexCoord2D(nTexCoords, hasTexCoordTransform) + " + vec2(textureOffset, 0.0);\n";
            nTexCoords++;
        }

        if (util::is_set(props.texUsage, TexUsage::NormalTexture))
        {
            source += "normTexCoord = " + TexCoord2D(nTexCoords, hasTexCoordTransform) + " + vec2(textureOffset, 0.0);\n";
            nTexCoords++;
        }

        if (util::is_set(props.texUsage, TexUsage::SpecularTexture))
        {
            source += "specTexCoord = " + TexCoord2D(nTexCoords, hasTexCoordTransform) + ";\n";
            nTexCoords++;
        }

        if (util::is_set(props.texUsage, TexUsage::NightTexture))
        {
            source += "nightTexCoord = " + TexCoord2D(nTexCoords, hasTexCoordTransform) + ";\n";
            nTexCoords++;
        }

        if (util::is_set(props.texUsage, TexUsage::EmissiveTexture))
        {
            source += "emissiveTexCoord = " + TexCoord2D(nTexCoords, hasTexCoordTransform) + ";\n";
            nTexCoords++;
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

    if (util::is_set(props.texUsage, TexUsage::OverlayTexture) && !props.hasSharedTextureCoords())
    {
        source += "overlayTexCoord = " + TexCoord2D(nTexCoords, hasTexCoordTransform) + ";\n";
        nTexCoords++;
    }

    if (util::is_set(props.texUsage, TexUsage::PointSprite))
        source += PointSizeCalculation();
    else if (util::is_set(props.texUsage, TexUsage::StaticPointSize))
        source += StaticPointSize();

    source += VertexPosition(props);
    source += "}\n";

    DumpVSSource(source);

    GLVertexShader* vs = nullptr;
    GLShaderStatus status = GLShaderLoader::CreateVertexShader(source, &vs);
    return status == GLShaderStatus::OK ? vs : nullptr;
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
    source += TextureCoordDeclarations(props, Shader_In);

    if (props.hasScattering())
        source += ScatteringConstantDeclarations(props);

    source += DeclareUniform("eyePosition", Shader_Vector3);

    if (util::is_set(props.lightModel, LightingModel::LunarLambertModel))
        source += DeclareUniform("lunarLambert", Shader_Float);

    if (props.lightModel != LightingModel::ParticleDiffuseModel)
        source += DeclareInput("normal", Shader_Vector3);

    if (props.isViewDependent() || props.hasScattering() || props.hasEclipseShadows())
        source += DeclareInput("position", Shader_Vector3);

    if (props.lightModel != LightingModel::UnlitModel)
    {
        source += DeclareUniform("ambientColor", Shader_Vector3);
        source += DeclareUniform("opacity", Shader_Float);
    }
    else
    {
        if (util::is_set(props.texUsage, TexUsage::VertexColors))
            source += DeclareInput("diff", Shader_Vector4);
    }

    // Declare lighting parameters
    if (props.usesTangentSpaceLighting())
        source += DeclareInput("tangent", Shader_Vector3);

    if (props.hasSpecular())
        source += DeclareUniform("shininess", Shader_Float);

    // Declare shadow parameters
    if (props.hasEclipseShadows())
    {
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
        source += DeclareInput("ringShadowTexCoord", Shader_Vector4);
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            if (props.hasRingShadowForLight(i))
                source += DeclareUniform(IndexedParameter("ringShadowLOD", i), Shader_Float);
        }
    }

    if (props.hasCloudShadows())
    {
        source += DeclareUniform("cloudShadowTex", Shader_Sampler2D);
        for (unsigned int i = 0; i < props.nLights; i++)
            source += DeclareInput(CloudShadowTexCoord(i), Shader_Vector2);
    }

    if (props.usePointSize())
        source += DeclareInput("pointFade", Shader_Float);

    if (props.hasShadowMap())
    {
        source += DeclareUniform("shadowMapSize", Shader_Float);
        source += CalculateShadow();
    }

    source += DeclareLights(props);

    source += "\nvoid main(void)\n{\n";
    source += "vec4 color;\n";

    if (props.lightModel != LightingModel::ParticleDiffuseModel)
        source += "vec3 N = normalize(normal);\n";

    if (props.isViewDependent() || props.hasScattering() || props.hasEclipseShadows())
        source += "vec3 nposition = normalize(position);\n";

    if (props.lightModel != LightingModel::UnlitModel)
        source += "vec4 diff = vec4(ambientColor, opacity);\n";
    else if (!util::is_set(props.texUsage, TexUsage::VertexColors))
        source += "vec4 diff = vec4(1.0);\n";

    if (props.usesTangentSpaceLighting())
    {
        source += "vec3 T = normalize(tangent);\n";
        if (props.isViewDependent())
            source += DeclareLocal("eyeDir_tan", Shader_Vector3); // tangent space eye vector
        for (unsigned int i = 0; i < props.nLights; i++)
            source += DeclareLocal(LightDir_tan(i), Shader_Vector3);
    }

    if (props.hasSpecular())
        source += "vec4 spec = vec4(0.0);\n";

    bool useSeparateDiffuse = !props.usesTangentSpaceLighting() &&
                              (props.hasSpecular() ||
                               props.usesShadows() ||
                               props.isViewDependent() ||
                               props.hasScattering());

    if (useSeparateDiffuse)
        source += DeclareLocal("diffFactors", Shader_Vector4);

    if (util::is_set(props.texUsage, TexUsage::NightTexture))
        source += "float totalLight = 0.0;\n";

    if (props.isViewDependent() || props.hasScattering())
            source += "vec3 eyeDir = normalize(eyePosition - nposition);\n";

    if (props.usesTangentSpaceLighting())
    {
        source += "vec3 bitangent = cross(N, T);\n";
        if (props.isViewDependent())
            source += TangentSpaceTransform("eyeDir_tan", "eyeDir");
    }
    else if (props.isViewDependent() || props.hasScattering())
    {
        source += "float NV = dot(N, eyeDir);\n";
    }

    if (props.lightModel != LightingModel::UnlitModel)
    {
        source += DeclareLocal("NL", Shader_Float);
        for (unsigned int i = 0; i < props.nLights; i++)
            source += AddDirectionalLightContrib(i, props);
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
        if (util::is_set(props.texUsage, TexUsage::NormalTexture))
        {
            if (util::is_set(props.texUsage, TexUsage::CompressedNormalTexture))
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

            if (props.hasSpecular())
            {
                source += "vec3 H;\n";
                source += "float NH;\n";
            }
            if (util::is_set(props.lightModel, LightingModel::LunarLambertModel))
                source += "float NV = dot(n, V);\n";
        }

        for (unsigned i = 0; i < props.nLights; i++)
        {
            // Bump mapping with self shadowing
            source += "NL = dot(" + LightDir_tan(i) + ", n);\n";
            if (util::is_set(props.lightModel, LightingModel::LunarLambertModel))
            {
                source += "NL = max(0.0, NL);\n";
                source += "l = mix(NL, (NL / (max(NV, 0.001) + NL)), lunarLambert) * clamp(" + LightDir_tan(i) + ".z * 8.0, 0.0, 1.0);\n";
            }
            else
            {
                source += "l = max(0.0, dot(" + LightDir_tan(i) + ", n)) * clamp(" + LightDir_tan(i) + ".z * 8.0, 0.0, 1.0);\n";
            }

            if (util::is_set(props.texUsage, TexUsage::NightTexture))
                source += "totalLight += l * " + LightProperty(i, "brightness") + ";\n";

            if (props.hasShadowsForLight(i))
                source += ShadowsForLightSource(props, i);

            std::string illum(props.hasShadowsForLight(i) ? "l * shadow" : "l");
            source += "diff.rgb += " + illum + " * " + LightProperty(i, "diffuse") + ";\n";

            if (props.hasSpecular())
            {
                source += "H = normalize(eyeDir_tan + " + LightDir_tan(i) + ");\n";
                source += "NH = max(0.0, dot(n, H));\n";
                source += "spec.rgb += " + illum + " * pow(NH, shininess) * " + LightProperty(i, "specular") + ";\n";
            }
        }
    }
    else if (props.hasSpecular())
    {
        source += "float NH;\n";
        source += "float shadowMapCoeff = 1.0;\n";

        // Sum the contributions from each light source
        for (unsigned i = 0; i < props.nLights; i++)
        {
            if (props.hasShadowsForLight(i))
                source += ShadowsForLightSource(props, i);

            std::string illum(props.hasShadowsForLight(i) ? "shadow" : SeparateDiffuse(i));
            source += "diff.rgb += " + illum + " * " + LightProperty(i, "diffuse") + ";\n";
            source += "NH = max(0.0, dot(N, normalize(" + LightProperty(i, "halfVector") + ")));\n";
            source += "spec.rgb += " + illum + " * pow(NH, shininess) * " + LightProperty(i, "specular") + ";\n";
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
            source += "diff.rgb += shadow * " + LightProperty(i, "diffuse") + ";\n";

            if (props.hasShadowMap() && i == 0)
                ApplyShadow(false);
        }
    }

    if (util::is_set(props.texUsage, TexUsage::DiffuseTexture))
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
    if (util::is_set(props.texUsage, TexUsage::OverlayTexture))
    {
        source += "vec4 overlayColor = texture2D(overlayTex, overlayTexCoord.st);\n";
        source += "color.rgb = mix(color.rgb, overlayColor.rgb, overlayColor.a);\n";
    }

    if (props.hasSpecular())
    {
        // Add in the specular color
        if (util::is_set(props.texUsage, TexUsage::SpecularInDiffuseAlpha))
            source += "gl_FragColor = vec4(color.rgb, 1.0) * diff + float(color.a) * spec;\n";
        else if (util::is_set(props.texUsage, TexUsage::SpecularTexture))
            source += "gl_FragColor = color * diff + texture2D(specTex, " + specTexCoord + ".st) * spec;\n";
        else
            source += "gl_FragColor = color * diff + spec;\n";
    }
    else
    {
        source += "gl_FragColor = color * diff;\n";
    }

    if (util::is_set(props.texUsage, TexUsage::NightTexture))
    {
        if (useSeparateDiffuse)
        {
            for (int k = 0; k < props.nLights - 1; k++)
                source += SeparateDiffuse(k) + " + ";
            source += SeparateDiffuse(props.nLights - 1) + ";\n";
        }

        source += NightTextureBlend();
        source += "gl_FragColor += texture2D(nightTex, " + nightTexCoord + ".st) * totalLight;\n";
    }

    // Add in the emissive color
    // TODO: support a constant emissive color, not just an emissive texture
    if (util::is_set(props.texUsage, TexUsage::EmissiveTexture))
    {
        source += "gl_FragColor += texture2D(emissiveTex, " + emissiveTexCoord + ".st);\n";
    }

    // Include the effect of atmospheric scattering.
    if (props.hasScattering())
    {
        source += DeclareLocal("scatterEx", Shader_Vector3);
        source += DeclareLocal("scatterColor", Shader_Vector3);
        source += AtmosphericEffects(props);
        source += "gl_FragColor.rgb = gl_FragColor.rgb * scatterEx + scatterColor;\n";
    }

    source += "}\n";

    DumpFSSource(source);

    GLFragmentShader* fs = nullptr;
    GLShaderStatus status = GLShaderLoader::CreateFragmentShader(source, &fs);
    return status == GLShaderStatus::OK ? fs : nullptr;
}

GLVertexShader*
ShaderManager::buildRingsVertexShader(const ShaderProperties& props)
{
    std::string source(VersionHeader);
    source += CommonHeader;
    source += VertexHeader;
    source += CommonAttribs;

    source += DeclareUniform("ringWidth", Shader_Float);
    source += DeclareUniform("ringRadius", Shader_Float);

    source += DeclareLights(props);

    source += DeclareOutput("position", Shader_Vector3);
    if (props.hasEclipseShadows())
    {
        source += DeclareOutput("shadowDepths", Shader_Vector4);
    }

    if (util::is_set(props.texUsage, TexUsage::DiffuseTexture))
        source += DeclareOutput("diffTexCoord", Shader_Vector2);

    source += VPFunction(props.fishEyeOverride != FisheyeOverrideMode::Disabled && fisheyeEnabled);

    source += "\nvoid main(void)\n{\n";

    if (util::is_set(props.texUsage, TexUsage::DiffuseTexture))
        source += "diffTexCoord = " + TexCoord2D(0, false) + ";\n";

    source += "position = in_Position.xyz * (ringRadius + ringWidth * in_TexCoord0.s);\n";
    if (props.hasEclipseShadows())
    {
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            source += ShadowDepth(i) + " = dot(position, " +
                       LightProperty(i, "direction") + ");\n";
        }
    }

    source += "set_vp(vec4(position.xyz, 1.0));\n";
    source += "}\n";

    DumpVSSource(source);

    GLVertexShader* vs = nullptr;
    GLShaderStatus status = GLShaderLoader::CreateVertexShader(source, &vs);
    return status == GLShaderStatus::OK ? vs : nullptr;
}


GLFragmentShader*
ShaderManager::buildRingsFragmentShader(const ShaderProperties& props)
{
    std::string source(VersionHeader);
    source += CommonHeader;

    source += DeclareUniform("ambientColor", Shader_Vector3);

    source += DeclareLights(props);

    source += DeclareUniform("eyePosition", Shader_Vector3);
    source += DeclareInput("position", Shader_Vector3);

    if (util::is_set(props.texUsage, TexUsage::DiffuseTexture))
    {
        source += DeclareInput("diffTexCoord", Shader_Vector2);
        source += DeclareUniform("diffTex", Shader_Sampler2D);
    }

    if (props.hasEclipseShadows())
    {
        source += DeclareInput("shadowDepths", Shader_Vector4);

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
    source += "vec3 eyeDir = normalize(eyePosition - position);\n";

    source += DeclareLocal("color", Shader_Vector4);
    if (util::is_set(props.texUsage, TexUsage::DiffuseTexture))
        source += "color = texture2D(diffTex, diffTexCoord.st);\n";
    else
        source += "color = vec4(1.0);\n";
    source += DeclareLocal("opticalDepth", Shader_Float, "color.a");
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
        source += "litSide = 1.0 - step(0.0, " + LightProperty(i, "direction") + ".y * eyeDir.y);\n";
        //source += assign("litSide", 1.0f - step(0.0f, sh_vec3("eyePosition")["y"]));

        source += "intensity = (dot(" + LightProperty(i, "direction") + ", eyeDir) + 1.0) * 0.5;\n";
        source += "intensity = mix(intensity, intensity * (1.0 - opticalDepth), litSide);\n";
        if (props.getEclipseShadowCountForLight(i) > 0)
        {
            source += "shadow = 1.0;\n";
            source += Shadow(i, 0);
            source += "shadow = min(1.0, shadow + step(0.0, " + ShadowDepth(i) + "));\n";
#if 0
            source += "diff.rgb += (shadow * " + SeparateDiffuse(i) + ") * " +
                FragLightProperty(i, "color") + ";\n";
#endif
            source += "diff.rgb += (shadow * intensity) * " + LightProperty(i, "diffuse") + ";\n";
        }
        else
        {
            source += "diff.rgb += intensity * " + LightProperty(i, "diffuse") + ";\n";
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
    return status == GLShaderStatus::OK ? fs : nullptr;
}


GLVertexShader*
ShaderManager::buildAtmosphereVertexShader(const ShaderProperties& props)
{
    std::string source(VersionHeader);
    source += "// buildAtmosphereVertexShader\n";
    source += CommonHeader;
    source += VertexHeader;
    source += CommonAttribs;
    if (props.hasTextureCoordTransform())
        source += TextureTransformUniforms;

    source += DeclareLights(props);
    source += DeclareUniform("eyePosition", Shader_Vector3);
    source += DeclareOutput("position", Shader_Vector3);
    source += DeclareOutput("normal", Shader_Vector3);

    source += VPFunction(props.fishEyeOverride != FisheyeOverrideMode::Disabled && fisheyeEnabled);

    // Begin main() function
    source += "\nvoid main(void)\n{\n";
    source += "    position = in_Position.xyz;\n";
    source += "    normal = in_Normal;\n";
    source += VertexPosition(props);
    source += "}\n";

    DumpVSSource(source);

    GLVertexShader* vs = nullptr;
    GLShaderStatus status = GLShaderLoader::CreateVertexShader(source, &vs);
    return status == GLShaderStatus::OK ? vs : nullptr;
}


GLFragmentShader*
ShaderManager::buildAtmosphereFragmentShader(const ShaderProperties& props)
{
    std::string source(VersionHeader);
    source += CommonHeader;

    source += DeclareLights(props);
    source += DeclareUniform("eyePosition", Shader_Vector3);
    source += ScatteringConstantDeclarations(props);

    source += DeclareInput("position", Shader_Vector3);
    source += DeclareInput("normal", Shader_Vector3);

    for (unsigned i = 0; i < props.nLights; i++)
        source += DeclareLocal(ScatteredColor(i), Shader_Vector3);

    source += "\nvoid main(void)\n{\n";

    source += "vec3 nposition = normalize(position);\n";
    source += "vec3 N = normalize(normal);\n";
    source += "vec3 eyeDir = normalize(eyePosition - nposition);\n";
    source += "float NV = dot(N, eyeDir);\n";

    source += DeclareLocal("NL", Shader_Float);
    source += DeclareLocal("scatterEx", Shader_Vector3);
    source += AtmosphericEffects(props);

    // Sum the contributions from each light source
    source += "vec3 color = vec3(0.0);\n";

    // Only do scattering calculations for the primary light source
    // TODO: Eventually handle multiple light sources, and removed the 'min'
    // from the line below.
    for (unsigned i = 0; i < std::min(static_cast<unsigned int>(props.nLights), 1u); i++)
    {
        source += "    float cosTheta = dot(eyeDir, " + LightProperty(i, "direction") + ");\n";
        source += ScatteringPhaseFunctions(props);

        // TODO: Consider premultiplying by invScatterCoeffSum
        source += "    color += (phRayleigh * rayleighCoeff + phMie * mieCoeff) * invScatterCoeffSum * " + ScatteredColor(i) + ";\n";
    }

    source += "    gl_FragColor = vec4(color, dot(scatterEx, vec3(0.333)));\n";
    source += "}\n";

    DumpFSSource(source);

    GLFragmentShader* fs = nullptr;
    GLShaderStatus status = GLShaderLoader::CreateFragmentShader(source, &fs);
    return status == GLShaderStatus::OK ? fs : nullptr;
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
    if (props.hasTextureCoordTransform())
        source += TextureTransformUniforms;

    source += DeclareUniform("opacity", Shader_Float);

    // There are no light sources used for the emissive light model, but
    // we still need the diffuse property of light 0. For other lighting
    // models, the material color is premultiplied with the light color.
    // Emissive shaders interoperate better with other shaders if they also
    // take the color from light source 0.
#ifdef USE_GLSL_STRUCTS
    source += std::string("uniform struct {\n   vec3 diffuse;\n} lights[1];\n");
#else
    source += DeclareUniform("light0_diffuse", Shader_Vector3);
#endif

    if (props.usePointSize())
        source += PointSizeDeclaration();

    source += DeclareOutput("v_Color", Shader_Vector4);
    source += DeclareOutput("v_TexCoord0", Shader_Vector2);

    source += VPFunction(props.fishEyeOverride != FisheyeOverrideMode::Disabled && fisheyeEnabled);

    // Begin main() function
    source += "\nvoid main(void)\n{\n";

    // Optional texture coordinates (generated automatically for point
    // sprites.)
    if (util::is_set(props.texUsage, TexUsage::DiffuseTexture) && !props.usePointSize())
        source += "    v_TexCoord0.st = " + TexCoord2D(0, props.hasTextureCoordTransform()) + ";\n";

    // Set the color.

    source += fmt::format("    v_Color = vec4({}, opacity);\n",
                          util::is_set(props.texUsage, TexUsage::VertexColors)
                              ? "in_Color.rgb"
                              : LightProperty(0, "diffuse"));

    // Optional point size
    if (util::is_set(props.texUsage, TexUsage::PointSprite))
        source += PointSizeCalculation();
    else if (util::is_set(props.texUsage, TexUsage::StaticPointSize))
        source += StaticPointSize();

    source += VertexPosition(props);
    source += "}\n";
    // End of main()

    DumpVSSource(source);

    GLVertexShader* vs = nullptr;
    GLShaderStatus status = GLShaderLoader::CreateVertexShader(source, &vs);
    return status == GLShaderStatus::OK ? vs : nullptr;
}


GLFragmentShader*
ShaderManager::buildEmissiveFragmentShader(const ShaderProperties& props)
{
    std::string source(VersionHeader);
    source += CommonHeader;

    if (util::is_set(props.texUsage, TexUsage::DiffuseTexture))
        source += DeclareUniform("diffTex", Shader_Sampler2D);

    if (props.usePointSize())
        source += DeclareInput("pointFade", Shader_Float);

    source += DeclareInput("v_Color", Shader_Vector4);
    source += DeclareInput("v_TexCoord0", Shader_Vector2);

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

    if (util::is_set(props.texUsage, TexUsage::DiffuseTexture))
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
    return status == GLShaderStatus::OK ? fs : nullptr;
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
    if (props.hasTextureCoordTransform())
        source << TextureTransformUniforms;

    source << "// PARTICLE SHADER\n";
    source << "// shadow count: " << props.shadowCounts << std::endl;

    source << DeclareLights(props);

    source << DeclareUniform("eyePosition", Shader_Vector3);

    // TODO: scattering constants

    if (props.usePointSize())
        source << PointSizeDeclaration();

     source << DeclareOutput("v_Color", Shader_Vector4);

    // Shadow parameters
    if (props.usesShadows())
        source << DeclareOutput("position", Shader_Vector3);

    source << VPFunction(props.fishEyeOverride != FisheyeOverrideMode::Disabled && fisheyeEnabled);

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
    if (util::is_set(props.texUsage, TexUsage::PointSprite))
        source << PointSizeCalculation();
    else if (util::is_set(props.texUsage, TexUsage::StaticPointSize))
        source << StaticPointSize();

    source << VertexPosition(props);
    source << "}\n";
    // End of main()

    DumpVSSource(source);

    GLVertexShader* vs = nullptr;
    GLShaderStatus status = GLShaderLoader::CreateVertexShader(source.str(), &vs);
    return status == GLShaderStatus::OK ? vs : nullptr;
}


GLFragmentShader*
ShaderManager::buildParticleFragmentShader(const ShaderProperties& props)
{
    std::ostringstream source;

    source << VersionHeader << CommonHeader;

    if (util::is_set(props.texUsage, TexUsage::DiffuseTexture))
        source << DeclareUniform("diffTex", Shader_Sampler2D);

    if (props.usePointSize())
        source << DeclareInput("pointFade", Shader_Float);

    if (props.usesShadows())
    {
        source << DeclareUniform("ambientColor", Shader_Vector3);
    }

    // Declare shadow parameters
    if (props.usesShadows())
    {
        source << DeclareInput("position", Shader_Vector3);
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

    source << DeclareInput("v_Color", Shader_Vector4);

    // Begin main()
    source << "\nvoid main(void)\n{\n";

    if (util::is_set(props.texUsage, TexUsage::DiffuseTexture))
        source << "    gl_FragColor = v_Color * texture2D(diffTex, gl_PointCoord);\n";
    else
        source << "    gl_FragColor = v_Color;\n";

    source << "}\n";
    // End of main()

    DumpFSSource(source);

    GLFragmentShader* fs = nullptr;
    GLShaderStatus status = GLShaderLoader::CreateFragmentShader(source.str(), &fs);
    return status == GLShaderStatus::OK ? fs : nullptr;
}

CelestiaGLProgram*
ShaderManager::buildProgram(const ShaderProperties& props)
{
    GLProgram* prog = nullptr;
    GLShaderStatus status;

    GLVertexShader* vs = nullptr;
    GLFragmentShader* fs = nullptr;

    if (props.lightModel == LightingModel::RingIllumModel)
    {
        vs = buildRingsVertexShader(props);
        fs = buildRingsFragmentShader(props);
    }
    else if (props.lightModel == LightingModel::AtmosphereModel)
    {
        vs = buildAtmosphereVertexShader(props);
        fs = buildAtmosphereFragmentShader(props);
    }
    else if (props.lightModel == LightingModel::EmissiveModel)
    {
        vs = buildEmissiveVertexShader(props);
        fs = buildEmissiveFragmentShader(props);
    }
    else if (props.lightModel == LightingModel::ParticleModel)
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
        if (status == GLShaderStatus::OK)
        {
            BindAttribLocations(prog);
            status = prog->link();
        }
    }
    else
    {
        status = GLShaderStatus::CompileError;
    }

    delete vs;
    delete fs;

    if (status != GLShaderStatus::OK)
    {
        // If the shader creation failed for some reason, substitute the
        // error shader.
        if (CreateErrorShader(&prog, fisheyeEnabled) != GLShaderStatus::OK)
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
    if (status == GLShaderStatus::OK)
    {
        BindAttribLocations(prog);
        status = prog->link();
    }

    if (status != GLShaderStatus::OK)
    {
        // If the shader creation failed for some reason, substitute the
        // error shader.
        if (CreateErrorShader(&prog, fisheyeEnabled) != GLShaderStatus::OK)
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
    if (status == GLShaderStatus::OK)
    {
        BindAttribLocations(prog);
        status = prog->link();
    }

    if (status != GLShaderStatus::OK)
    {
        // If the shader creation failed for some reason, substitute the
        // error shader.
        if (CreateErrorShader(&prog, fisheyeEnabled) != GLShaderStatus::OK)
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
                             params->nOutVertices);
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
    if (status == GLShaderStatus::OK)
    {
        BindAttribLocations(prog);
        status = prog->link();
    }

    if (status != GLShaderStatus::OK)
    {
        // If the shader creation failed for some reason, substitute the
        // error shader.
        if (CreateErrorShader(&prog, fisheyeEnabled) != GLShaderStatus::OK)
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
CelestiaGLProgram::floatParam(const char *paramName)
{
    return FloatShaderParameter(program->getID(), paramName);
}


IntegerShaderParameter
CelestiaGLProgram::intParam(const char *paramName)
{
    return IntegerShaderParameter(program->getID(), paramName);
}

IntegerShaderParameter
CelestiaGLProgram::samplerParam(const char *paramName)
{
    return IntegerShaderParameter(program->getID(), paramName);
}


Vec2ShaderParameter
CelestiaGLProgram::vec2Param(const char *paramName)
{
    return Vec2ShaderParameter(program->getID(), paramName);
}


Vec3ShaderParameter
CelestiaGLProgram::vec3Param(const char *paramName)
{
    return Vec3ShaderParameter(program->getID(), paramName);
}


Vec4ShaderParameter
CelestiaGLProgram::vec4Param(const char *paramName)
{
    return Vec4ShaderParameter(program->getID(), paramName);
}


Mat3ShaderParameter
CelestiaGLProgram::mat3Param(const char *paramName)
{
    return Mat3ShaderParameter(program->getID(), paramName);
}


Mat4ShaderParameter
CelestiaGLProgram::mat4Param(const char *paramName)
{
    return Mat4ShaderParameter(program->getID(), paramName);
}


int
CelestiaGLProgram::attribIndex(const char *paramName) const
{
    return glGetAttribLocation(program->getID(), paramName);
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
        lights[i].direction  = vec3Param(LightProperty(i, "direction").c_str());
        lights[i].diffuse    = vec3Param(LightProperty(i, "diffuse").c_str());
        lights[i].specular   = vec3Param(LightProperty(i, "specular").c_str());
        lights[i].halfVector = vec3Param(LightProperty(i, "halfVector").c_str());
        if (util::is_set(props.texUsage, TexUsage::NightTexture))
            lights[i].brightness = floatParam(LightProperty(i, "brightness").c_str());
        if (props.lightModel == LightingModel::AtmosphereModel || props.hasScattering())
            lights[i].color = vec3Param(LightProperty(i, "color").c_str());

        if (props.hasRingShadowForLight(i))
            ringShadowLOD[i] = floatParam(IndexedParameter("ringShadowLOD", i).c_str());
        for (unsigned int j = 0; j < props.getEclipseShadowCountForLight(i); j++)
        {
            shadows[i][j].texGenS =
                vec4Param(IndexedParameter("shadowTexGenS", i, j).c_str());
            shadows[i][j].texGenT =
                vec4Param(IndexedParameter("shadowTexGenT", i, j).c_str());
            shadows[i][j].falloff =
                floatParam(IndexedParameter("shadowFalloff", i, j).c_str());
            shadows[i][j].maxDepth =
                floatParam(IndexedParameter("shadowMaxDepth", i, j).c_str());
        }
    }

    if (props.hasTextureCoordTransform())
    {
        for (unsigned int i = 0; i < texCoordTransforms.size(); ++i)
        {
            texCoordTransforms[i].base = vec2Param(fmt::format("texCoordBase{}", i).c_str());
            texCoordTransforms[i].delta = vec2Param(fmt::format("texCoordDelta{}", i).c_str());
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

    if (util::is_set(props.texUsage, TexUsage::RingShadowTexture))
    {
        ringWidth            = floatParam("ringWidth");
        ringRadius           = floatParam("ringRadius");
        ringPlane            = vec4Param("ringPlane");
        ringCenter           = vec3Param("ringCenter");
    }
    else if (props.lightModel == LightingModel::RingIllumModel)
    {
        ringWidth            = floatParam("ringWidth");
        ringRadius           = floatParam("ringRadius");
    }

    textureOffset = floatParam("textureOffset");

    if (util::is_set(props.texUsage, TexUsage::CloudShadowTexture))
    {
        cloudHeight         = floatParam("cloudHeight");
        shadowTextureOffset = floatParam("cloudShadowTexOffset");
    }

    if (util::is_set(props.texUsage, TexUsage::ShadowMapTexture))
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

    if (util::is_set(props.lightModel, LightingModel::LunarLambertModel))
    {
        lunarLambert         = floatParam("lunarLambert");
    }

    if (props.usePointSize())
    {
        pointScale           = floatParam("pointScale");
    }

    if (util::is_set(props.texUsage, TexUsage::LineAsTriangles))
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

    if (util::is_set(props.texUsage, TexUsage::DiffuseTexture))
    {
        int slot = glGetUniformLocation(program->getID(), "diffTex");
        if (slot != -1)
            glUniform1i(slot, nSamplers++);
    }

    if (util::is_set(props.texUsage, TexUsage::NormalTexture))
    {
        int slot = glGetUniformLocation(program->getID(), "normTex");
        if (slot != -1)
            glUniform1i(slot, nSamplers++);
    }

    if (util::is_set(props.texUsage, TexUsage::SpecularTexture))
    {
        int slot = glGetUniformLocation(program->getID(), "specTex");
        if (slot != -1)
            glUniform1i(slot, nSamplers++);
    }

    if (util::is_set(props.texUsage, TexUsage::NightTexture))
    {
        int slot = glGetUniformLocation(program->getID(), "nightTex");
        if (slot != -1)
            glUniform1i(slot, nSamplers++);
    }

    if (util::is_set(props.texUsage, TexUsage::EmissiveTexture))
    {
        int slot = glGetUniformLocation(program->getID(), "emissiveTex");
        if (slot != -1)
            glUniform1i(slot, nSamplers++);
    }

    if (util::is_set(props.texUsage, TexUsage::OverlayTexture))
    {
        int slot = glGetUniformLocation(program->getID(), "overlayTex");
        if (slot != -1)
            glUniform1i(slot, nSamplers++);
    }

    if (util::is_set(props.texUsage, TexUsage::RingShadowTexture))
    {
        int slot = glGetUniformLocation(program->getID(), "ringTex");
        if (slot != -1)
            glUniform1i(slot, nSamplers++);
    }

    if (util::is_set(props.texUsage, TexUsage::CloudShadowTexture))
    {
        int slot = glGetUniformLocation(program->getID(), "cloudShadowTex");
        if (slot != -1)
            glUniform1i(slot, nSamplers++);
    }

    if (util::is_set(props.texUsage, TexUsage::ShadowMapTexture))
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

    Eigen::Vector3f diffuseColor = materialDiffuse.toVector3();
    Eigen::Vector3f specularColor = materialSpecular.toVector3();

    for (unsigned int i = 0; i < nLights; i++)
    {
        const DirectionalLight& light = ls.lights[i];

        Eigen::Vector3f lightColor = light.color.toVector3() * light.irradiance;
        lights[i].color = light.color.toVector3();
        lights[i].direction = light.direction_obj;

        // Include a phase-based normalization factor to prevent planets from appearing
        // too dim when rendered with non-Lambertian photometric functions.
        float cosPhaseAngle = light.direction_obj.dot(ls.eyeDir_obj);
        if (util::is_set(props.lightModel, LightingModel::LunarLambertModel))
        {
            float photometricNormFactor = std::max(1.0f, 1.0f + cosPhaseAngle * 0.5f);
            lightColor *= photometricNormFactor;
        }

        lights[i].diffuse = lightColor.cwiseProduct(diffuseColor);
        lights[i].brightness = lightColor.maxCoeff();
        lights[i].specular = lightColor.cwiseProduct(specularColor);

        Eigen::Vector3f halfAngle_obj = ls.eyeDir_obj + light.direction_obj;
        if (halfAngle_obj.norm() != 0.0f)
            halfAngle_obj.normalize();
        lights[i].halfVector = halfAngle_obj;
    }

    eyePosition = ls.eyePos_obj;
    ambientColor = ls.ambientColor.cwiseProduct(diffuseColor) + materialEmissive.toVector3();
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
