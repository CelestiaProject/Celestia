// rendcontext.cpp
//
// Copyright (C) 2004-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <vector>
#include "rendcontext.h"
#include "texmanager.h"
#include "modelgeometry.h"
#include "body.h"
#include "glsupport.h"
#include <celmath/geomutil.h>
#include "render.h"
#include "shadowmap.h"

using namespace cmod;
using namespace Eigen;
using namespace std;

static Material defaultMaterial;

static GLenum GLPrimitiveModes[Mesh::PrimitiveTypeMax] =
{
    GL_TRIANGLES,
    GL_TRIANGLE_STRIP,
    GL_TRIANGLE_FAN,
    GL_LINES,
    GL_LINE_STRIP,
    GL_POINTS,
    GL_POINTS,
};

static GLenum GLComponentTypes[Mesh::FormatMax] =
{
     GL_FLOAT,          // Float1
     GL_FLOAT,          // Float2
     GL_FLOAT,          // Float3
     GL_FLOAT,          // Float4,
     GL_UNSIGNED_BYTE,  // UByte4
};

static int GLComponentCounts[Mesh::FormatMax] =
{
     1,  // Float1
     2,  // Float2
     3,  // Float3
     4,  // Float4,
     4,  // UByte4
};



static void
setStandardVertexArrays(const Mesh::VertexDescription& desc,
                        const void* vertexData);
static void
setExtendedVertexArrays(const Mesh::VertexDescription& desc,
                        const void* vertexData);


static ResourceHandle
GetTextureHandle(Material::TextureResource* texResource)
{
    CelestiaTextureResource* t = reinterpret_cast<CelestiaTextureResource*>(texResource);
    return t ? t->textureHandle() : InvalidResource;
}


RenderContext::RenderContext(const Renderer* _renderer) :
    material(&defaultMaterial),
    renderer(_renderer)
{
}


RenderContext::RenderContext(const Material* _material)
{
    if (_material == nullptr)
        material = &defaultMaterial;
    else
        material = _material;
}


const Material*
RenderContext::getMaterial() const
{
    return material;
}


void
RenderContext::setMaterial(const Material* newMaterial)
{
    if (!locked)
    {
        if (newMaterial == nullptr)
            newMaterial = &defaultMaterial;

        if (renderPass == PrimaryPass)
        {
            if (newMaterial != material)
            {
                material = newMaterial;
                makeCurrent(*material);
            }
        }
        else if (renderPass == EmissivePass)
        {
            if (material->maps[Material::EmissiveMap] !=
                newMaterial->maps[Material::EmissiveMap])
            {
                material = newMaterial;
                makeCurrent(*material);
            }
        }
    }
}


void
RenderContext::setPointScale(float _pointScale)
{
    pointScale = _pointScale;
}


float
RenderContext::getPointScale() const
{
    return pointScale;
}


void
RenderContext::setCameraOrientation(const Quaternionf& q)
{
    cameraOrientation = q;
}


Quaternionf
RenderContext::getCameraOrientation() const
{
    return cameraOrientation;
}


void
RenderContext::drawGroup(const Mesh::PrimitiveGroup& group)
{
    // Skip rendering if this is the emissive pass but there's no
    // emissive texture.
    ResourceHandle emissiveMap = GetTextureHandle(material->maps[Material::EmissiveMap]);

    if (renderPass == EmissivePass && emissiveMap == InvalidResource)
    {
        return;
    }

    if (group.prim == Mesh::SpriteList)
    {
#ifndef GL_ES
        glEnable(GL_POINT_SPRITE);
        glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif
        glActiveTexture(GL_TEXTURE0);
    }

    glDrawElements(GLPrimitiveModes[(int) group.prim],
                   group.nIndices,
                   GL_UNSIGNED_INT,
                   group.indices);
#ifndef GL_ES
    if (group.prim == Mesh::SpriteList)
    {
        glDisable(GL_POINT_SPRITE);
        glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
    }
#endif
}


void
RenderContext::setVertexArrays(const Mesh::VertexDescription& desc,
                               const void* vertexData)
{
    setStandardVertexArrays(desc, vertexData);
    setExtendedVertexArrays(desc, vertexData);

    // Normally, the shader that will be used depends only on the material.
    // But the presence of point size and normals can also affect the
    // shader, so force an update of the material if those attributes appear
    // or disappear in the new set of vertex arrays.
    bool usePointSizeNow = (desc.getAttribute(Mesh::PointSize).format == Mesh::Float1);
    bool useNormalsNow = (desc.getAttribute(Mesh::Normal).format == Mesh::Float3);
    bool useColorsNow = (desc.getAttribute(Mesh::Color0).format != Mesh::InvalidFormat);
    bool useTexCoordsNow = (desc.getAttribute(Mesh::Texture0).format != Mesh::InvalidFormat);

    if (usePointSizeNow != usePointSize ||
        useNormalsNow   != useNormals   ||
        useColorsNow    != useColors    ||
        useTexCoordsNow != useTexCoords)
    {
        usePointSize = usePointSizeNow;
        useNormals = useNormalsNow;
        useColors = useColorsNow;
        useTexCoords = useTexCoordsNow;
        if (getMaterial() != nullptr)
            makeCurrent(*getMaterial());
    }
}

void
RenderContext::setProjectionMatrix(const Eigen::Matrix4f *m)
{
    projectionMatrix = m;
}

void
RenderContext::setModelViewMatrix(const Eigen::Matrix4f *m)
{
    modelViewMatrix = m;
}

void
setStandardVertexArrays(const Mesh::VertexDescription& desc,
                        const void* vertexData)
{
    const Mesh::VertexAttribute& position  = desc.getAttribute(Mesh::Position);
    const Mesh::VertexAttribute& normal    = desc.getAttribute(Mesh::Normal);
    const Mesh::VertexAttribute& color0    = desc.getAttribute(Mesh::Color0);
    const Mesh::VertexAttribute& texCoord0 = desc.getAttribute(Mesh::Texture0);

    // Can't render anything unless we have positions
    if (position.format != Mesh::Float3)
        return;

    // Set up the vertex arrays
    glEnableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glVertexAttribPointer(CelestiaGLProgram::VertexCoordAttributeIndex,
                          3, GL_FLOAT, GL_FALSE, desc.stride,
                          reinterpret_cast<const char*>(vertexData) + position.offset);

    // Set up the normal array
    switch (normal.format)
    {
    case Mesh::Float3:
        glEnableVertexAttribArray(CelestiaGLProgram::NormalAttributeIndex);
        glVertexAttribPointer(CelestiaGLProgram::NormalAttributeIndex,
                              3, GLComponentTypes[(int) normal.format],
                              GL_FALSE, desc.stride,
                              reinterpret_cast<const char*>(vertexData) + normal.offset);
        break;
    default:
        glDisableVertexAttribArray(CelestiaGLProgram::NormalAttributeIndex);
        break;
    }

    GLint normalized = GL_TRUE;
    // Set up the color array
    switch (color0.format)
    {
    case Mesh::Float3:
    case Mesh::Float4:
        normalized = GL_FALSE;
    case Mesh::UByte4:
        glEnableVertexAttribArray(CelestiaGLProgram::ColorAttributeIndex);
        glVertexAttribPointer(CelestiaGLProgram::ColorAttributeIndex,
                              GLComponentCounts[color0.format],
                              GLComponentTypes[color0.format],
                              normalized, desc.stride,
                              reinterpret_cast<const char*>(vertexData) + color0.offset);
        break;
    default:
        glDisableVertexAttribArray(CelestiaGLProgram::ColorAttributeIndex);
        break;
    }

    // Set up the texture coordinate array
    switch (texCoord0.format)
    {
    case Mesh::Float1:
    case Mesh::Float2:
    case Mesh::Float3:
    case Mesh::Float4:
        glEnableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex);
        glVertexAttribPointer(CelestiaGLProgram::TextureCoord0AttributeIndex,
                              GLComponentCounts[(int) texCoord0.format],
                              GLComponentTypes[(int) texCoord0.format],
                              GL_FALSE,
                              desc.stride,
                              reinterpret_cast<const char*>(vertexData) + texCoord0.offset);
        break;
    default:
        glDisableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex);
        break;
    }
}


void
setExtendedVertexArrays(const Mesh::VertexDescription& desc,
                        const void* vertexData)
{
    const Mesh::VertexAttribute& tangent  = desc.getAttribute(Mesh::Tangent);
    const auto* vertices = reinterpret_cast<const char*>(vertexData);

    switch (tangent.format)
    {
    case Mesh::Float3:
        glEnableVertexAttribArray(CelestiaGLProgram::TangentAttributeIndex);
        glVertexAttribPointer(CelestiaGLProgram::TangentAttributeIndex,
                                      GLComponentCounts[(int) tangent.format],
                                      GLComponentTypes[(int) tangent.format],
                                      GL_FALSE,
                                      desc.stride,
                                      vertices + tangent.offset);
        break;
    default:
        glDisableVertexAttribArray(CelestiaGLProgram::TangentAttributeIndex);
        break;
    }

    const Mesh::VertexAttribute& pointsize = desc.getAttribute(Mesh::PointSize);
    switch (pointsize.format)
    {
    case Mesh::Float1:
        glEnableVertexAttribArray(CelestiaGLProgram::PointSizeAttributeIndex);
        glVertexAttribPointer(CelestiaGLProgram::PointSizeAttributeIndex,
                                      GLComponentCounts[(int) pointsize.format],
                                      GLComponentTypes[(int) pointsize.format],
                                      GL_FALSE,
                                      desc.stride,
                                      vertices + pointsize.offset);
        break;
    default:
        glDisableVertexAttribArray(CelestiaGLProgram::PointSizeAttributeIndex);
        break;
    }
}


/***** GLSL render context ******/

GLSL_RenderContext::GLSL_RenderContext(const Renderer* renderer,
                                       const LightingState& ls,
                                       float _objRadius,
                                       const Quaternionf& orientation) :
    RenderContext(renderer),
    lightingState(ls),
    objRadius(_objRadius),
    objScale(Vector3f::Constant(_objRadius)),
    objOrientation(orientation)
{
    initLightingEnvironment();
}


GLSL_RenderContext::GLSL_RenderContext(const Renderer* renderer,
                                       const LightingState& ls,
                                       const Eigen::Vector3f& _objScale,
                                       const Quaternionf& orientation) :
    RenderContext(renderer),
    lightingState(ls),
    objRadius(_objScale.maxCoeff()),
    objScale(_objScale),
    objOrientation(orientation)
{
    initLightingEnvironment();
}


GLSL_RenderContext::~GLSL_RenderContext()
{
    glDisableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glDisableVertexAttribArray(CelestiaGLProgram::NormalAttributeIndex);
    glDisableVertexAttribArray(CelestiaGLProgram::ColorAttributeIndex);
    glDisableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex);
    glDisableVertexAttribArray(CelestiaGLProgram::TangentAttributeIndex);
    glDisableVertexAttribArray(CelestiaGLProgram::PointSizeAttributeIndex);
}


void
GLSL_RenderContext::initLightingEnvironment()
{
    // Set the light and shadow environment, which is constant for the entire model.
    // The material properties will be set per mesh.
    shaderProps.nLights = min(lightingState.nLights, MaxShaderLights);

    // Set the shadow information.
    // Track the total number of shadows; if there are too many, we'll have
    // to fall back to multipass.
    unsigned int totalShadows = 0;
    for (unsigned int li = 0; li < lightingState.nLights; li++)
    {
        if (lightingState.shadows[li] && !lightingState.shadows[li]->empty())
        {
            unsigned int nShadows = (unsigned int) min((size_t) MaxShaderEclipseShadows, lightingState.shadows[li]->size());
            shaderProps.setEclipseShadowCountForLight(li, nShadows);
            totalShadows += nShadows;
        }
    }

}

void
GLSL_RenderContext::makeCurrent(const Material& m)
{
    Texture* textures[4] = { nullptr, nullptr, nullptr, nullptr };
    unsigned int nTextures = 0;

    // Set up the textures used by this object
    Texture* baseTex = nullptr;
    Texture* bumpTex = nullptr;
    Texture* specTex = nullptr;
    Texture* emissiveTex = nullptr;

    shaderProps.texUsage = ShaderProperties::SharedTextureCoords;

    if (useNormals)
    {
        if (lunarLambert == 0.0f)
            shaderProps.lightModel = ShaderProperties::DiffuseModel;
        else
            shaderProps.lightModel = ShaderProperties::LunarLambertModel;
    }
    else
    {
        // "particle" lighting is the only type that doesn't
        // depend on having a surface normal.
        // Enable alternate particle model when vertex colors are present;
        // eventually, a render context method will enable the particle
        // model.
        if (useColors)
            shaderProps.lightModel = ShaderProperties::ParticleModel;
        else
            shaderProps.lightModel = ShaderProperties::ParticleDiffuseModel;
    }

    ResourceHandle diffuseMap  = GetTextureHandle(m.maps[Material::DiffuseMap]);
    ResourceHandle normalMap   = GetTextureHandle(m.maps[Material::NormalMap]);
    ResourceHandle specularMap = GetTextureHandle(m.maps[Material::SpecularMap]);
    ResourceHandle emissiveMap = GetTextureHandle(m.maps[Material::EmissiveMap]);

    if (diffuseMap != InvalidResource && (useTexCoords || usePointSize))
    {
        baseTex = GetTextureManager()->find(diffuseMap);
        if (baseTex != nullptr)
        {
            shaderProps.texUsage |= ShaderProperties::DiffuseTexture;
            textures[nTextures++] = baseTex;
        }
    }

    if (normalMap != InvalidResource)
    {
        bumpTex = GetTextureManager()->find(normalMap);
        if (bumpTex != nullptr)
        {
            shaderProps.texUsage |= ShaderProperties::NormalTexture;
            if (bumpTex->getFormatOptions() & Texture::DXT5NormalMap)
            {
                shaderProps.texUsage |= ShaderProperties::CompressedNormalTexture;
            }
            textures[nTextures++] = bumpTex;
        }
    }

    if (m.specular != Material::Color(0.0f, 0.0f, 0.0f) && useNormals)
    {
        shaderProps.lightModel = ShaderProperties::PerPixelSpecularModel;
        specTex = GetTextureManager()->find(specularMap);
        if (specTex == nullptr)
        {
            if (baseTex != nullptr)
                shaderProps.texUsage |= ShaderProperties::SpecularInDiffuseAlpha;
        }
        else
        {
            shaderProps.texUsage |= ShaderProperties::SpecularTexture;
            textures[nTextures++] = specTex;
        }
    }

    if (emissiveMap != InvalidResource)
    {
        emissiveTex = GetTextureManager()->find(emissiveMap);
        if (emissiveTex != nullptr)
        {
            shaderProps.texUsage |= ShaderProperties::EmissiveTexture;
            textures[nTextures++] = emissiveTex;
        }
    }

    if (lightingState.shadowingRingSystem)
    {
        Texture* ringsTex = lightingState.shadowingRingSystem->texture.find(medres);
        if (ringsTex != nullptr)
        {
            glActiveTexture(GL_TEXTURE0 + nTextures);
            ringsTex->bind();
            textures[nTextures++] = ringsTex;

            // Tweak the texture--set clamp to border and a border color with
            // a zero alpha.
            float bc[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
#ifndef GL_ES
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, bc);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
#else
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR_OES, bc);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER_OES);
#endif
            glActiveTexture(GL_TEXTURE0);

            shaderProps.texUsage |= ShaderProperties::RingShadowTexture;
            for (unsigned int lightIndex = 0; lightIndex < lightingState.nLights; lightIndex++)
            {
                if (lightingState.lights[lightIndex].castsShadows &&
                    lightingState.shadowingRingSystem == lightingState.ringShadows[lightIndex].ringSystem)
                {
                    shaderProps.setRingShadowForLight(lightIndex, true);
                }
                else
                {
                    shaderProps.setRingShadowForLight(lightIndex, false);
                }
            }
        }
    }

    if (usePointSize)
        shaderProps.texUsage |= ShaderProperties::PointSprite;
    if (useColors)
        shaderProps.texUsage |= ShaderProperties::VertexColors;

    if (atmosphere != nullptr)
    {
        // Only use new atmosphere code in OpenGL 2.0 path when new style parameters are defined.
        if (atmosphere->mieScaleHeight > 0.0f)
            shaderProps.texUsage |= ShaderProperties::Scattering;
    }

    bool hasShadowMap = shadowMap != 0 && shadowMapWidth != 0 && lightMatrix != nullptr;
    if (hasShadowMap)
        shaderProps.texUsage |= ShaderProperties::ShadowMapTexture;

    // Get a shader for the current rendering configuration
    assert(renderer != nullptr);
    CelestiaGLProgram* prog = renderer->getShaderManager().getShader(shaderProps);
    if (prog == nullptr)
        return;

    prog->use();
    prog->MVPMatrix = (*projectionMatrix) * (*modelViewMatrix);

    for (unsigned int i = 0; i < nTextures; i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        textures[i]->bind();
    }

    if (hasShadowMap)
    {
        glActiveTexture(GL_TEXTURE0 + nTextures);
        glBindTexture(GL_TEXTURE_2D, shadowMap);
#if GL_ONLY_SHADOWS
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
#endif
        Matrix4f shadowBias(Matrix4f::Zero());
        shadowBias.diagonal() = Vector4f(0.5f, 0.5f, 0.5f, 1.0f);
        shadowBias.col(3) = Vector4f(0.5f, 0.5f, 0.5f, 1.0f);
        prog->ShadowMatrix0 = shadowBias * (*lightMatrix);
        prog->floatParam("shadowMapSize") = static_cast<float>(shadowMapWidth);
    }

    // setLightParameters() expects opacity in the alpha channel of the diffuse color
#ifdef HDR_COMPRESS
    Color diffuse(m.diffuse.red() * 0.5f, m.diffuse.green() * 0.5f, m.diffuse.blue() * 0.5f, m.opacity);
#else
    Color diffuse(m.diffuse.red(), m.diffuse.green(), m.diffuse.blue(), m.opacity);
#endif
    Color specular(m.specular.red(), m.specular.green(), m.specular.blue());
    Color emissive(m.emissive.red(), m.emissive.green(), m.emissive.blue());

    prog->setLightParameters(lightingState, diffuse, specular, emissive);

    if (shaderProps.hasEclipseShadows() != 0)
        prog->setEclipseShadowParameters(lightingState, objScale, objOrientation);

    // TODO: handle emissive color
    prog->shininess = m.specularPower;
    if (shaderProps.lightModel == ShaderProperties::LunarLambertModel)
    {
        prog->lunarLambert = lunarLambert;
    }

    // Generally, we want to disable depth writes for blend because it
    // makes translucent objects look a bit better (though there are
    // still problems when rendering them without sorting.) However,
    // when scattering atmospheres are enabled, we need to render with
    // depth writes on, otherwise the atmosphere will be drawn over
    // a planet mesh. See SourceForge bug #1855894 for more details.
    bool disableDepthWriteOnBlend = true;

    if (atmosphere != nullptr && shaderProps.hasScattering())
    {
        prog->setAtmosphereParameters(*atmosphere, objRadius, objRadius);
        disableDepthWriteOnBlend = false;
    }

    if ((shaderProps.texUsage & ShaderProperties::PointSprite) != 0)
    {
        prog->pointScale = getPointScale();
    }

    // Ring shadow parameters
    if ((shaderProps.texUsage & ShaderProperties::RingShadowTexture) != 0)
    {
        const RingSystem* rings = lightingState.shadowingRingSystem;
        float ringWidth = rings->outerRadius - rings->innerRadius;
        prog->ringRadius = rings->innerRadius / objRadius;
        prog->ringWidth = objRadius / ringWidth;
        prog->ringPlane = Hyperplane<float, 3>(lightingState.ringPlaneNormal, lightingState.ringCenter / objRadius).coeffs();
        prog->ringCenter = lightingState.ringCenter / objRadius;

        for (unsigned int lightIndex = 0; lightIndex < lightingState.nLights; ++lightIndex)
        {
            if (shaderProps.hasRingShadowForLight(lightIndex))
            {
                prog->ringShadowLOD[lightIndex] = lightingState.ringShadows[lightIndex].texLod;
            }
        }
    }

    Material::BlendMode newBlendMode = Material::InvalidBlend;
    if (m.opacity != 1.0f ||
        m.blend == Material::AdditiveBlend ||
        (baseTex != nullptr && baseTex->hasAlpha()))
    {
        newBlendMode = m.blend;
    }

    if (newBlendMode != blendMode)
    {
        blendMode = newBlendMode;
        switch (blendMode)
        {
        case Material::NormalBlend:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(disableDepthWriteOnBlend ? GL_FALSE : GL_TRUE);
            break;
        case Material::AdditiveBlend:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDepthMask(disableDepthWriteOnBlend ? GL_FALSE : GL_TRUE);
            break;
        case Material::PremultipliedAlphaBlend:
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(disableDepthWriteOnBlend ? GL_FALSE : GL_TRUE);
            break;
        default:
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
            break;
        }
    }
}


void
GLSL_RenderContext::setAtmosphere(const Atmosphere* _atmosphere)
{
    atmosphere = _atmosphere;
}

// Extended material properties -- currently just lunarLambert term
void
GLSL_RenderContext::setLunarLambert(float l)
{
    lunarLambert = l;
}

void
GLSL_RenderContext::setShadowMap(GLuint _shadowMap, GLuint _width, const Eigen::Matrix4f *_lightMatrix)
{
    shadowMap      = _shadowMap;
    shadowMapWidth = _width;
    lightMatrix    = _lightMatrix;
}

/***** GLSL-Unlit render context ******/

GLSLUnlit_RenderContext::GLSLUnlit_RenderContext(const Renderer* renderer, float _objRadius) :
    RenderContext(renderer),
    blendMode(Material::InvalidBlend),
    objRadius(_objRadius)
{
    initLightingEnvironment();
}


GLSLUnlit_RenderContext::~GLSLUnlit_RenderContext()
{
    glDisableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glDisableVertexAttribArray(CelestiaGLProgram::NormalAttributeIndex);
    glDisableVertexAttribArray(CelestiaGLProgram::ColorAttributeIndex);
    glDisableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex);
    glDisableVertexAttribArray(CelestiaGLProgram::TangentAttributeIndex);
    glDisableVertexAttribArray(CelestiaGLProgram::PointSizeAttributeIndex);
}


void
GLSLUnlit_RenderContext::initLightingEnvironment()
{
    // Set the light and shadow environment, which is constant for the entire model.
    // The material properties will be set per mesh.
    shaderProps.nLights = 1;
}


void
GLSLUnlit_RenderContext::makeCurrent(const Material& m)
{
    Texture* textures[4] = { nullptr, nullptr, nullptr, nullptr };
    unsigned int nTextures = 0;

    // Set up the textures used by this object
    Texture* baseTex = nullptr;

    shaderProps.lightModel = ShaderProperties::EmissiveModel;
    shaderProps.texUsage = ShaderProperties::SharedTextureCoords;

    ResourceHandle diffuseMap = GetTextureHandle(m.maps[Material::DiffuseMap]);
    if (diffuseMap != InvalidResource && (useTexCoords || usePointSize))
    {
        baseTex = GetTextureManager()->find(diffuseMap);
        if (baseTex != nullptr)
        {
            shaderProps.texUsage |= ShaderProperties::DiffuseTexture;
            textures[nTextures++] = baseTex;
        }
    }

    if (usePointSize)
        shaderProps.texUsage |= ShaderProperties::PointSprite;
    if (useColors)
        shaderProps.texUsage |= ShaderProperties::VertexColors;

    // Get a shader for the current rendering configuration
    assert(renderer != nullptr);
    CelestiaGLProgram* prog = renderer->getShaderManager().getShader(shaderProps);
    if (prog == nullptr)
        return;

    prog->use();
    prog->MVPMatrix = (*projectionMatrix) * (*modelViewMatrix);
    if (usePointSize)
        prog->ModelViewMatrix = *modelViewMatrix;

    for (unsigned int i = 0; i < nTextures; i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        textures[i]->bind();
    }

#ifdef HDR_COMPRESS
    prog->lights[0].diffuse = m.diffuse.toVector3() * 0.5f;
#else
    prog->lights[0].diffuse = m.diffuse.toVector3();
#endif
    prog->opacity = m.opacity;

    if ((shaderProps.texUsage & ShaderProperties::PointSprite) != 0)
    {
        prog->pointScale = getPointScale();
    }

    Material::BlendMode newBlendMode = Material::InvalidBlend;
    if (m.opacity != 1.0f ||
        m.blend == Material::AdditiveBlend ||
        (baseTex != nullptr && baseTex->hasAlpha()))
    {
        newBlendMode = m.blend;
    }

    if (newBlendMode != blendMode)
    {
        blendMode = newBlendMode;
        switch (blendMode)
        {
        case Material::NormalBlend:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            break;
        case Material::AdditiveBlend:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDepthMask(GL_FALSE);
            break;
        case Material::PremultipliedAlphaBlend:
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            break;
        default:
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
            break;
        }
    }
}
