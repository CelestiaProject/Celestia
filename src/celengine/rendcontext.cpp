// rendcontext.cpp
//
// Copyright (C) 2004-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <cstddef>

#include <celutil/color.h>
#include "body.h"
#include "lightenv.h"
#include "rendcontext.h"
#include "render.h"
#include "shadowmap.h" // GL_ONLY_SHADOWS definition
#include "texmanager.h"
#include "texture.h"


namespace
{

const cmod::Material defaultMaterial;

constexpr GLenum GLPrimitiveModes[static_cast<std::size_t>(cmod::PrimitiveGroupType::PrimitiveTypeMax)] =
{
    GL_TRIANGLES,
    GL_TRIANGLE_STRIP,
    GL_TRIANGLE_FAN,
    GL_LINES,
    GL_LINE_STRIP,
    GL_POINTS,
    GL_POINTS,
};

constexpr GLenum GLComponentTypes[static_cast<std::size_t>(cmod::VertexAttributeFormat::FormatMax)] =
{
     GL_FLOAT,          // Float1
     GL_FLOAT,          // Float2
     GL_FLOAT,          // Float3
     GL_FLOAT,          // Float4,
     GL_UNSIGNED_BYTE,  // UByte4
};

constexpr int GLComponentCounts[static_cast<std::size_t>(cmod::VertexAttributeFormat::FormatMax)] =
{
     1,  // Float1
     2,  // Float2
     3,  // Float3
     4,  // Float4,
     4,  // UByte4
};


void
setStandardVertexArrays(const cmod::VertexDescription& desc,
                        const cmod::VWord* vertexData)
{
    const cmod::VertexAttribute& position  = desc.getAttribute(cmod::VertexAttributeSemantic::Position);
    const cmod::VertexAttribute& normal    = desc.getAttribute(cmod::VertexAttributeSemantic::Normal);
    const cmod::VertexAttribute& color0    = desc.getAttribute(cmod::VertexAttributeSemantic::Color0);
    const cmod::VertexAttribute& texCoord0 = desc.getAttribute(cmod::VertexAttributeSemantic::Texture0);

    // Can't render anything unless we have positions
    if (position.format != cmod::VertexAttributeFormat::Float3)
        return;

    // Set up the vertex arrays
    glEnableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glVertexAttribPointer(CelestiaGLProgram::VertexCoordAttributeIndex,
                          3, GL_FLOAT, GL_FALSE, desc.strideBytes,
                          vertexData + position.offsetWords);

    // Set up the normal array
    switch (normal.format)
    {
    case cmod::VertexAttributeFormat::Float3:
        glEnableVertexAttribArray(CelestiaGLProgram::NormalAttributeIndex);
        glVertexAttribPointer(CelestiaGLProgram::NormalAttributeIndex,
                              3, GLComponentTypes[static_cast<std::size_t>(normal.format)],
                              GL_FALSE, desc.strideBytes,
                              vertexData + normal.offsetWords);
        break;
    default:
        glDisableVertexAttribArray(CelestiaGLProgram::NormalAttributeIndex);
        break;
    }

    GLint normalized = GL_TRUE;
    // Set up the color array
    switch (color0.format)
    {
    case cmod::VertexAttributeFormat::Float3:
    case cmod::VertexAttributeFormat::Float4:
        normalized = GL_FALSE;
    case cmod::VertexAttributeFormat::UByte4:
        glEnableVertexAttribArray(CelestiaGLProgram::ColorAttributeIndex);
        glVertexAttribPointer(CelestiaGLProgram::ColorAttributeIndex,
                              GLComponentCounts[static_cast<std::size_t>(color0.format)],
                              GLComponentTypes[static_cast<std::size_t>(color0.format)],
                              normalized, desc.strideBytes,
                              vertexData + color0.offsetWords);
        break;
    default:
        glDisableVertexAttribArray(CelestiaGLProgram::ColorAttributeIndex);
        break;
    }

    // Set up the texture coordinate array
    switch (texCoord0.format)
    {
    case cmod::VertexAttributeFormat::Float1:
    case cmod::VertexAttributeFormat::Float2:
    case cmod::VertexAttributeFormat::Float3:
    case cmod::VertexAttributeFormat::Float4:
        glEnableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex);
        glVertexAttribPointer(CelestiaGLProgram::TextureCoord0AttributeIndex,
                              GLComponentCounts[static_cast<std::size_t>(texCoord0.format)],
                              GLComponentTypes[static_cast<std::size_t>(texCoord0.format)],
                              GL_FALSE,
                              desc.strideBytes,
                              vertexData + texCoord0.offsetWords);
        break;
    default:
        glDisableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex);
        break;
    }
}


void
setExtendedVertexArrays(const cmod::VertexDescription& desc,
                        const cmod::VWord* vertexData)
{
    const cmod::VertexAttribute& tangent  = desc.getAttribute(cmod::VertexAttributeSemantic::Tangent);
    switch (tangent.format)
    {
    case cmod::VertexAttributeFormat::Float3:
        glEnableVertexAttribArray(CelestiaGLProgram::TangentAttributeIndex);
        glVertexAttribPointer(CelestiaGLProgram::TangentAttributeIndex,
                                      GLComponentCounts[static_cast<std::size_t>(tangent.format)],
                                      GLComponentTypes[static_cast<std::size_t>(tangent.format)],
                                      GL_FALSE,
                                      desc.strideBytes,
                                      vertexData + tangent.offsetWords);
        break;
    default:
        glDisableVertexAttribArray(CelestiaGLProgram::TangentAttributeIndex);
        break;
    }

    const cmod::VertexAttribute& pointsize = desc.getAttribute(cmod::VertexAttributeSemantic::PointSize);
    switch (pointsize.format)
    {
    case cmod::VertexAttributeFormat::Float1:
        glEnableVertexAttribArray(CelestiaGLProgram::PointSizeAttributeIndex);
        glVertexAttribPointer(CelestiaGLProgram::PointSizeAttributeIndex,
                                      GLComponentCounts[static_cast<std::size_t>(pointsize.format)],
                                      GLComponentTypes[static_cast<std::size_t>(pointsize.format)],
                                      GL_FALSE,
                                      desc.strideBytes,
                                      vertexData + pointsize.offsetWords);
        break;
    default:
        glDisableVertexAttribArray(CelestiaGLProgram::PointSizeAttributeIndex);
        break;
    }

    const cmod::VertexAttribute& nextPos = desc.getAttribute(cmod::VertexAttributeSemantic::NextPosition);
    switch (nextPos.format)
    {
    case cmod::VertexAttributeFormat::Float3:
        glEnableVertexAttribArray(CelestiaGLProgram::NextVCoordAttributeIndex);
        glVertexAttribPointer(CelestiaGLProgram::NextVCoordAttributeIndex,
                                      GLComponentCounts[static_cast<std::size_t>(nextPos.format)],
                                      GLComponentTypes[static_cast<std::size_t>(nextPos.format)],
                                      GL_FALSE,
                                      desc.strideBytes,
                                      vertexData + nextPos.offsetWords);
        break;
    default:
        glDisableVertexAttribArray(CelestiaGLProgram::NextVCoordAttributeIndex);
        break;
    }

    const cmod::VertexAttribute& scaleFac = desc.getAttribute(cmod::VertexAttributeSemantic::ScaleFactor);
    switch (scaleFac.format)
    {
    case cmod::VertexAttributeFormat::Float1:
        glEnableVertexAttribArray(CelestiaGLProgram::ScaleFactorAttributeIndex);
        glVertexAttribPointer(CelestiaGLProgram::ScaleFactorAttributeIndex,
                                      GLComponentCounts[static_cast<std::size_t>(scaleFac.format)],
                                      GLComponentTypes[static_cast<std::size_t>(scaleFac.format)],
                                      GL_FALSE,
                                      desc.strideBytes,
                                      vertexData + scaleFac.offsetWords);
        break;
    default:
        glDisableVertexAttribArray(CelestiaGLProgram::ScaleFactorAttributeIndex);
        break;
    }
}

} // end unnamed namespace




RenderContext::RenderContext(Renderer* _renderer) :
    material(&defaultMaterial),
    renderer(_renderer)
{
}


RenderContext::RenderContext(const cmod::Material* _material)
{
    if (_material == nullptr)
        material = &defaultMaterial;
    else
        material = _material;
}


const cmod::Material*
RenderContext::getMaterial() const
{
    return material;
}


void
RenderContext::setMaterial(const cmod::Material* newMaterial)
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
            if (material->getMap(cmod::TextureSemantic::EmissiveMap) !=
                newMaterial->getMap(cmod::TextureSemantic::EmissiveMap))
            {
                material = newMaterial;
                makeCurrent(*material);
            }
        }
    }
}


bool
RenderContext::shouldDrawLineAsTriangles() const
{
    return renderer->shouldDrawLineAsTriangles();
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
RenderContext::setCameraOrientation(const Eigen::Quaternionf& q)
{
    cameraOrientation = q;
}


Eigen::Quaternionf
RenderContext::getCameraOrientation() const
{
    return cameraOrientation;
}


void
RenderContext::drawGroup(const cmod::PrimitiveGroup& group, bool useOverride)
{
    // Skip rendering if this is the emissive pass but there's no
    // emissive texture.
    ResourceHandle emissiveMap = material->getMap(cmod::TextureSemantic::EmissiveMap);

    if (renderPass == EmissivePass && emissiveMap == InvalidResource)
    {
        return;
    }

    bool drawPoints = false;
    if (group.prim == cmod::PrimitiveGroupType::SpriteList || group.prim == cmod::PrimitiveGroupType::PointList)
    {
        drawPoints = true;
        if (group.prim == cmod::PrimitiveGroupType::PointList)
            glVertexAttrib1f(CelestiaGLProgram::PointSizeAttributeIndex, 1.0f);
#ifndef GL_ES
        glEnable(GL_POINT_SPRITE);
        glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif
        glActiveTexture(GL_TEXTURE0);
    }

    glDrawElements(GLPrimitiveModes[(int)(useOverride ? group.primOverride : group.prim)],
                   useOverride ? group.indicesOverride.size() :  group.indices.size(),
                   GL_UNSIGNED_INT,
                   useOverride ? group.indicesOverride.data() : group.indices.data());
#ifndef GL_ES
    if (drawPoints)
    {
        glDisable(GL_POINT_SPRITE);
        glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
    }
#endif
}


void
RenderContext::setVertexArrays(const cmod::VertexDescription& desc,
                               const cmod::VWord* vertexData)
{
    setStandardVertexArrays(desc, vertexData);
    setExtendedVertexArrays(desc, vertexData);
}

void
RenderContext::updateShader(const cmod::VertexDescription& desc, cmod::PrimitiveGroupType primType)
{
    // Normally, the shader that will be used depends only on the material.
    // But the presence of point size and normals can also affect the
    // shader, so force an update of the material if those attributes appear
    // or disappear in the new set of vertex arrays.
    bool usePointSizeNow = (desc.getAttribute(cmod::VertexAttributeSemantic::PointSize).format
                            == cmod::VertexAttributeFormat::Float1);
    bool useNormalsNow = (desc.getAttribute(cmod::VertexAttributeSemantic::Normal).format
                          == cmod::VertexAttributeFormat::Float3);
    bool useColorsNow = (desc.getAttribute(cmod::VertexAttributeSemantic::Color0).format
                         != cmod::VertexAttributeFormat::InvalidFormat);
    bool useTexCoordsNow = (desc.getAttribute(cmod::VertexAttributeSemantic::Texture0).format
                            != cmod::VertexAttributeFormat::InvalidFormat);
    bool drawLineNow = (desc.getAttribute(cmod::VertexAttributeSemantic::NextPosition).format
                        == cmod::VertexAttributeFormat::Float3);
    bool useStaticPointSizeNow = primType == cmod::PrimitiveGroupType::PointList;

    if (usePointSizeNow         != usePointSize       ||
        useStaticPointSizeNow   != useStaticPointSize ||
        useNormalsNow           != useNormals         ||
        useColorsNow            != useColors          ||
        useTexCoordsNow         != useTexCoords       ||
        drawLineNow             != drawLine)
    {
        usePointSize = usePointSizeNow;
        useStaticPointSize = useStaticPointSizeNow;
        useNormals = useNormalsNow;
        useColors = useColorsNow;
        useTexCoords = useTexCoordsNow;
        drawLine = drawLineNow;
        if (getMaterial() != nullptr)
            makeCurrent(*getMaterial());
    }
}


/***** GLSL render context ******/

GLSL_RenderContext::GLSL_RenderContext(Renderer* renderer,
                                       const LightingState& ls,
                                       float _objRadius,
                                       const Eigen::Quaternionf& orientation,
                                       const Eigen::Matrix4f* _modelViewMatrix,
                                       const Eigen::Matrix4f* _projectionMatrix) :
    RenderContext(renderer),
    lightingState(ls),
    objRadius(_objRadius),
    objScale(Eigen::Vector3f::Constant(_objRadius)),
    objOrientation(orientation),
    modelViewMatrix(_modelViewMatrix),
    projectionMatrix(_projectionMatrix)
{
    initLightingEnvironment();
}


GLSL_RenderContext::GLSL_RenderContext(Renderer* renderer,
                                       const LightingState& ls,
                                       const Eigen::Vector3f& _objScale,
                                       const Eigen::Quaternionf& orientation,
                                       const Eigen::Matrix4f* _modelViewMatrix,
                                       const Eigen::Matrix4f* _projectionMatrix) :
    RenderContext(renderer),
    lightingState(ls),
    objRadius(_objScale.maxCoeff()),
    objScale(_objScale),
    objOrientation(orientation),
    modelViewMatrix(_modelViewMatrix),
    projectionMatrix(_projectionMatrix)
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
    shaderProps.nLights = std::min(lightingState.nLights, MaxShaderLights);

    // Set the shadow information.
    // Track the total number of shadows; if there are too many, we'll have
    // to fall back to multipass.
    unsigned int totalShadows = 0;
    for (unsigned int li = 0; li < lightingState.nLights; li++)
    {
        if (lightingState.shadows[li] && !lightingState.shadows[li]->empty())
        {
            unsigned int nShadows = static_cast<unsigned int>(std::min(static_cast<std::size_t>(MaxShaderEclipseShadows),
                                                                       lightingState.shadows[li]->size()));
            shaderProps.setEclipseShadowCountForLight(li, nShadows);
            totalShadows += nShadows;
        }
    }

}

void
GLSL_RenderContext::makeCurrent(const cmod::Material& m)
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

    ResourceHandle diffuseMap  = m.getMap(cmod::TextureSemantic::DiffuseMap);
    ResourceHandle normalMap   = m.getMap(cmod::TextureSemantic::NormalMap);
    ResourceHandle specularMap = m.getMap(cmod::TextureSemantic::SpecularMap);
    ResourceHandle emissiveMap = m.getMap(cmod::TextureSemantic::EmissiveMap);

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

    if (m.specular != cmod::Color(0.0f, 0.0f, 0.0f) && useNormals)
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

#ifdef GL_ES
            if (celestia::gl::OES_texture_border_clamp)
            {
#endif
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

#ifdef GL_ES
            }
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
    else if (useStaticPointSize)
        shaderProps.texUsage |= ShaderProperties::StaticPointSize;

    if (drawLine)
        shaderProps.texUsage |= ShaderProperties::LineAsTriangles;

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
    prog->setMVPMatrices(*projectionMatrix, *modelViewMatrix);

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
        Eigen::Matrix4f shadowBias(Eigen::Matrix4f::Zero());
        shadowBias.diagonal() = Eigen::Vector4f(0.5f, 0.5f, 0.5f, 1.0f);
        shadowBias.col(3) = Eigen::Vector4f(0.5f, 0.5f, 0.5f, 1.0f);
        prog->ShadowMatrix0 = shadowBias * (*lightMatrix);
        prog->floatParam("shadowMapSize") = static_cast<float>(shadowMapWidth);
    }

    // setLightParameters() expects opacity in the alpha channel of the diffuse color
    Color diffuse(m.diffuse.red(), m.diffuse.green(), m.diffuse.blue(), m.opacity);
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

    if (usePointSize)
    {
        prog->pointScale = getPointScale();
    }
    else if (useStaticPointSize)
    {
        prog->pointScale = renderer->getScreenDpi() / 96.0f;
    }

    if (drawLine)
    {
        prog->lineWidthX = renderer->getLineWidthX();
        prog->lineWidthY = renderer->getLineWidthY();
    }

    // Ring shadow parameters
    if ((shaderProps.texUsage & ShaderProperties::RingShadowTexture) != 0)
    {
        const RingSystem* rings = lightingState.shadowingRingSystem;
        float ringWidth = rings->outerRadius - rings->innerRadius;
        prog->ringRadius = rings->innerRadius / objRadius;
        prog->ringWidth = objRadius / ringWidth;
        prog->ringPlane = Eigen::Hyperplane<float, 3>(lightingState.ringPlaneNormal, lightingState.ringCenter / objRadius).coeffs();
        prog->ringCenter = lightingState.ringCenter / objRadius;

        for (unsigned int lightIndex = 0; lightIndex < lightingState.nLights; ++lightIndex)
        {
            if (shaderProps.hasRingShadowForLight(lightIndex))
            {
                prog->ringShadowLOD[lightIndex] = lightingState.ringShadows[lightIndex].texLod;
            }
        }
    }

    cmod::BlendMode newBlendMode = cmod::BlendMode::InvalidBlend;
    if (m.opacity != 1.0f ||
        m.blend == cmod::BlendMode::AdditiveBlend ||
        (baseTex != nullptr && baseTex->hasAlpha()))
    {
        newBlendMode = m.blend;
    }

    if (newBlendMode != blendMode)
    {
        blendMode = newBlendMode;
        Renderer::PipelineState ps;
        switch (blendMode)
        {
        case cmod::BlendMode::NormalBlend:
            ps.blending = true;
            ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
            ps.depthMask = !disableDepthWriteOnBlend;
            break;
        case cmod::BlendMode::AdditiveBlend:
            ps.blending = true;
            ps.blendFunc = {GL_SRC_ALPHA, GL_ONE};
            ps.depthMask = !disableDepthWriteOnBlend;
            break;
        case cmod::BlendMode::PremultipliedAlphaBlend:
            ps.blending = true;
            ps.blendFunc = {GL_ONE, GL_ONE_MINUS_SRC_ALPHA};
            ps.depthMask = !disableDepthWriteOnBlend;
            break;
        default:
            ps.depthMask = true;
            break;
        }
        renderer->setPipelineState(ps);
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

GLSLUnlit_RenderContext::GLSLUnlit_RenderContext(Renderer* renderer,
                                                 float _objRadius,
                                                 const Eigen::Matrix4f* _modelViewMatrix,
                                                 const Eigen::Matrix4f* _projectionMatrix) :
    RenderContext(renderer),
    blendMode(cmod::BlendMode::InvalidBlend),
    objRadius(_objRadius),
    modelViewMatrix(_modelViewMatrix),
    projectionMatrix(_projectionMatrix)
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
GLSLUnlit_RenderContext::makeCurrent(const cmod::Material& m)
{
    Texture* textures[4] = { nullptr, nullptr, nullptr, nullptr };
    unsigned int nTextures = 0;

    // Set up the textures used by this object
    Texture* baseTex = nullptr;

    shaderProps.lightModel = ShaderProperties::EmissiveModel;
    shaderProps.texUsage = ShaderProperties::SharedTextureCoords;

    ResourceHandle diffuseMap = m.getMap(cmod::TextureSemantic::DiffuseMap);
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
    else if (useStaticPointSize)
        shaderProps.texUsage |= ShaderProperties::StaticPointSize;

    if (drawLine)
        shaderProps.texUsage |= ShaderProperties::LineAsTriangles;

    if (useColors)
        shaderProps.texUsage |= ShaderProperties::VertexColors;

    // Get a shader for the current rendering configuration
    assert(renderer != nullptr);
    CelestiaGLProgram* prog = renderer->getShaderManager().getShader(shaderProps);
    if (prog == nullptr)
        return;

    prog->use();
    prog->setMVPMatrices(*projectionMatrix, *modelViewMatrix);

    for (unsigned int i = 0; i < nTextures; i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        textures[i]->bind();
    }

    prog->lights[0].diffuse = m.diffuse.toVector3();
    prog->opacity = m.opacity;

    if (usePointSize)
    {
        prog->pointScale = getPointScale();
    }
    else if (useStaticPointSize)
    {
        prog->pointScale = renderer->getScreenDpi() / 96.0f;
    }

    if (drawLine)
    {
        prog->lineWidthX = renderer->getLineWidthX();
        prog->lineWidthY = renderer->getLineWidthY();
    }

    cmod::BlendMode newBlendMode = cmod::BlendMode::InvalidBlend;
    if (m.opacity != 1.0f ||
        m.blend == cmod::BlendMode::AdditiveBlend ||
        (baseTex != nullptr && baseTex->hasAlpha()))
    {
        newBlendMode = m.blend;
    }

    if (newBlendMode != blendMode)
    {
        blendMode = newBlendMode;
        Renderer::PipelineState ps;
        switch (blendMode)
        {
        case cmod::BlendMode::NormalBlend:
            ps.blending = true;
            ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
            break;
        case cmod::BlendMode::AdditiveBlend:
            ps.blending = true;
            ps.blendFunc = {GL_SRC_ALPHA, GL_ONE};
            break;
        case cmod::BlendMode::PremultipliedAlphaBlend:
            ps.blending = true;
            ps.blendFunc = {GL_ONE, GL_ONE_MINUS_SRC_ALPHA};
            break;
        default:
            ps.depthMask = true;
            break;
        }
        renderer->setPipelineState(ps);
    }
}
