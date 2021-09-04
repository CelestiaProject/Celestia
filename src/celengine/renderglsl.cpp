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

#include <config.h>
#include <algorithm>
#include <memory>
#include "framebuffer.h"
#include "geometry.h"
#include "lodspheremesh.h"
#include "meshmanager.h"
#include "modelgeometry.h"
#include "render.h"
#include "renderinfo.h"
#include "renderglsl.h"
#include "shadermanager.h"
#include "texmanager.h"
#include "vecgl.h"
#include <celengine/astro.h>
#include <celmath/frustum.h>
#include <celmath/distance.h>
#include <celmath/geomutil.h>
#include <celmath/intersect.h>
#include <celutil/utf8.h>
#include <celutil/util.h>
#include "shadowmap.h"

using namespace cmod;
using namespace Eigen;
using namespace std;
using namespace celmath;
using namespace celestia;

static
void renderGeometryShadow_GLSL(Geometry* geometry,
                               FramebufferObject* shadowFbo,
                               const LightingState& ls,
                               int lightIndex,
                               double tsec,
                               Renderer* renderer,
                               Matrix4f *lightMatrix);

static
Matrix4f directionalLightMatrix(const Vector3f& lightDirection);

// Render a planet sphere with GLSL shaders
void renderEllipsoid_GLSL(const RenderInfo& ri,
                       const LightingState& ls,
                       Atmosphere* atmosphere,
                       float cloudTexOffset,
                       const Vector3f& semiAxes,
                       unsigned int textureRes,
                       uint64_t renderFlags,
                       const Quaternionf& planetOrientation,
                       const Frustum& frustum,
                       const Matrices &m,
                       Renderer* renderer)
{
    float radius = semiAxes.maxCoeff();

    Texture* textures[MAX_SPHERE_MESH_TEXTURES] =
        { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
    unsigned int nTextures = 0;

    ShaderProperties shadprop;
    shadprop.nLights = min(ls.nLights, MaxShaderLights);

    // Set up the textures used by this object
    if (ri.baseTex != nullptr)
    {
        shadprop.texUsage = ShaderProperties::DiffuseTexture;
        textures[nTextures++] = ri.baseTex;
    }

    if (ri.bumpTex != nullptr)
    {
        shadprop.texUsage |= ShaderProperties::NormalTexture;
        textures[nTextures++] = ri.bumpTex;
        if (ri.bumpTex->getFormatOptions() & Texture::DXT5NormalMap)
            shadprop.texUsage |= ShaderProperties::CompressedNormalTexture;
    }

    if (ri.specularColor != Color::Black)
    {
        shadprop.lightModel = ShaderProperties::PerPixelSpecularModel;
        if (ri.glossTex == nullptr)
        {
            shadprop.texUsage |= ShaderProperties::SpecularInDiffuseAlpha;
        }
        else
        {
            shadprop.texUsage |= ShaderProperties::SpecularTexture;
            textures[nTextures++] = ri.glossTex;
        }
    }
    if (ri.lunarLambert != 0.0f)
    {
        shadprop.lightModel |= ShaderProperties::LunarLambertModel;
    }

    if (ri.nightTex != nullptr)
    {
        shadprop.texUsage |= ShaderProperties::NightTexture;
        textures[nTextures++] = ri.nightTex;
    }

    if (ri.overlayTex != nullptr)
    {
        shadprop.texUsage |= ShaderProperties::OverlayTexture;
        textures[nTextures++] = ri.overlayTex;
    }

    if (atmosphere != nullptr)
    {
        if ((renderFlags & Renderer::ShowAtmospheres) != 0)
        {
            // Only use new atmosphere code in OpenGL 2.0 path when new style parameters are defined.
            // ... but don't show atmospheres when there are no light sources.
            if (atmosphere->mieScaleHeight > 0.0f && shadprop.nLights > 0)
                shadprop.texUsage |= ShaderProperties::Scattering;
        }

        if ((renderFlags & Renderer::ShowCloudMaps) != 0 &&
            (renderFlags & Renderer::ShowCloudShadows) != 0)
        {
            Texture* cloudTex = nullptr;
            if (atmosphere->cloudTexture.tex[textureRes] != InvalidResource)
                cloudTex = atmosphere->cloudTexture.find(textureRes);

            // The current implementation of cloud shadows is not compatible
            // with virtual or split textures.
            bool allowCloudShadows = true;
            for (unsigned int i = 0; i < nTextures; i++)
            {
                if (textures[i] != nullptr &&
                    (textures[i]->getLODCount() > 1 ||
                     textures[i]->getUTileCount(0) > 1 ||
                     textures[i]->getVTileCount(0) > 1))
                {
                    allowCloudShadows = false;
                }
            }

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
                shadprop.texUsage |= ShaderProperties::CloudShadowTexture;
                textures[nTextures++] = cloudTex;
                glActiveTexture(GL_TEXTURE0 + nTextures);
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
    // Track the total number of shadows; if there are too many, we'll have
    // to fall back to multipass.
    unsigned int totalShadows = 0;

    for (unsigned int li = 0; li < ls.nLights; li++)
    {
        if (ls.shadows[li] && !ls.shadows[li]->empty())
        {
            unsigned int nShadows = (unsigned int) min((size_t) MaxShaderEclipseShadows, ls.shadows[li]->size());
            shadprop.setEclipseShadowCountForLight(li, nShadows);
            totalShadows += nShadows;
        }
    }

    if (ls.shadowingRingSystem)
    {
        Texture* ringsTex = ls.shadowingRingSystem->texture.find(textureRes);
        if (ringsTex != nullptr)
        {
            glActiveTexture(GL_TEXTURE0 + nTextures);
            ringsTex->bind();
            nTextures++;

#ifdef GL_ES
            if (gl::OES_texture_border_clamp)
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

            shadprop.texUsage |= ShaderProperties::RingShadowTexture;

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

#ifdef USE_HDR
    prog->setLightParameters(ls, ri.color, ri.specularColor, Color::Black, ri.nightLightScale);
#else
    prog->setLightParameters(ls, ri.color, ri.specularColor, Color::Black);
#endif

    prog->eyePosition = ls.eyePos_obj;
    prog->shininess = ri.specularPower;
    if ((shadprop.lightModel & ShaderProperties::LunarLambertModel) != 0)
        prog->lunarLambert = ri.lunarLambert;

    if ((shadprop.texUsage & ShaderProperties::RingShadowTexture) != 0)
    {
        float ringWidth = ls.shadowingRingSystem->outerRadius - ls.shadowingRingSystem->innerRadius;
        prog->ringRadius = ls.shadowingRingSystem->innerRadius / radius;
        prog->ringWidth = radius / ringWidth;
        prog->ringPlane = Hyperplane<float, 3>(ls.ringPlaneNormal, ls.ringCenter / radius).coeffs();
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
        if ((shadprop.texUsage & ShaderProperties::CloudShadowTexture) != 0)
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
    g_lodSphere->render(attributes,
                        frustum, ri.pixWidth,
                        textures[0], textures[1], textures[2], textures[3]);
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
                         uint64_t renderFlags,
                         const Quaternionf& planetOrientation,
                         double tsec,
                         const Matrices &m,
                         Renderer* renderer)
{
    auto *shadowBuffer = renderer->getShadowFBO(0);
    Matrix4f lightMatrix(Matrix4f::Identity());

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
        renderer->disableDepthTest();
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
        renderer->enableDepthTest();
#endif
        glDepthRange(range[0], range[1]);
    }

    GLSL_RenderContext rc(renderer, ls, geometryScale, planetOrientation);
    rc.setModelViewMatrix(m.modelview);
    rc.setProjectionMatrix(m.projection);

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

    // Handle material override; a texture specified in an ssc file will
    // override all materials specified in the geometry file.
    if (texOverride != InvalidResource)
    {
        Material m;
        m.diffuse = Material::Color(ri.color);
        m.specular = Material::Color(ri.specularColor);
        m.specularPower = ri.specularPower;

        CelestiaTextureResource textureResource(texOverride);
        m.maps[Material::DiffuseMap] = &textureResource;
        rc.setMaterial(&m);
        rc.lock();
        geometry->render(rc, tsec);
        m.maps[Material::DiffuseMap] = nullptr; // prevent Material destructor from deleting the texture resource
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
                               uint64_t /* renderFlags */,
                               const Quaternionf& /* planetOrientation */,
                               double tsec,
                               const Matrices &m,
                               Renderer* renderer)
{
    GLSLUnlit_RenderContext rc(renderer, geometryScale);

    rc.setModelViewMatrix(m.modelview);
    rc.setProjectionMatrix(m.projection);
    rc.setPointScale(ri.pointScale);

    // Handle material override; a texture specified in an ssc file will
    // override all materials specified in the model file.
    if (texOverride != InvalidResource)
    {
        Material m;
        m.diffuse = Material::Color(ri.color);
        m.specular = Material::Color(ri.specularColor);
        m.specularPower = ri.specularPower;

        CelestiaTextureResource textureResource(texOverride);
        m.maps[Material::DiffuseMap] = &textureResource;
        rc.setMaterial(&m);
        rc.lock();
        geometry->render(rc, tsec);
        m.maps[Material::DiffuseMap] = nullptr; // prevent Material destructor from deleting the texture resource
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
                       const Vector3f& semiAxes,
                       unsigned int /*textureRes*/,
                       uint64_t renderFlags,
                       const Quaternionf& planetOrientation,
                       const Frustum& frustum,
                       const Matrices &m,
                       Renderer* renderer)
{
    float radius = semiAxes.maxCoeff();

    Texture* textures[MAX_SPHERE_MESH_TEXTURES] =
        { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
    unsigned int nTextures = 0;

    ShaderProperties shadprop;
    shadprop.nLights = ls.nLights;

    // Set up the textures used by this object
    if (cloudTex != nullptr)
    {
        shadprop.texUsage = ShaderProperties::DiffuseTexture;
        textures[nTextures++] = cloudTex;
    }

    if (cloudNormalMap != nullptr)
    {
        shadprop.texUsage |= ShaderProperties::NormalTexture;
        textures[nTextures++] = cloudNormalMap;
        if (cloudNormalMap->getFormatOptions() & Texture::DXT5NormalMap)
            shadprop.texUsage |= ShaderProperties::CompressedNormalTexture;
    }

#if 0
    if (rings != nullptr && (renderFlags & Renderer::ShowRingShadows) != 0)
    {
        Texture* ringsTex = rings->texture.find(textureRes);
        if (ringsTex != nullptr)
        {
            glActiveTexture(GL_TEXTURE0 + nTextures);
            ringsTex->bind();
            nTextures++;

            // Tweak the texture--set clamp to border and a border color with
            // a zero alpha.
            float bc[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, bc);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                            GL_CLAMP_TO_BORDER);
            glActiveTexture(GL_TEXTURE0);

            shadprop.texUsage |= ShaderProperties::RingShadowTexture;
        }
    }
#endif

    if (atmosphere != nullptr)
    {
        if ((renderFlags & Renderer::ShowAtmospheres) != 0)
        {
            // Only use new atmosphere code in OpenGL 2.0 path when new style parameters are defined.
            // ... but don't show atmospheres when there are no light sources.
            if (atmosphere->mieScaleHeight > 0.0f && shadprop.nLights > 0)
                shadprop.texUsage |= ShaderProperties::Scattering;
        }
    }

    // Set the shadow information.
    // Track the total number of shadows; if there are too many, we'll have
    // to fall back to multipass.
    unsigned int totalShadows = 0;
    for (unsigned int li = 0; li < ls.nLights; li++)
    {
        if (ls.shadows[li] && !ls.shadows[li]->empty())
        {
            unsigned int nShadows = (unsigned int) min((size_t) MaxShaderEclipseShadows, ls.shadows[li]->size());
            shadprop.setEclipseShadowCountForLight(li, nShadows);
            totalShadows += nShadows;
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
    g_lodSphere->render(attributes,
                        frustum, ri.pixWidth,
                        textures[0], textures[1], textures[2], textures[3]);

    prog->textureOffset = 0.0f;
}


// Render the sky sphere for a world with an atmosphere
void
renderAtmosphere_GLSL(const RenderInfo& ri,
                      const LightingState& ls,
                      Atmosphere* atmosphere,
                      float radius,
                      const Quaternionf& /*planetOrientation*/,
                      const Frustum& frustum,
                      const Matrices &m,
                      Renderer* renderer)
{
    // Currently, we just skip rendering an atmosphere when there are no
    // light sources, even though the atmosphere would still the light
    // of planets and stars behind it.
    if (ls.nLights == 0)
        return;

    ShaderProperties shadprop;
    shadprop.nLights = ls.nLights;

    shadprop.texUsage |= ShaderProperties::Scattering;
    shadprop.lightModel = ShaderProperties::AtmosphereModel;

    // Get a shader for the current rendering configuration
    CelestiaGLProgram* prog = renderer->getShaderManager().getShader(shadprop);
    if (prog == nullptr)
        return;

    prog->use();

    prog->setLightParameters(ls, ri.color, ri.specularColor, Color::Black);
    prog->ambientColor = Vector3f::Zero();

    float atmosphereRadius = radius + -atmosphere->mieScaleHeight * log(AtmosphereExtinctionThreshold);
    float atmScale = atmosphereRadius / radius;

    prog->eyePosition = ls.eyePos_obj / atmScale;
    prog->setAtmosphereParameters(*atmosphere, radius, atmosphereRadius);

#if 0
    // Currently eclipse shadows are ignored when rendering atmospheres
    if (shadprop.shadowCounts != 0)
        prog->setEclipseShadowParameters(ls, radius, planetOrientation);
#endif

    prog->setMVPMatrices(*m.projection, (*m.modelview) * vecgl::scale(atmScale));

    glFrontFace(GL_CW);
    renderer->enableBlending();
    renderer->disableDepthMask();
    renderer->setBlendingFactors(GL_ONE, GL_SRC_ALPHA);

    g_lodSphere->render(LODSphereMesh::Normals,
                        frustum,
                        ri.pixWidth,
                        nullptr);

    renderer->disableBlending();
    renderer->enableDepthMask();
    glFrontFace(GL_CCW);
}

static void renderRingSystem(GLuint *vboId,
                             float innerRadius,
                             float outerRadius,
                             unsigned int nSections = 180)
{
    struct RingVertex
    {
        GLfloat pos[3];
        GLshort tex[2];
    };

    constexpr const float angle = 2*static_cast<float>(PI);

    if (*vboId == 0)
    {
        struct RingVertex vertex;
        vector<struct RingVertex> ringCoord;
        ringCoord.reserve(2 * nSections);
        for (unsigned i = 0; i <= nSections; i++)
        {
            float t = (float) i / (float) nSections;
            float theta = t * angle;
            float s = (float) sin(theta);
            float c = (float) cos(theta);

            // inner point
            vertex.pos[0] = c * innerRadius;
            vertex.pos[1] = 0.0f;
            vertex.pos[2] = s * innerRadius;
            vertex.tex[0] = 0;
            vertex.tex[1] = (i & 1) ^ 1; // even?(i) ? 0 : 1;
            ringCoord.push_back(vertex);

            // outer point
            vertex.pos[0] = c * outerRadius;
            // vertex.pos[1] = 0.0f;
            vertex.pos[2] = s * outerRadius;
            vertex.tex[0] = 1;
            // vertex.tex[1] = (i & 1) ^ 1;
            ringCoord.push_back(vertex);
        }

        glGenBuffers(1, vboId);
        glBindBuffer(GL_ARRAY_BUFFER, *vboId);
        glBufferData(GL_ARRAY_BUFFER,
                     ringCoord.size() * sizeof(struct RingVertex),
                     ringCoord.data(),
                     GL_STATIC_DRAW);
    }
    else
    {
        glBindBuffer(GL_ARRAY_BUFFER, *vboId);
    }
    glEnableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex);
    glVertexAttribPointer(CelestiaGLProgram::TextureCoord0AttributeIndex,
                          2, GL_SHORT, GL_FALSE,
                          sizeof(struct RingVertex),
                          (GLvoid*) offsetof(struct RingVertex, tex));

    glEnableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glVertexAttribPointer(CelestiaGLProgram::VertexCoordAttributeIndex,
                          3, GL_FLOAT, GL_FALSE,
                          sizeof(struct RingVertex), 0);

    // Celestia uses glCullFace(GL_BACK) by default so we just skip it here
    glDrawArrays(GL_TRIANGLE_STRIP, 0, (nSections+1)*2);
    glCullFace(GL_FRONT);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, (nSections+1)*2);
    glCullFace(GL_BACK);

    glDisableVertexAttribArray(CelestiaGLProgram::TextureCoord0AttributeIndex);
    glDisableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}


class GLRingRenderData : public RingRenderData
{
 public:
    ~GLRingRenderData() override
    {
        glDeleteBuffers(vboId.size(), vboId.data());
        vboId.fill(0);
    }

    std::array<GLuint, 4> vboId {{ 0, 0, 0, 0 }};
};

// Render a planetary ring system
void renderRings_GLSL(RingSystem& rings,
                      RenderInfo& ri,
                      const LightingState& ls,
                      float planetRadius,
                      float planetOblateness,
                      unsigned int textureResolution,
                      bool renderShadow,
                      float segmentSizeInPixels,
                      const Matrices &m,
                      Renderer* renderer)
{
    float inner = rings.innerRadius / planetRadius;
    float outer = rings.outerRadius / planetRadius;
    Texture* ringsTex = rings.texture.find(textureResolution);

    ShaderProperties shadprop;
    // Set up the shader properties for ring rendering
    {
        shadprop.lightModel = ShaderProperties::RingIllumModel;
        shadprop.nLights = min(ls.nLights, MaxShaderLights);

        if (renderShadow)
        {
            // Set one shadow (the planet's) per light
            for (unsigned int li = 0; li < ls.nLights; li++)
                shadprop.setEclipseShadowCountForLight(li, 1);
        }

        if (ringsTex != nullptr)
            shadprop.texUsage = ShaderProperties::DiffuseTexture;
    }


    // Get a shader for the current rendering configuration
    CelestiaGLProgram* prog = renderer->getShaderManager().getShader(shadprop);
    if (prog == nullptr)
        return;

    prog->use();
    prog->setMVPMatrices(*m.projection, *m.modelview);

    prog->eyePosition = ls.eyePos_obj;
    prog->ambientColor = ri.ambientColor.toVector3();
    prog->setLightParameters(ls, ri.color, ri.specularColor, Color::Black);

    for (unsigned int li = 0; li < ls.nLights; li++)
    {
        const DirectionalLight& light = ls.lights[li];

        // Compute the projection vectors based on the sun direction.
        // I'm being a little careless here--if the sun direction lies
        // along the y-axis, this will fail.  It's unlikely that a
        // planet would ever orbit underneath its sun (an orbital
        // inclination of 90 degrees), but this should be made
        // more robust anyway.
        Vector3f axis = Vector3f::UnitY().cross(light.direction_obj);
        float cosAngle = Vector3f::UnitY().dot(light.direction_obj);
        axis.normalize();

        float tScale = 1.0f;
        if (planetOblateness != 0.0f)
        {
            // For oblate planets, the size of the shadow volume will vary
            // based on the light direction.

            // A vertical slice of the planet is an ellipse
            float a = 1.0f;                          // semimajor axis
            float b = a * (1.0f - planetOblateness); // semiminor axis
            float ecc2 = 1.0f - (b * b) / (a * a);   // square of eccentricity

            // Calculate the radius of the ellipse at the incident angle of the
            // light on the ring plane + 90 degrees.
            float r = a * (float) sqrt((1.0f - ecc2) /
                                       (1.0f - ecc2 * square(cosAngle)));

            tScale *= a / r;
        }

        // The s axis is perpendicular to the shadow axis in the plane of the
        // of the rings, and the t axis completes the orthonormal basis.
        Vector3f sAxis = axis * 0.5f;
        Vector3f tAxis = (axis.cross(light.direction_obj)) * 0.5f * tScale;
        Vector4f texGenS;
        texGenS.head(3) = sAxis;
        texGenS[3] = 0.5f;
        Vector4f texGenT;
        texGenT.head(3) = tAxis;
        texGenT[3] = 0.5f;

        // r0 and r1 determine the size of the planet's shadow and penumbra
        // on the rings.
        // TODO: A more accurate ring shadow calculation would set r1 / r0
        // to the ratio of the apparent sizes of the planet and sun as seen
        // from the rings. Even more realism could be attained by letting
        // this ratio vary across the rings, though it may not make enough
        // of a visual difference to be worth the extra effort.
        float r0 = 0.24f;
        float r1 = 0.25f;
        float bias = 1.0f / (1.0f - r1 / r0);

        prog->shadows[li][0].texGenS = texGenS;
        prog->shadows[li][0].texGenT = texGenT;
        prog->shadows[li][0].maxDepth = 1.0f;
        prog->shadows[li][0].falloff = bias / r0;
    }

    renderer->enableBlending();
    renderer->setBlendingFactors(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (ringsTex != nullptr)
        ringsTex->bind();

    if (rings.renderData == nullptr)
        rings.renderData = shared_ptr<GLRingRenderData>(new GLRingRenderData);
    auto data = reinterpret_cast<GLRingRenderData*>(rings.renderData.get());

    unsigned nSections = 180;
    size_t i = 0;
    for (i = 0; i < data->vboId.size() - 1; i++)
    {
        float s = segmentSizeInPixels * tan(PI / nSections);
        if (s < 30.0f) // TODO: make configurable
            break;
        nSections <<= 1;
    }
    renderRingSystem(&data->vboId[i], inner, outer, nSections);

    renderer->setBlendingFactors(GL_SRC_ALPHA, GL_ONE);
}

// Calculate the matrix used to render the model from the
// perspective of the light.
static
Matrix4f directionalLightMatrix(const Vector3f& lightDirection)
{
    const Vector3f &viewDir = lightDirection;
    Vector3f upDir = viewDir.unitOrthogonal();
    Vector3f rightDir = upDir.cross(viewDir);
    Matrix4f m = Matrix4f::Identity();

    m.row(0).head(3) = rightDir;
    m.row(1).head(3) = upDir;
    m.row(2).head(3) = viewDir;

    return m;
}

/*! Render a mesh object
 *  Parameters:
 *    tsec : animation clock time in seconds
 */
static
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
    renderer->enableDepthMask();
    renderer->enableDepthTest();
    glClear(GL_DEPTH_BUFFER_BIT);
    // Render backfaces only in order to reduce self-shadowing artifacts
    glCullFace(GL_FRONT);

    Shadow_RenderContext rc(renderer);

    prog->use();

    // Enable poligon offset to decrease "shadow acne"
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(.001f, .001f);

    Matrix4f projMat = Ortho(-1.f, 1.f, -1.f, 1.f, -1.f, 1.f);
    Matrix4f modelViewMat = directionalLightMatrix(ls.lights[lightIndex].direction_obj);
    *lightMatrix = projMat * modelViewMat;
    prog->setMVPMatrices(projMat, modelViewMat);
    geometry->render(rc, tsec);

    glDisable(GL_POLYGON_OFFSET_FILL);
    // Re-enable the color buffer
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glCullFace(GL_BACK);
    shadowFbo->unbind(oldFboId);
}
