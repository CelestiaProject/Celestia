// shadermanager.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "celutil/util.h"
#include "shadermanager.h"
#include <GL/glew.h>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdio>
#include <cassert>
#include <Eigen/Geometry>

using namespace Eigen;
using namespace std;

// GLSL on Mac OS X appears to have a bug that precludes us from using structs
// #define USE_GLSL_STRUCTS

ShaderManager g_ShaderManager;


static const char* errorVertexShaderSource =
    "void main(void) {\n"
    "   gl_Position = ftransform();\n"
    "}\n";
static const char* errorFragmentShaderSource =
    "void main(void) {\n"
    "   gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
    "}\n";


static const string CommonHeader("#version 110\n");

ShaderManager&
GetShaderManager()
{
    return g_ShaderManager;
}


ShaderProperties::ShaderProperties() :
    nLights(0),
    texUsage(0),
    lightModel(DiffuseModel),
    shadowCounts(0),
    effects(0)
{
}


bool
ShaderProperties::usesShadows() const
{
    return  (texUsage & RingShadowTexture) != 0 ||
             (texUsage & CloudShadowTexture) != 0 ||
             shadowCounts != 0;
}


bool
ShaderProperties::usesFragmentLighting() const
{
    if ((texUsage & NormalTexture) != 0 || lightModel == PerPixelSpecularModel)
        return true;
    else
        return false;
}


unsigned int
ShaderProperties::getShadowCountForLight(unsigned int i) const
{
    return (shadowCounts >> i * 2) & 3;
}


void
ShaderProperties::setShadowCountForLight(unsigned int light, unsigned int n)
{
    assert(n <= MaxShaderShadows);
    assert(light < MaxShaderLights);
    if (n <= MaxShaderShadows && light < MaxShaderLights)
    {
        shadowCounts &= ~(3 << light * 2);
        shadowCounts |= n << light * 2;
    }
}


bool
ShaderProperties::hasShadowsForLight(unsigned int light) const
{
    assert(light < MaxShaderLights);
    return getShadowCountForLight(light) != 0 ||
            (texUsage & (RingShadowTexture | CloudShadowTexture)) != 0;
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
    else if (p1.texUsage < p0.texUsage)
        return false;

    if (p0.nLights < p1.nLights)
        return true;
    else if (p1.nLights < p0.nLights)
        return false;

    if (p0.shadowCounts < p1.shadowCounts)
        return true;
    else if (p1.shadowCounts < p0.shadowCounts)
        return false;

    if (p0.effects < p1.effects)
        return true;
    else if (p1.effects < p0.effects)
        return false;

    return (p0.lightModel < p1.lightModel);
}


ShaderManager::ShaderManager()
{
#if defined(_DEBUG) || defined(DEBUG)
    // Only write to shader log file if this is a debug build

    if (g_shaderLogFile == NULL)
#ifdef _WIN32
        g_shaderLogFile = new ofstream("shaders.log");
#else
        g_shaderLogFile = new ofstream("/tmp/celestia-shaders.log");
#endif

#endif
}


ShaderManager::~ShaderManager()
{
}


CelestiaGLProgram*
ShaderManager::getShader(const ShaderProperties& props)
{
    map<ShaderProperties, CelestiaGLProgram*>::iterator iter = shaders.find(props);
    if (iter != shaders.end())
    {
        // Shader already exists
        return iter->second;
    }
    else
    {
        // Create a new shader and add it to the table of created shaders
        CelestiaGLProgram* prog = buildProgram(props);
        shaders[props] = prog;

        return prog;
    }
}


static string
LightProperty(unsigned int i, const char* property)
{
    char buf[64];

#ifndef USE_GLSL_STRUCTS
    sprintf(buf, "light%d_%s", i, property);
#else
    sprintf(buf, "lights[%d].%s", i, property);
#endif
    return string(buf);
}


static string
FragLightProperty(unsigned int i, const char* property)
{
    char buf[64];
    sprintf(buf, "light%s%d", property, i);
    return string(buf);
}

#if 0
static string
IndexedParameter(const char* name, unsigned int index)
{
    char buf[64];
    sprintf(buf, "%s%d", name, index);
    return string(buf);
}
#endif

static string
IndexedParameter(const char* name, unsigned int index0, unsigned int index1)
{
    char buf[64];
    sprintf(buf, "%s%d_%d", name, index0, index1);
    return string(buf);
}


static string
RingShadowTexCoord(unsigned int index)
{
    char buf[64];
    sprintf(buf, "ringShadowTexCoord.%c", "xyzw"[index]);
    return string(buf);
}


static string
CloudShadowTexCoord(unsigned int index)
{
    char buf[64];
    sprintf(buf, "cloudShadowTexCoord%d", index);
    return string(buf);
}


static string
VarScatterInVS()
{
    return string("gl_FrontSecondaryColor.rgb");
}

static string
VarScatterInFS()
{
    return string("gl_SecondaryColor.rgb");
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


static string
DeclareLights(const ShaderProperties& props)
{
    if (props.nLights == 0)
        return string("");

#ifndef USE_GLSL_STRUCTS
    ostringstream stream;

    for (unsigned int i = 0; i < props.nLights; i++)
    {
        stream << "uniform vec3 light" << i << "_direction;\n";
        stream << "uniform vec3 light" << i << "_diffuse;\n";
        stream << "uniform vec3 light" << i << "_specular;\n";
        stream << "uniform vec3 light" << i << "_halfVector;\n";
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
    stream << "} lights[%d];\n";
#endif

    return stream.str();
}


static string
SeparateDiffuse(unsigned int i)
{
    // Used for packing multiple diffuse factors into the diffuse color.
    char buf[32];
    sprintf(buf, "diffFactors.%c", "xyzw"[i & 3]);
    return string(buf);
}


static string
SeparateSpecular(unsigned int i)
{
    // Used for packing multiple specular factors into the specular color.
    char buf[32];
    sprintf(buf, "specFactors.%c", "xyzw"[i & 3]);
    return string(buf);
}


// Used by rings shader
static string
ShadowDepth(unsigned int i)
{
    char buf[32];
    sprintf(buf, "shadowDepths.%c", "xyzw"[i & 3]);
    return string(buf);
}


static string
TexCoord2D(unsigned int i)
{
    char buf[64];
    sprintf(buf, "gl_MultiTexCoord%d.st", i);
    return string(buf);
}


// Tangent space light direction
static string
LightDir_tan(unsigned int i)
{
    char buf[32];
    sprintf(buf, "lightDir_tan_%d", i);
    return string(buf);
}


static string
LightHalfVector(unsigned int i)
{
    char buf[32];
    sprintf(buf, "lightHalfVec%d", i);
    return string(buf);
}


static string
ScatteredColor(unsigned int i)
{
    char buf[32];
    sprintf(buf, "scatteredColor%d", i);
    return string(buf);
}


static string
TangentSpaceTransform(const string& dst, const string& src)
{
    string source;

    source += dst + ".x = dot(tangent, " + src + ");\n";
    source += dst + ".y = dot(-bitangent, " + src + ");\n";
    source += dst + ".z = dot(gl_Normal, " + src + ");\n";

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
        source += "NL = max(0.0, dot(gl_Normal, " +
            LightProperty(i, "direction") + "));\n";
    }

    if (props.lightModel == ShaderProperties::SpecularModel)
    {
        source += "H = normalize(" + LightProperty(i, "direction") + " + normalize(eyePosition - gl_Vertex.xyz));\n";
        source += "NH = max(0.0, dot(gl_Normal, H));\n";

        // The calculation below uses the infinite viewer approximation. It's slightly faster,
        // but results in less accurate rendering.
        // source += "NH = max(0.0, dot(gl_Normal, " + LightProperty(i, "halfVector") + "));\n";
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
        source += "float cosAzimuth = dot(normalize(eye - gl_Normal * NV), normalize(light - gl_Normal * NL));\n";
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

    if ((props.texUsage & ShaderProperties::NightTexture) && VSComputesColorSum(props))
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

    if (props.texUsage & ShaderProperties::RingShadowTexture)
    {
        source += "shadow *= (1.0 - texture2D(ringTex, vec2(" +
            RingShadowTexCoord(light) + ", 0.0)).a);\n";
    }

    if (props.texUsage & ShaderProperties::CloudShadowTexture)
    {
        source += "shadow *= (1.0 - texture2D(cloudShadowTex, " +
            CloudShadowTexCoord(light) + ").a * 0.75);\n";
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

    for (unsigned int i = 0; i < props.getShadowCountForLight(light); i++)
        source += Shadow(light, i);

    return source;
}


static string
ScatteringPhaseFunctions(const ShaderProperties&)
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
    source += "    float d = sqrt(rq * rq - qq);\n";
    source += "    vec3 atmEnter = eyePosition + min(0.0, (-rq + d)) * eyeDir;\n";
    source += "    vec3 atmLeave = gl_Vertex.xyz;\n";

    source += "    vec3 atmSamplePoint = (atmEnter + atmLeave) * 0.5;\n";
    //source += "    vec3 atmSamplePoint = atmEnter * 0.2 + atmLeave * 0.8;\n";

    // Compute the distance through the atmosphere from the sample point to the sun
    source += "    vec3 atmSamplePointSun = atmEnter * 0.5 + atmLeave * 0.5;\n";
    source += "    rq = dot(atmSamplePointSun, " + LightProperty(0, "direction") + ");\n";
    source += "    qq = dot(atmSamplePointSun, atmSamplePointSun) - atmosphereRadius.y;\n";
    source += "    d = sqrt(rq * rq - qq);\n";
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

    if (hasAbsorption)
    {
        source += "    vec3 sunColor = exp(-extinctionCoeff * density * distSun);\n";
        source += "    vec3 ex = exp(-extinctionCoeff * density * distAtm);\n";
    }
    else
    {
        source += "    vec3 sunColor = exp(-scatterCoeffSum * density * distSun);\n";
        source += "    vec3 ex = exp(-scatterCoeffSum * density * distAtm);\n";
    }

    string scatter;
    if (hasAbsorption)
    {
        scatter = "(1.0 - exp(-scatterCoeffSum * density * distAtm))";
    }
    else
    {
        // If there's no absorption, the extinction coefficients are just the scattering coefficients,
        // so there's no need to recompute the scattering.
        scatter = "(1.0 - ex)";
    }

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
    source += "    float d = sqrt(rq * rq - qq);\n";
    source += "    vec3 atmEnter = eyePosition + min(0.0, (-rq + d)) * eyeDir;\n";
    source += "    vec3 atmLeave = gl_Vertex.xyz;\n";

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
    source += "        d = sqrt(rq * rq - qq);\n";
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

    return source;
}


string
PointSizeCalculation()
{
    return string("gl_PointSize = pointScale * pointSize / length(vec3(gl_ModelViewMatrix * gl_Vertex));\n");
}


GLVertexShader*
ShaderManager::buildVertexShader(const ShaderProperties& props)
{
    string source = CommonHeader;

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

    if (props.isViewDependent() || props.hasScattering())
    {
        source += "vec3 eyeDir = normalize(eyePosition - gl_Vertex.xyz);\n";
        if (!props.usesTangentSpaceLighting())
            source += "float NV = dot(gl_Normal, eyeDir);\n";
    }

    if (props.texUsage & ShaderProperties::PointSprite)
    {
        source += "uniform float pointScale;\n";
        source += "attribute float pointSize;\n";
    }

    if (props.usesTangentSpaceLighting())
    {
        source += "attribute vec3 tangent;\n";
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
            source += "vec3 eyeDir = normalize(eyePosition - gl_Vertex.xyz);\n";
        }
    }
    else
    {
        source += "uniform vec3 ambientColor;\n";
        source += "uniform float opacity;\n";
        source += "varying vec4 diff;\n";
        if (props.lightModel == ShaderProperties::SpecularModel)
        {
            source += "varying vec4 spec;\n";
            source += "vec3 eyeDir = normalize(eyePosition - gl_Vertex.xyz);\n";
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
    }

    // Shadow parameters
    if (props.shadowCounts != 0)
    {
        source += "varying vec3 position_obj;\n";
    }

    if (props.texUsage & ShaderProperties::RingShadowTexture)
    {
        source += "uniform float ringWidth;\n";
        source += "uniform float ringRadius;\n";
        source += "varying vec4 ringShadowTexCoord;\n";
    }

    if (props.texUsage & ShaderProperties::CloudShadowTexture)
    {
        source += "uniform float cloudShadowTexOffset;\n";
        source += "uniform float cloudHeight;\n";
        for (unsigned int i = 0; i < props.nLights; i++)
            source += "varying vec2 " + CloudShadowTexCoord(i) + ";\n";
    }

    // Begin main() function
    source += "\nvoid main(void)\n{\n";
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
        source += "vec3 bitangent = cross(gl_Normal, tangent);\n";
        if (props.isViewDependent() &&
            props.lightModel != ShaderProperties::SpecularModel)
        {
            source += TangentSpaceTransform("eyeDir_tan", "eyeDir");
        }
    }
    else if (props.lightModel == ShaderProperties::PerPixelSpecularModel)
    {
        source += "normal = gl_Normal;\n";
    }
    else if (props.usesShadows())
    {
    }
    else
    {
        source += "diff = vec4(ambientColor, opacity);\n";
        if (props.hasSpecular())
            source += "spec = vec4(0.0, 0.0, 0.0, 0.0);\n";
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
    if (props.texUsage & ShaderProperties::RingShadowTexture)
    {
        source += "vec3 ringShadowProj;\n";
        for (unsigned int j = 0; j < props.nLights; j++)
        {
            source += "ringShadowProj = gl_Vertex.xyz + " +
                LightProperty(j, "direction") +
                " * max(0.0, gl_Vertex.y / -" +
                LightProperty(j, "direction") + ".y);\n";
            source += RingShadowTexCoord(j) +
                " = (length(ringShadowProj) - ringRadius) * ringWidth;\n";
        }
    }

    if (props.texUsage & ShaderProperties::CloudShadowTexture)
    {
        for (unsigned int j = 0; j < props.nLights; j++)
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
            source += "    float rq = dot(" + LightProperty(j, "direction") + ", gl_Vertex.xyz);\n";
            source += "    float qq = dot(gl_Vertex.xyz, gl_Vertex.xyz) - cloudHeight * cloudHeight;\n";
            source += "    float d = sqrt(rq * rq - qq);\n";
            source += "    vec3 cloudSpherePos = (gl_Vertex.xyz + (-rq + d) * " + LightProperty(j, "direction") + ");\n";
            //source += "    vec3 cloudSpherePos = gl_Vertex.xyz;\n";

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

    if (props.hasScattering())
    {
        source += AtmosphericEffects(props);
    }

    if ((props.texUsage & ShaderProperties::OverlayTexture) && !props.hasSharedTextureCoords())
    {
        source += "overlayTexCoord = " + TexCoord2D(nTexCoords) + ";\n";
        nTexCoords++;
    }

    if (props.shadowCounts != 0)
    {
        source += "position_obj = gl_Vertex.xyz;\n";
    }

    if ((props.texUsage & ShaderProperties::PointSprite) != 0)
        source += PointSizeCalculation();

    source += "gl_Position = ftransform();\n";
    source += "}\n";

    if (g_shaderLogFile != NULL)
    {
        *g_shaderLogFile << "Vertex shader source:\n";
        DumpShaderSource(*g_shaderLogFile, source);
        *g_shaderLogFile << '\n';
    }

    GLVertexShader* vs = NULL;
    GLShaderStatus status = GLShaderLoader::CreateVertexShader(source, &vs);
    if (status != ShaderStatus_OK)
        return NULL;
    else
        return vs;
}


GLFragmentShader*
ShaderManager::buildFragmentShader(const ShaderProperties& props)
{
    string source = CommonHeader;

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
        source += "vec4 diff = vec4(ambientColor, opacity);\n";
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
        source += "vec4 diff = vec4(ambientColor, opacity);\n";
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
        source += "vec4 diff = vec4(ambientColor, opacity);\n";
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
            for (unsigned int j = 0; j < props.getShadowCountForLight(i); j++)
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

    if (props.texUsage & ShaderProperties::RingShadowTexture)
    {
        source += "uniform sampler2D ringTex;\n";
        source += "varying vec4 ringShadowTexCoord;\n";
    }

    if (props.texUsage & ShaderProperties::CloudShadowTexture)
    {
        source += "uniform sampler2D cloudShadowTex;\n";
        for (unsigned int i = 0; i < props.nLights; i++)
            source += "varying vec2 " + CloudShadowTexCoord(i) + ";\n";
    }

    source += "\nvoid main(void)\n{\n";
    source += "vec4 color;\n";

    if (props.usesShadows())
    {
        // Temporaries required for shadows
        source += "float shadow;\n";
        if (props.shadowCounts != 0)
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
        }
    }
    else if (props.usesShadows())
    {
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
        }
    }

    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        if (props.texUsage & ShaderProperties::PointSprite)
            source += "color = texture2D(diffTex, gl_TexCoord[0].st);\n";
        else
            source += "color = texture2D(diffTex, " + diffTexCoord + ".st);\n";
    }
    else
    {
        source += "color = vec4(1.0, 1.0, 1.0, 1.0);\n";
    }

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
                    source += "0.0f\n";
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

    if (g_shaderLogFile != NULL)
    {
        *g_shaderLogFile << "Fragment shader source:\n";
        DumpShaderSource(*g_shaderLogFile, source);
        *g_shaderLogFile << '\n';
    }

    GLFragmentShader* fs = NULL;
    GLShaderStatus status = GLShaderLoader::CreateFragmentShader(source, &fs);
    if (status != ShaderStatus_OK)
        return NULL;
    else
        return fs;
}


GLVertexShader*
ShaderManager::buildRingsVertexShader(const ShaderProperties& props)
{
    string source = CommonHeader;

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
    source += "vec3 eyeDir = normalize(eyePosition - gl_Vertex.xyz);\n";

    for (unsigned int i = 0; i < props.nLights; i++)
    {
        source += SeparateDiffuse(i) + " = (dot(" +
            LightProperty(i, "direction") + ", eyeDir) + 1.0) * 0.5;\n";
    }

    if (props.texUsage & ShaderProperties::DiffuseTexture)
        source += "diffTexCoord = " + TexCoord2D(0) + ";\n";

    if (props.shadowCounts != 0)
    {
        source += "position_obj = gl_Vertex.xyz;\n";
        for (unsigned int i = 0; i < props.nLights; i++)
        {
            source += ShadowDepth(i) + " = dot(gl_Vertex.xyz, " +
                       LightProperty(i, "direction") + ");\n";
        }
    }

    source += "gl_Position = ftransform();\n";
    source += "}\n";

    if (g_shaderLogFile != NULL)
    {
        *g_shaderLogFile << "Vertex shader source:\n";
        DumpShaderSource(*g_shaderLogFile, source);
        *g_shaderLogFile << '\n';
    }

    GLVertexShader* vs = NULL;
    GLShaderStatus status = GLShaderLoader::CreateVertexShader(source, &vs);
    if (status != ShaderStatus_OK)
        return NULL;
    else
        return vs;
}


GLFragmentShader*
ShaderManager::buildRingsFragmentShader(const ShaderProperties& props)
{
    string source = CommonHeader;

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


    if (props.shadowCounts != 0)
    {
        source += "varying vec3 position_obj;\n";
        source += "varying vec4 shadowDepths;\n";

        for (unsigned int i = 0; i < props.nLights; i++)
        {
            for (unsigned int j = 0; j < props.getShadowCountForLight(i); j++)
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

    if (props.usesShadows())
    {
        // Temporaries required for shadows
        source += "float shadow;\n";
        source += "vec2 shadowCenter;\n";
        source += "float shadowR;\n";
    }

    // Sum the contributions from each light source
    for (unsigned i = 0; i < props.nLights; i++)
    {
        if (props.usesShadows())
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

    if (g_shaderLogFile != NULL)
    {
        *g_shaderLogFile << "Fragment shader source:\n";
        DumpShaderSource(*g_shaderLogFile, source);
        *g_shaderLogFile << '\n';
    }

    GLFragmentShader* fs = NULL;
    GLShaderStatus status = GLShaderLoader::CreateFragmentShader(source, &fs);
    if (status != ShaderStatus_OK)
        return NULL;
    else
        return fs;
}


GLVertexShader*
ShaderManager::buildAtmosphereVertexShader(const ShaderProperties& props)
{
    string source = CommonHeader;

    source += DeclareLights(props);
    source += "uniform vec3 eyePosition;\n";
    source += ScatteringConstantDeclarations(props);
    for (unsigned int i = 0; i < props.nLights; i++)
    {
        source += "varying vec3 " + ScatteredColor(i) + ";\n";
    }

    source += "vec3 eyeDir = normalize(eyePosition - gl_Vertex.xyz);\n";
    source += "float NV = dot(gl_Normal, eyeDir);\n";

    source += "varying vec3 scatterEx;\n";
    source += "varying vec3 eyeDir_obj;\n";

    // Begin main() function
    source += "\nvoid main(void)\n{\n";
    source += "float NL;\n";

    source += AtmosphericEffects(props);

    source += "eyeDir_obj = eyeDir;\n";
    source += "gl_Position = ftransform();\n";
    source += "}\n";

    if (g_shaderLogFile != NULL)
    {
        *g_shaderLogFile << "Vertex shader source:\n";
        DumpShaderSource(*g_shaderLogFile, source);
        *g_shaderLogFile << '\n';
    }

    GLVertexShader* vs = NULL;
    GLShaderStatus status = GLShaderLoader::CreateVertexShader(source, &vs);
    if (status != ShaderStatus_OK)
        return NULL;
    else
        return vs;
}


GLFragmentShader*
ShaderManager::buildAtmosphereFragmentShader(const ShaderProperties& props)
{
    string source = CommonHeader;

    source += "varying vec3 scatterEx;\n";
    source += "varying vec3 eyeDir_obj;\n";

    // Scattering constants
    source += "uniform float mieK;\n";
    source += "uniform float mieCoeff;\n";
    source += "uniform vec3  rayleighCoeff;\n";
    source += "uniform vec3  invScatterCoeffSum;\n";

    unsigned int i;
    for (i = 0; i < props.nLights; i++)
    {
        source += "uniform vec3 " + LightProperty(i, "direction") + ";\n";
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

    if (g_shaderLogFile != NULL)
    {
        *g_shaderLogFile << "Fragment shader source:\n";
        DumpShaderSource(*g_shaderLogFile, source);
        *g_shaderLogFile << '\n';
    }

    GLFragmentShader* fs = NULL;
    GLShaderStatus status = GLShaderLoader::CreateFragmentShader(source, &fs);
    if (status != ShaderStatus_OK)
        return NULL;
    else
        return fs;
}


// The emissive shader ignores all lighting and uses the diffuse color
// as the final fragment color.
GLVertexShader*
ShaderManager::buildEmissiveVertexShader(const ShaderProperties& props)
{
    string source = CommonHeader;

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
        source += "attribute float pointSize;\n";
    }

    // Begin main() function
    source += "\nvoid main(void)\n{\n";

    // Optional texture coordinates (generated automatically for point
    // sprites.)
    if ((props.texUsage & ShaderProperties::DiffuseTexture) &&
        !(props.texUsage & ShaderProperties::PointSprite))
    {
        source += "    gl_TexCoord[0].st = " + TexCoord2D(0) + ";\n";
    }

    // Set the color. 
    string colorSource;
    if (props.texUsage & ShaderProperties::VertexColors)
        colorSource = "gl_Color.rgb";
    else
        colorSource = LightProperty(0, "diffuse");

    source += "    gl_FrontColor = vec4(" + colorSource + ", opacity);\n";

    // Optional point size
    if ((props.texUsage & ShaderProperties::PointSprite) != 0)
        source += PointSizeCalculation();

    source += "    gl_Position = ftransform();\n";

    source += "}\n";
    // End of main()

    if (g_shaderLogFile != NULL)
    {
        *g_shaderLogFile << "Vertex shader source:\n";
        DumpShaderSource(*g_shaderLogFile, source);
        *g_shaderLogFile << '\n';
    }

    GLVertexShader* vs = NULL;
    GLShaderStatus status = GLShaderLoader::CreateVertexShader(source, &vs);
    if (status != ShaderStatus_OK)
        return NULL;
    else
        return vs;
}


GLFragmentShader*
ShaderManager::buildEmissiveFragmentShader(const ShaderProperties& props)
{
    string source = CommonHeader;

    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        source += "uniform sampler2D diffTex;\n";
    }

    // Begin main()
    source += "\nvoid main(void)\n";
    source += "{\n";

    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        source += "    gl_FragColor = gl_Color * texture2D(diffTex, gl_TexCoord[0].st);\n";
    }
    else
    {
        source += "    gl_FragColor = gl_Color;\n";
    }

    source += "}\n";
    // End of main()

    if (g_shaderLogFile != NULL)
    {
        *g_shaderLogFile << "Fragment shader source:\n";
        DumpShaderSource(*g_shaderLogFile, source);
        *g_shaderLogFile << '\n';
    }

    GLFragmentShader* fs = NULL;
    GLShaderStatus status = GLShaderLoader::CreateFragmentShader(source, &fs);
    if (status != ShaderStatus_OK)
        return NULL;
    else
        return fs;
}


// Build the vertex shader used for rendering particle systems.
GLVertexShader*
ShaderManager::buildParticleVertexShader(const ShaderProperties& props)
{
    ostringstream source;
    
    source << CommonHeader;
    
    source << "// PARTICLE SHADER\n";
    source << "// shadow count: " << props.shadowCounts << endl;
    
    source << DeclareLights(props);
    
    source << "uniform vec3 eyePosition;\n";

    // TODO: scattering constants
    
    if (props.texUsage & ShaderProperties::PointSprite)
    {
        source << "uniform float pointScale;\n";
        source << "attribute float pointSize;\n";
    }
    
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

    source << "    vec3 eyeDir = normalize(eyePosition - gl_Vertex.xyz);\n";
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

    // Optional texture coordinates (generated automatically for point
    // sprites.)
    if ((props.texUsage & ShaderProperties::DiffuseTexture) &&
        !(props.texUsage & ShaderProperties::PointSprite))
    {
        source << "    gl_TexCoord[0].st = " << TexCoord2D(0) << ";\n";
    }
    
    // Set the color. Should *always* use vertex colors for color and opacity.
    source << "    gl_FrontColor = gl_Color * brightness;\n";
    
    // Optional point size
    if ((props.texUsage & ShaderProperties::PointSprite) != 0)
        source << PointSizeCalculation();
    
    source << "    gl_Position = ftransform();\n";
    
    source << "}\n";
    // End of main()
    
    if (g_shaderLogFile != NULL)
    {
        *g_shaderLogFile << "Vertex shader source:\n";
        DumpShaderSource(*g_shaderLogFile, source.str());
        *g_shaderLogFile << endl;
    }
    
    GLVertexShader* vs = NULL;
    GLShaderStatus status = GLShaderLoader::CreateVertexShader(source.str(), &vs);
    if (status != ShaderStatus_OK)
        return NULL;
    else
        return vs;
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
            for (unsigned int j = 0; j < props.getShadowCountForLight(i); j++)
            {
                source << "uniform vec4 " << IndexedParameter("shadowTexGenS", i, j) << ";\n";
                source << "uniform vec4 " << IndexedParameter("shadowTexGenT", i, j) << ";\n";
                source << "uniform float " << IndexedParameter("shadowFalloff", i, j) << ";\n";
                source << "uniform float " << IndexedParameter("shadowMaxDepth", i, j) << ";\n";
            }
        }
    }
    
    // Begin main()
    source << "\nvoid main(void)\n";
    source << "{\n";
    
    if (props.texUsage & ShaderProperties::DiffuseTexture)
    {
        source << "    gl_FragColor = gl_Color * texture2D(diffTex, gl_TexCoord[0].st);\n";
    }
    else
    {
        source << "    gl_FragColor = gl_Color;\n";
    }
    
    source << "}\n";
    // End of main()
    
    if (g_shaderLogFile != NULL)
    {
        *g_shaderLogFile << "Fragment shader source:\n";
        DumpShaderSource(*g_shaderLogFile, source.str());
        *g_shaderLogFile << '\n';
    }
    
    GLFragmentShader* fs = NULL;
    GLShaderStatus status = GLShaderLoader::CreateFragmentShader(source.str(), &fs);
    if (status != ShaderStatus_OK)
        return NULL;
    else
        return fs;
}


CelestiaGLProgram*
ShaderManager::buildProgram(const ShaderProperties& props)
{
    GLProgram* prog = NULL;
    GLShaderStatus status;

    GLVertexShader* vs = NULL;
    GLFragmentShader* fs = NULL;

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

    if (vs != NULL && fs != NULL)
    {
        status = GLShaderLoader::CreateProgram(*vs, *fs, &prog);
        if (status == ShaderStatus_OK)
        {
            if (props.texUsage & ShaderProperties::NormalTexture)
            {
                // Tangents always in attribute 6 (should be a constant
                // someplace)
                glBindAttribLocationARB(prog->getID(), 6, "tangent");
            }

            if (props.texUsage & ShaderProperties::PointSprite)
            {
                // Point size is always in attribute 7
                glBindAttribLocationARB(prog->getID(), 7, "pointSize");
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
            if (g_shaderLogFile != NULL)
                *g_shaderLogFile << "Failed to create error shader!\n";
        }
        else
        {
            status = prog->link();
        }
    }

    if (prog == NULL)
        return NULL;
    else
        return new CelestiaGLProgram(*prog, props);
}


CelestiaGLProgram::CelestiaGLProgram(GLProgram& _program,
                                     const ShaderProperties& _props) :
    program(&_program),
    props(_props)
{
    initParameters();
    initSamplers();
};


CelestiaGLProgram::~CelestiaGLProgram()
{
    delete program;
}


FloatShaderParameter
CelestiaGLProgram::floatParam(const string& paramName)
{
    return FloatShaderParameter(program->getID(), paramName.c_str());
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


void
CelestiaGLProgram::initParameters()
{
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

        for (unsigned int j = 0; j < props.getShadowCountForLight(i); j++)
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
    }

    textureOffset = floatParam("textureOffset");

    if (props.texUsage & ShaderProperties::CloudShadowTexture)
    {
        cloudHeight         = floatParam("cloudHeight");
        shadowTextureOffset = floatParam("cloudShadowTexOffset");
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
        int slot = glGetUniformLocationARB(program->getID(), "diffTex");
        if (slot != -1)
            glUniform1iARB(slot, nSamplers++);
    }

    if (props.texUsage & ShaderProperties::NormalTexture)
    {
        int slot = glGetUniformLocationARB(program->getID(), "normTex");
        if (slot != -1)
            glUniform1iARB(slot, nSamplers++);
    }

    if (props.texUsage & ShaderProperties::SpecularTexture)
    {
        int slot = glGetUniformLocationARB(program->getID(), "specTex");
        if (slot != -1)
            glUniform1iARB(slot, nSamplers++);
    }

    if (props.texUsage & ShaderProperties::NightTexture)
    {
        int slot = glGetUniformLocationARB(program->getID(), "nightTex");
        if (slot != -1)
            glUniform1iARB(slot, nSamplers++);
    }

    if (props.texUsage & ShaderProperties::EmissiveTexture)
    {
        int slot = glGetUniformLocationARB(program->getID(), "emissiveTex");
        if (slot != -1)
            glUniform1iARB(slot, nSamplers++);
    }

    if (props.texUsage & ShaderProperties::OverlayTexture)
    {
        int slot = glGetUniformLocationARB(program->getID(), "overlayTex");
        if (slot != -1)
            glUniform1iARB(slot, nSamplers++);
    }

    if (props.texUsage & ShaderProperties::RingShadowTexture)
    {
        int slot = glGetUniformLocationARB(program->getID(), "ringTex");
        if (slot != -1)
            glUniform1iARB(slot, nSamplers++);
    }

    if (props.texUsage & ShaderProperties::CloudShadowTexture)
    {
        int slot = glGetUniformLocationARB(program->getID(), "cloudShadowTex");
        if (slot != -1)
            glUniform1iARB(slot, nSamplers++);
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

        if (props.usesShadows() ||
            props.usesFragmentLighting() ||
            props.lightModel == ShaderProperties::RingIllumModel)
        {
            fragLightColor[i] = lightColor.cwise() * diffuseColor;
            if (props.hasSpecular())
            {
                fragLightSpecColor[i] = lightColor.cwise() * specularColor;
            }
            fragLightBrightness[i] = lightColor.maxCoeff();
        }
        else
        {
            lights[i].diffuse = lightColor.cwise() * diffuseColor;
        }

        lights[i].brightness = lightColor.maxCoeff();
        lights[i].specular = lightColor.cwise() * specularColor;

        Vector3f halfAngle_obj = ls.eyeDir_obj + light.direction_obj;
        if (halfAngle_obj.norm() != 0.0f)
            halfAngle_obj.normalize();
        lights[i].halfVector = halfAngle_obj;
    }

    eyePosition = ls.eyePos_obj;
    ambientColor = ls.ambientColor.cwise() * diffuseColor + 
        Vector3f(materialEmissive.red(), materialEmissive.green(), materialEmissive.blue());
    opacity = materialDiffuse.alpha();
#ifdef USE_HDR
    nightLightScale = _nightLightScale;
#endif
}


// Set GLSL shader constants for shadows from ellipsoid occluders; shadows from
// irregular objects are not handled yet.
void
CelestiaGLProgram::setEclipseShadowParameters(const LightingState& ls,
                                              float planetRadius,
                                              const Eigen::Quaternionf& planetOrientation)
{
    Matrix3f planetTransform = planetOrientation.toRotationMatrix();

    for (unsigned int li = 0;
         li < min(ls.nLights, MaxShaderLights);
         li++)
    {
        if (shadows != NULL)
        {
            unsigned int nShadows = min((size_t) MaxShaderShadows, ls.shadows[li]->size());

            for (unsigned int i = 0; i < nShadows; i++)
            {
                EclipseShadow& shadow = ls.shadows[li]->at(i);
                CelestiaGLProgramShadow& shadowParams = shadows[li][i];

                // Compute shadow parameters: max depth of at the center of the shadow
                // (always 1 if an eclipse is total) and the linear falloff
                // rate from the center to the outer endge of the penumbra.
                float u = shadow.umbraRadius / shadow.penumbraRadius;
                shadowParams.falloff = -shadow.maxDepth / std::max(0.001f, 1.0f - std::fabs(u));
                shadowParams.maxDepth = shadow.maxDepth;

                // Compute the transformation to use for generating texture
                // coordinates from the object vertices.
                Vector3f origin = planetTransform * shadow.origin;
                Vector3f dir    = planetTransform * shadow.direction;
                float scale = planetRadius / shadow.penumbraRadius;
                
                Quaternionf shadowRotation;
                shadowRotation.setFromTwoVectors(Vector3f::UnitY(), dir);
                Matrix3f m = shadowRotation.toRotationMatrix();
                Vector3f sAxis = m * Vector3f::UnitX() * (0.5f * scale);
                Vector3f tAxis = m * Vector3f::UnitZ() * (0.5f * scale);

                Vector4f texGenS;
                Vector4f texGenT;
                texGenS.start<3>() = sAxis;
                texGenT.start<3>() = tAxis;
                texGenS[3] = -origin.dot(sAxis) / planetRadius + 0.5f;
                texGenT[3] = -origin.dot(tAxis) / planetRadius + 0.5f;
                shadowParams.texGenS = texGenS;
                shadowParams.texGenT = texGenT;
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
    float skySphereRadius = atmPlanetRadius + -atmosphere.mieScaleHeight * (float) log(AtmosphereExtinctionThreshold);

    float tMieCoeff        = atmosphere.mieCoeff * objRadius;
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
    Vector3f tScatterCoeffSum = tRayleighCoeff.cwise() + tMieCoeff;
    scatterCoeffSum = tScatterCoeffSum;
    invScatterCoeffSum = tScatterCoeffSum.cwise().inverse();
    extinctionCoeff = tScatterCoeffSum + tAbsorptionCoeff;
}
