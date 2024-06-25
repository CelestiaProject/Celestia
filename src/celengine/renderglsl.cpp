// renderglsl.cpp
//
// Functions for rendering objects using dynamically generated GLSL shaders.
//
// Copyright (C) 2006-2020, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>

#include <boost/container/static_vector.hpp>

#include <celcompat/numbers.h>
#include <celmath/geomutil.h>
#include <celmath/mathlib.h>
#include <celmodel/material.h>
#include <celrender/gl/buffer.h>
#include <celrender/gl/vertexobject.h>
#include <celutil/color.h>
#include <celutil/flag.h>
#include "atmosphere.h"
#include "body.h"
#include "framebuffer.h"
#include "geometry.h"
#include "glsupport.h"
#include "lodspheremesh.h"
#include "rendcontext.h"
#include "render.h"
#include "renderglsl.h"
#include "renderinfo.h"
#include "shadermanager.h"
#include "shadowmap.h" // GL_ONLY_SHADOWS definition
#include "texture.h"

using namespace celestia;

namespace
{

// Calculate the matrix used to render the model from the
// perspective of the light.
Eigen::Matrix4f directionalLightMatrix(const Eigen::Vector3f& lightDirection)
{
    const Eigen::Vector3f &viewDir = lightDirection;
    Eigen::Vector3f upDir = viewDir.unitOrthogonal();
    Eigen::Vector3f rightDir = upDir.cross(viewDir);
    Eigen::Matrix4f m = Eigen::Matrix4f::Identity();

    m.row(0).head(3) = rightDir;
    m.row(1).head(3) = upDir;
    m.row(2).head(3) = viewDir;

    return m;
}


/*! Render a mesh object
 *  Parameters:
 *    tsec : animation clock time in seconds
 */
void renderGeometryShadow_GLSL(Geometry* geometry,
                               FramebufferObject* shadowFbo,
                               const LightingState& ls,
                               int lightIndex,
                               double tsec,
                               Renderer* renderer,
                               Eigen::Matrix4f *lightMatrix)
{
    auto *prog = renderer->getShaderManager().getShader("depth");
    if (prog == nullptr)
        return;

    GLint oldFboId;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFboId);
    shadowFbo->bind();
    glViewport(0, 0, shadowFbo->width(), shadowFbo->height());

    // Write only to the depth buffer
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glClear(GL_DEPTH_BUFFER_BIT);
    // Render backfaces only in order to reduce self-shadowing artifacts
    glCullFace(GL_FRONT);

    Renderer::PipelineState ps;
    ps.depthMask = true;
    ps.depthTest = true;
    renderer->setPipelineState(ps);

    Shadow_RenderContext rc(renderer);

    prog->use();

    // Enable poligon offset to decrease "shadow acne"
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(.001f, .001f);

    Eigen::Matrix4f projMat = math::Ortho(-1.f, 1.f, -1.f, 1.f, -1.f, 1.f);
    Eigen::Matrix4f modelViewMat = directionalLightMatrix(ls.lights[lightIndex].direction_obj);
    *lightMatrix = projMat * modelViewMat;
    prog->setMVPMatrices(projMat, modelViewMat);
    geometry->render(rc, tsec);

    glDisable(GL_POLYGON_OFFSET_FILL);
    // Re-enable the color buffer
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glCullFace(GL_BACK);
    shadowFbo->unbind(oldFboId);
}

} // end unnamed namespace

// Render a planet sphere with GLSL shaders
void renderEllipsoid_GLSL(const RenderInfo& ri,
                          const LightingState& ls,
                          Atmosphere* atmosphere,
                          float cloudTexOffset,
                          const Eigen::Vector3f& semiAxes,
                          unsigned int textureRes,
                          std::uint64_t renderFlags,
                          const Eigen::Quaternionf& planetOrientation,
                          const math::Frustum& frustum,
                          const Matrices &m,
                          Renderer* renderer)
{
    float radius = semiAxes.maxCoeff();

    boost::container::static_vector<Texture*, LODSphereMesh::MAX_SPHERE_MESH_TEXTURES> textures;

    ShaderProperties shadprop;
    shadprop.texUsage = TexUsage::TextureCoordTransform;
    shadprop.nLights = std::min(ls.nLights, MaxShaderLights);

    // Set up the textures used by this object
    if (ri.baseTex != nullptr)
    {
        shadprop.texUsage |= TexUsage::DiffuseTexture;
        textures.push_back(ri.baseTex);
    }

    if (ri.bumpTex != nullptr)
    {
        shadprop.texUsage |= TexUsage::NormalTexture;
        textures.push_back(ri.bumpTex);
        if (ri.bumpTex->getFormatOptions() & Texture::DXT5NormalMap)
            shadprop.texUsage |= TexUsage::CompressedNormalTexture;
    }

    if (ri.specularColor != Color::Black)
    {
        shadprop.lightModel = LightingModel::PerPixelSpecularModel;
        if (ri.glossTex == nullptr)
        {
            shadprop.texUsage |= TexUsage::SpecularInDiffuseAlpha;
        }
        else
        {
            shadprop.texUsage |= TexUsage::SpecularTexture;
            textures.push_back(ri.glossTex);
        }
    }
    if (ri.lunarLambert != 0.0f)
    {
        shadprop.lightModel |= LightingModel::LunarLambertModel;
    }

    if (ri.nightTex != nullptr)
    {
        shadprop.texUsage |= TexUsage::NightTexture;
        textures.push_back(ri.nightTex);
    }

    if (ri.overlayTex != nullptr)
    {
        shadprop.texUsage |= TexUsage::OverlayTexture;
        textures.push_back(ri.overlayTex);
    }

    if (atmosphere != nullptr)
    {
        if ((renderFlags & Renderer::ShowAtmospheres) != 0)
        {
            // Only use new atmosphere code in OpenGL 2.0 path when new style parameters are defined.
            // ... but don't show atmospheres when there are no light sources.
            if (atmosphere->mieScaleHeight > 0.0f && shadprop.nLights > 0)
                shadprop.texUsage |= TexUsage::Scattering;
        }

        if ((renderFlags & Renderer::ShowCloudMaps) != 0 &&
            (renderFlags & Renderer::ShowCloudShadows) != 0)
        {
            Texture* cloudTex = nullptr;
            if (atmosphere->cloudTexture.tex[textureRes] != InvalidResource)
                cloudTex = atmosphere->cloudTexture.find(textureRes);

            // The current implementation of cloud shadows is not compatible
            // with virtual or split textures.
            bool allowCloudShadows = std::none_of(textures.cbegin(), textures.cend(),
                                                  [](const Texture* tex) { return tex != nullptr &&
                                                                                  (tex->getLODCount() > 1 ||
                                                                                   tex->getUTileCount(0) > 1 ||
                                                                                   tex->getVTileCount(0) > 1); });

            // Split cloud shadows can't cast shadows
            if (cloudTex != nullptr)
            {
                if (cloudTex->getLODCount() > 1 ||
                    cloudTex->getUTileCount(0) > 1 ||
                    cloudTex->getVTileCount(0) > 1)
                {
                    allowCloudShadows = false;
                }
            }

            if (cloudTex != nullptr && allowCloudShadows && atmosphere->cloudShadowDepth > 0.0f)
            {
                shadprop.texUsage |= TexUsage::CloudShadowTexture;
                textures.push_back(cloudTex);
                glActiveTexture(GL_TEXTURE0 + textures.size());
                cloudTex->bind();
                glActiveTexture(GL_TEXTURE0);

                for (unsigned int lightIndex = 0; lightIndex < ls.nLights; lightIndex++)
                {
                    if (ls.lights[lightIndex].castsShadows)
                    {
                        shadprop.setCloudShadowForLight(lightIndex, true);
                    }
                }

            }
        }
    }

    // Set the shadow information.
    for (unsigned int li = 0; li < ls.nLights; li++)
    {
        if (ls.shadows[li] && !ls.shadows[li]->empty())
        {
            auto nShadows = std::min(MaxShaderEclipseShadows, static_cast<unsigned int>(ls.shadows[li]->size()));
            shadprop.setEclipseShadowCountForLight(li, nShadows);
        }
    }

    if (ls.shadowingRingSystem)
    {
        Texture* ringsTex = ls.shadowingRingSystem->texture.find(textureRes);
        if (ringsTex != nullptr)
        {
            glActiveTexture(GL_TEXTURE0 + textures.size());
            ringsTex->bind();

#ifdef GL_ES
            if (gl::OES_texture_border_clamp)
#endif
            {
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
            }
            glActiveTexture(GL_TEXTURE0);

            shadprop.texUsage |= TexUsage::RingShadowTexture;

            for (unsigned int lightIndex = 0; lightIndex < ls.nLights; lightIndex++)
            {
                if (ls.lights[lightIndex].castsShadows &&
                    ls.shadowingRingSystem == ls.ringShadows[lightIndex].ringSystem)
                {
                    shadprop.setRingShadowForLight(lightIndex, true);
                }
            }
        }
    }


    // Get a shader for the current rendering configuration
    CelestiaGLProgram* prog = renderer->getShaderManager().getShader(shadprop);
    if (prog == nullptr)
        return;

    prog->use();
    prog->setMVPMatrices(*m.projection, *m.modelview);

    prog->setLightParameters(ls, ri.color, ri.specularColor, Color::Black);

    prog->eyePosition = ls.eyePos_obj;
    prog->shininess = ri.specularPower;
    if (util::is_set(shadprop.lightModel, LightingModel::LunarLambertModel))
        prog->lunarLambert = ri.lunarLambert;

    if (util::is_set(shadprop.texUsage, TexUsage::RingShadowTexture))
    {
        float ringWidth = ls.shadowingRingSystem->outerRadius - ls.shadowingRingSystem->innerRadius;
        prog->ringRadius = ls.shadowingRingSystem->innerRadius / radius;
        prog->ringWidth = radius / ringWidth;
        prog->ringPlane = Eigen::Hyperplane<float, 3>(ls.ringPlaneNormal, ls.ringCenter / radius).coeffs();
        prog->ringCenter = ls.ringCenter / radius;
        for (unsigned int lightIndex = 0; lightIndex < ls.nLights; ++lightIndex)
        {
            if (shadprop.hasRingShadowForLight(lightIndex))
            {
                prog->ringShadowLOD[lightIndex] = ls.ringShadows[lightIndex].texLod;
            }
        }
    }

    if (atmosphere != nullptr)
    {
        if (util::is_set(shadprop.texUsage, TexUsage::CloudShadowTexture))
        {
            prog->shadowTextureOffset = cloudTexOffset;
            prog->cloudHeight = 1.0f + atmosphere->cloudHeight / radius;
        }

        if (shadprop.hasScattering())
        {
            prog->setAtmosphereParameters(*atmosphere, radius, radius);
        }
    }

    if (shadprop.hasEclipseShadows())
        prog->setEclipseShadowParameters(ls, semiAxes, planetOrientation);

    unsigned int attributes = LODSphereMesh::Normals;
    if (ri.bumpTex != nullptr)
        attributes |= LODSphereMesh::Tangents;

    Renderer::PipelineState ps;
    ps.depthMask = true;
    ps.depthTest = true;
    renderer->setPipelineState(ps);

    auto endTextures = std::remove(textures.begin(), textures.end(), nullptr);
    textures.erase(endTextures, textures.end());
    g_lodSphere->render(attributes,
                        frustum, ri.pixWidth,
                        textures.data(), static_cast<int>(textures.size()), prog);
}


#undef DEPTH_BUFFER_DEBUG

/*! Render a mesh object
 *  Parameters:
 *    tsec : animation clock time in seconds
 */
void renderGeometry_GLSL(Geometry* geometry,
                         const RenderInfo& ri,
                         ResourceHandle texOverride,
                         const LightingState& ls,
                         const Atmosphere* atmosphere,
                         float geometryScale,
                         std::uint64_t renderFlags,
                         const Eigen::Quaternionf& planetOrientation,
                         double tsec,
                         const Matrices &m,
                         Renderer* renderer)
{
    auto *shadowBuffer = renderer->getShadowFBO(0);
    Eigen::Matrix4f lightMatrix(Eigen::Matrix4f::Identity());

    if (shadowBuffer != nullptr && shadowBuffer->isValid())
    {
        std::array<int, 4> viewport;
        renderer->getViewport(viewport);

        float range[2];
        glGetFloatv(GL_DEPTH_RANGE, range);
        glDepthRange(0.0f, 1.0f);

#ifdef DEPTH_STATE_DEBUG
        float bias, bits, clear, range[2], scale;
        glGetFloatv(GL_DEPTH_BIAS, &bias);
        glGetFloatv(GL_DEPTH_BITS, &bits);
        glGetFloatv(GL_DEPTH_CLEAR_VALUE, &clear);
        glGetFloatv(GL_DEPTH_RANGE, range);
        glGetFloatv(GL_DEPTH_SCALE, &scale);
        fmt::printf("bias: %f bits: %f clear: %f range: %f - %f, scale:%f\n", bias, bits, clear, range[0], range[1], scale);
#endif

        renderGeometryShadow_GLSL(geometry, shadowBuffer, ls, 0,
                                  tsec, renderer, &lightMatrix);
        renderer->setViewport(viewport);
#ifdef DEPTH_BUFFER_DEBUG
        glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadMatrixf(Ortho2D(0.0f, (float)viewport[2], 0.0f, (float)viewport[3]).data());
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glUseProgram(0);
        glColor4f(1, 1, 1, 1);

        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, shadowBuffer->depthTexture());
#if GL_ONLY_SHADOWS
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
#endif

        glBegin(GL_QUADS);
        float side = 300.0f;
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(side, 0.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(side, side);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(0.0f, side);
        glEnd();

        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_DEPTH_TEST);
#endif
        glDepthRange(range[0], range[1]);
    }

    GLSL_RenderContext rc(renderer, ls, geometryScale, planetOrientation, m.modelview, m.projection);

    if ((renderFlags & Renderer::ShowAtmospheres) != 0)
    {
        rc.setAtmosphere(atmosphere);
    }

    if (shadowBuffer != nullptr && shadowBuffer->isValid())
    {
        rc.setShadowMap(shadowBuffer->depthTexture(), shadowBuffer->width(), &lightMatrix);
    }

    rc.setCameraOrientation(ri.orientation);
    rc.setPointScale(ri.pointScale);

    // Handle extended material attributes (per model only, not per submesh)
    rc.setLunarLambert(ri.lunarLambert);

    Renderer::PipelineState ps;
    ps.depthMask = true;
    ps.depthTest = true;
    renderer->setPipelineState(ps);

    // Handle material override; a texture specified in an ssc file will
    // override all materials specified in the geometry file.
    if (texOverride != InvalidResource)
    {
        cmod::Material m;
        m.diffuse = cmod::Color(ri.color);
        m.specular = cmod::Color(ri.specularColor);
        m.specularPower = ri.specularPower;

        m.setMap(cmod::TextureSemantic::DiffuseMap, texOverride);
        rc.setMaterial(&m);
        rc.lock();
        geometry->render(rc, tsec);
    }
    else
    {
        geometry->render(rc, tsec);
    }
}


/*! Render a mesh object without lighting.
 *  Parameters:
 *    tsec : animation clock time in seconds
 */
void renderGeometry_GLSL_Unlit(Geometry* geometry,
                               const RenderInfo& ri,
                               ResourceHandle texOverride,
                               float geometryScale,
                               std::uint64_t /* renderFlags */,
                               const Eigen::Quaternionf& /* planetOrientation */,
                               double tsec,
                               const Matrices &m,
                               Renderer* renderer)
{
    GLSLUnlit_RenderContext rc(renderer, geometryScale, m.modelview, m.projection);
    rc.setPointScale(ri.pointScale);

    Renderer::PipelineState ps;
    ps.depthMask = true;
    ps.depthTest = true;
    renderer->setPipelineState(ps);

    // Handle material override; a texture specified in an ssc file will
    // override all materials specified in the model file.
    if (texOverride != InvalidResource)
    {
        cmod::Material m;
        m.diffuse = cmod::Color(ri.color);
        m.specular = cmod::Color(ri.specularColor);
        m.specularPower = ri.specularPower;

        m.setMap(cmod::TextureSemantic::DiffuseMap, texOverride);
        rc.setMaterial(&m);
        rc.lock();
        geometry->render(rc, tsec);
    }
    else
    {
        geometry->render(rc, tsec);
    }
}


// Render the cloud sphere for a world a cloud layer defined
void renderClouds_GLSL(const RenderInfo& ri,
                       const LightingState& ls,
                       Atmosphere* atmosphere,
                       Texture* cloudTex,
                       Texture* cloudNormalMap,
                       float texOffset,
                       const Eigen::Vector3f& semiAxes,
                       unsigned int /*textureRes*/,
                       std::uint64_t renderFlags,
                       const Eigen::Quaternionf& planetOrientation,
                       const math::Frustum& frustum,
                       const Matrices &m,
                       Renderer* renderer)
{
    float radius = semiAxes.maxCoeff();

    boost::container::static_vector<Texture*, LODSphereMesh::MAX_SPHERE_MESH_TEXTURES> textures;

    ShaderProperties shadprop;
    shadprop.texUsage = TexUsage::TextureCoordTransform;
    shadprop.nLights = ls.nLights;

    // Set up the textures used by this object
    if (cloudTex != nullptr)
    {
        shadprop.texUsage |= TexUsage::DiffuseTexture;
        textures.push_back(cloudTex);
    }

    if (cloudNormalMap != nullptr)
    {
        shadprop.texUsage |= TexUsage::NormalTexture;
        textures.push_back(cloudNormalMap);
        if (cloudNormalMap->getFormatOptions() & Texture::DXT5NormalMap)
            shadprop.texUsage |= TexUsage::CompressedNormalTexture;
    }

    if (atmosphere != nullptr)
    {
        if ((renderFlags & Renderer::ShowAtmospheres) != 0)
        {
            // Only use new atmosphere code in OpenGL 2.0 path when new style parameters are defined.
            // ... but don't show atmospheres when there are no light sources.
            if (atmosphere->mieScaleHeight > 0.0f && shadprop.nLights > 0)
                shadprop.texUsage |= TexUsage::Scattering;
        }
    }

    // Set the shadow information.
    for (unsigned int li = 0; li < ls.nLights; li++)
    {
        if (ls.shadows[li] && !ls.shadows[li]->empty())
        {
            unsigned int nShadows = std::min(MaxShaderEclipseShadows,
                                             static_cast<unsigned int>(ls.shadows[li]->size()));
            shadprop.setEclipseShadowCountForLight(li, nShadows);
        }
    }

    // Get a shader for the current rendering configuration
    CelestiaGLProgram* prog = renderer->getShaderManager().getShader(shadprop);
    if (prog == nullptr)
        return;

    prog->use();
    prog->setMVPMatrices(*m.projection, *m.modelview);

    prog->setLightParameters(ls, ri.color, ri.specularColor, Color::Black);
    prog->eyePosition = ls.eyePos_obj;
    prog->ambientColor = ri.ambientColor.toVector3();
    prog->textureOffset = texOffset;

    if (atmosphere != nullptr)
    {
        float cloudRadius = radius + atmosphere->cloudHeight;

        if (shadprop.hasScattering())
        {
            prog->setAtmosphereParameters(*atmosphere, radius, cloudRadius);
        }
    }

#if 0
    if (shadprop.texUsage & ShaderProperties::RingShadowTexture)
    {
        float ringWidth = rings->outerRadius - rings->innerRadius;
        prog->ringRadius = rings->innerRadius / cloudRadius;
        prog->ringWidth = 1.0f / (ringWidth / cloudRadius);
    }
#endif

    if (shadprop.shadowCounts != 0)
        prog->setEclipseShadowParameters(ls, semiAxes, planetOrientation);

    unsigned int attributes = LODSphereMesh::Normals;
    if (cloudNormalMap != nullptr)
        attributes |= LODSphereMesh::Tangents;

    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    ps.depthTest = true;
    renderer->setPipelineState(ps);

    auto endTextures = std::remove(textures.begin(), textures.end(), nullptr);
    textures.erase(endTextures, textures.end());
    g_lodSphere->render(attributes,
                        frustum, ri.pixWidth,
                        textures.data(), static_cast<int>(textures.size()), prog);

    prog->textureOffset = 0.0f;
}
