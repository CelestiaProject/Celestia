// renderglsl.cpp
//
// Functions for rendering objects using dynamically generated GLSL shaders.
//
// Copyright (C) 2006-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cassert>

#ifndef _WIN32
#ifndef TARGET_OS_MAC
#include <config.h>
#endif
#endif /* _WIN32 */

#include "render.h"
#include "astro.h"
#include "glshader.h"
#include "shadermanager.h"
#include "spheremesh.h"
#include "lodspheremesh.h"
#include "geometry.h"
#include "regcombine.h"
#include "vertexprog.h"
#include "texmanager.h"
#include "meshmanager.h"
#include "renderinfo.h"
#include "renderglsl.h"
#include "modelgeometry.h"
#include "vecgl.h"
#include <celutil/debug.h>
#include <celmath/frustum.h>
#include <celmath/distance.h>
#include <celmath/intersect.h>
#include <celutil/utf8.h>
#include <celutil/util.h>

using namespace cmod;
using namespace Eigen;
using namespace std;


const double AtmosphereExtinctionThreshold = 0.05;


// Render a planet sphere with GLSL shaders
void renderEllipsoid_GLSL(const RenderInfo& ri,
                       const LightingState& ls,
                       Atmosphere* atmosphere,
                       float cloudTexOffset,
                       const Vector3f& semiAxes,
                       unsigned int textureRes,
                       int renderFlags,
                       const Quaternionf& planetOrientation,
                       const Frustum& frustum,
                       const GLContext& context)
{
    float radius = semiAxes.maxCoeff();

    Texture* textures[MAX_SPHERE_MESH_TEXTURES] = 
        { NULL, NULL, NULL, NULL, NULL, NULL };
    unsigned int nTextures = 0;

    glDisable(GL_LIGHTING);

    ShaderProperties shadprop;
    shadprop.nLights = min(ls.nLights, MaxShaderLights);

    // Set up the textures used by this object
    if (ri.baseTex != NULL)
    {
        shadprop.texUsage = ShaderProperties::DiffuseTexture;
        textures[nTextures++] = ri.baseTex;
    }

    if (ri.bumpTex != NULL)
    {
        shadprop.texUsage |= ShaderProperties::NormalTexture;
        textures[nTextures++] = ri.bumpTex;
        if (ri.bumpTex->getFormatOptions() & Texture::DXT5NormalMap)
            shadprop.texUsage |= ShaderProperties::CompressedNormalTexture;
    }

    if (ri.specularColor != Color::Black)
    {
        shadprop.lightModel = ShaderProperties::PerPixelSpecularModel;
        if (ri.glossTex == NULL)
        {
            shadprop.texUsage |= ShaderProperties::SpecularInDiffuseAlpha;
        }
        else
        {
            shadprop.texUsage |= ShaderProperties::SpecularTexture;
            textures[nTextures++] = ri.glossTex;
        }
    }
    else if (ri.lunarLambert != 0.0f)
    {
        // TODO: Lunar-Lambert model and specular color should not be mutually exclusive
        shadprop.lightModel = ShaderProperties::LunarLambertModel;
    }

    if (ri.nightTex != NULL)
    {
        shadprop.texUsage |= ShaderProperties::NightTexture;
        textures[nTextures++] = ri.nightTex;
    }

    if (ri.overlayTex != NULL)
    {
        shadprop.texUsage |= ShaderProperties::OverlayTexture;
        textures[nTextures++] = ri.overlayTex;
    }

    if (atmosphere != NULL)
    {
        if (renderFlags & Renderer::ShowAtmospheres)
        {
            // Only use new atmosphere code in OpenGL 2.0 path when new style parameters are defined.
            // ... but don't show atmospheres when there are no light sources.
            if (atmosphere->mieScaleHeight > 0.0f && shadprop.nLights > 0)
                shadprop.texUsage |= ShaderProperties::Scattering;
        }

        if ((renderFlags & Renderer::ShowCloudMaps) != 0 &&
            (renderFlags & Renderer::ShowCloudShadows) != 0)
        {
            Texture* cloudTex = NULL;
            if (atmosphere->cloudTexture.tex[textureRes] != InvalidResource)
                cloudTex = atmosphere->cloudTexture.find(textureRes);

            // The current implementation of cloud shadows is not compatible
            // with virtual or split textures.
            bool allowCloudShadows = true;
            for (unsigned int i = 0; i < nTextures; i++)
            {
                if (textures[i] != NULL &&
                    (textures[i]->getLODCount() > 1 ||
                     textures[i]->getUTileCount(0) > 1 ||
                     textures[i]->getVTileCount(0) > 1))
                {
                    allowCloudShadows = false;
                }
            }

            // Split cloud shadows can't cast shadows
            if (cloudTex != NULL)
            {
                if (cloudTex->getLODCount() > 1 ||
                    cloudTex->getUTileCount(0) > 1 ||
                    cloudTex->getVTileCount(0) > 1)
                {
                    allowCloudShadows = false;
                }
            }

            if (cloudTex != NULL && allowCloudShadows && atmosphere->cloudShadowDepth > 0.0f)
            {
                shadprop.texUsage |= ShaderProperties::CloudShadowTexture;
                textures[nTextures++] = cloudTex;
                glActiveTextureARB(GL_TEXTURE0_ARB + nTextures);
                cloudTex->bind();
                glActiveTextureARB(GL_TEXTURE0_ARB);

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
        if (ringsTex != NULL)
        {
            glActiveTextureARB(GL_TEXTURE0_ARB + nTextures);
            ringsTex->bind();
            nTextures++;
            
            // Tweak the texture--set clamp to border and a border color with
            // a zero alpha.
            float bc[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, bc);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER_ARB);
            glActiveTextureARB(GL_TEXTURE0_ARB);

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
    CelestiaGLProgram* prog = GetShaderManager().getShader(shadprop);
    if (prog == NULL)
        return;

    prog->use();

#ifdef USE_HDR
    prog->setLightParameters(ls, ri.color, ri.specularColor, Color::Black, ri.nightLightScale);
#else
    prog->setLightParameters(ls, ri.color, ri.specularColor, Color::Black);
#endif

    prog->eyePosition = ls.eyePos_obj;
    prog->shininess = ri.specularPower;
    if (shadprop.lightModel == ShaderProperties::LunarLambertModel)
        prog->lunarLambert = ri.lunarLambert;

    if (shadprop.texUsage & ShaderProperties::RingShadowTexture)
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

    if (shadprop.texUsage & ShaderProperties::CloudShadowTexture)
    {
        prog->shadowTextureOffset = cloudTexOffset;
        prog->cloudHeight = 1.0f + atmosphere->cloudHeight / radius;
    }

    if (shadprop.hasScattering())
    {
        prog->setAtmosphereParameters(*atmosphere, radius, radius);
    }

    if (shadprop.hasEclipseShadows() != 0)
        prog->setEclipseShadowParameters(ls, semiAxes, planetOrientation);

    glColor(ri.color);

    unsigned int attributes = LODSphereMesh::Normals;
    if (ri.bumpTex != NULL)
        attributes |= LODSphereMesh::Tangents;
    g_lodSphere->render(context,
                        attributes,
                        frustum, ri.pixWidth,
                        textures[0], textures[1], textures[2], textures[3]);

    glUseProgramObjectARB(0);
}


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
                         int renderFlags,
                         const Quaternionf& planetOrientation,
                         double tsec)
{
    glDisable(GL_LIGHTING);

    GLSL_RenderContext rc(ls, geometryScale, planetOrientation);

    if (renderFlags & Renderer::ShowAtmospheres)
    {
        rc.setAtmosphere(atmosphere);
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
        m.diffuse = Material::Color(ri.color.red(), ri.color.green(), ri.color.blue());
        m.specular = Material::Color(ri.specularColor.red(), ri.specularColor.green(), ri.specularColor.blue());
        m.specularPower = ri.specularPower;

        CelestiaTextureResource textureResource(texOverride);
        m.maps[Material::DiffuseMap] = &textureResource;
        rc.setMaterial(&m);
        rc.lock();
        geometry->render(rc, tsec);
        m.maps[Material::DiffuseMap] = NULL; // prevent Material destructor from deleting the texture resource
    }
    else
    {
        geometry->render(rc, tsec);
    }

    glUseProgramObjectARB(0);
}


/*! Render a mesh object without lighting.
 *  Parameters:
 *    tsec : animation clock time in seconds
 */
void renderGeometry_GLSL_Unlit(Geometry* geometry,
                               const RenderInfo& ri,
                               ResourceHandle texOverride,
                               float geometryScale,
                               int /* renderFlags */,
                               const Quaternionf& /* planetOrientation */,
                               double tsec)
{
    glDisable(GL_LIGHTING);

    GLSLUnlit_RenderContext rc(geometryScale);

    rc.setPointScale(ri.pointScale);

    // Handle material override; a texture specified in an ssc file will
    // override all materials specified in the model file.
    if (texOverride != InvalidResource)
    {
        Material m;
        m.diffuse = Material::Color(ri.color.red(), ri.color.green(), ri.color.blue());
        m.specular = Material::Color(ri.specularColor.red(), ri.specularColor.green(), ri.specularColor.blue());
        m.specularPower = ri.specularPower;

        CelestiaTextureResource textureResource(texOverride);
        m.maps[Material::DiffuseMap] = &textureResource;
        rc.setMaterial(&m);
        rc.lock();
        geometry->render(rc, tsec);
        m.maps[Material::DiffuseMap] = NULL; // prevent Material destructor from deleting the texture resource
    }
    else
    {
        geometry->render(rc, tsec);
    }

    glUseProgramObjectARB(0);
}


// Render the cloud sphere for a world a cloud layer defined
void renderClouds_GLSL(const RenderInfo& ri,
                       const LightingState& ls,
                       Atmosphere* atmosphere,
                       Texture* cloudTex,
                       Texture* cloudNormalMap,
                       float texOffset,
                       const Vector3f& semiAxes,
                       unsigned int textureRes,
                       int renderFlags,
                       const Quaternionf& planetOrientation,
                       const Frustum& frustum,
                       const GLContext& context)
{
    float radius = semiAxes.maxCoeff();

    Texture* textures[MAX_SPHERE_MESH_TEXTURES] = 
        { NULL, NULL, NULL, NULL, NULL, NULL };
    unsigned int nTextures = 0;

    glDisable(GL_LIGHTING);

    ShaderProperties shadprop;
    shadprop.nLights = ls.nLights;

    // Set up the textures used by this object
    if (cloudTex != NULL)
    {
        shadprop.texUsage = ShaderProperties::DiffuseTexture;
        textures[nTextures++] = cloudTex;
    }

    if (cloudNormalMap != NULL)
    {
        shadprop.texUsage |= ShaderProperties::NormalTexture;
        textures[nTextures++] = cloudNormalMap;
        if (cloudNormalMap->getFormatOptions() & Texture::DXT5NormalMap)
            shadprop.texUsage |= ShaderProperties::CompressedNormalTexture;
    }

#if 0
    if (rings != NULL && (renderFlags & Renderer::ShowRingShadows) != 0)
    {
        Texture* ringsTex = rings->texture.find(textureRes);
        if (ringsTex != NULL)
        {
            glActiveTextureARB(GL_TEXTURE0_ARB + nTextures);
            ringsTex->bind();
            nTextures++;

            // Tweak the texture--set clamp to border and a border color with
            // a zero alpha.
            float bc[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, bc);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                            GL_CLAMP_TO_BORDER_ARB);
            glActiveTextureARB(GL_TEXTURE0_ARB);

            shadprop.texUsage |= ShaderProperties::RingShadowTexture;
        }
    }
#endif
    
    if (atmosphere != NULL)
    {
        if (renderFlags & Renderer::ShowAtmospheres)
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
    CelestiaGLProgram* prog = GetShaderManager().getShader(shadprop);
    if (prog == NULL)
        return;

    prog->use();

    prog->setLightParameters(ls, ri.color, ri.specularColor, Color::Black);
    prog->eyePosition = ls.eyePos_obj;
    prog->ambientColor = Vector3f(ri.ambientColor.red(), ri.ambientColor.green(),
                                  ri.ambientColor.blue());
    prog->textureOffset = texOffset;

    float cloudRadius = radius + atmosphere->cloudHeight;

    if (shadprop.hasScattering())
    {
        prog->setAtmosphereParameters(*atmosphere, radius, cloudRadius);
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
    if (cloudNormalMap != NULL)
        attributes |= LODSphereMesh::Tangents;
    g_lodSphere->render(context,
                        attributes,
                        frustum, ri.pixWidth,
                        textures[0], textures[1], textures[2], textures[3]);

    prog->textureOffset = 0.0f;

    glUseProgramObjectARB(0);
}


// Render the sky sphere for a world with an atmosphere
void
renderAtmosphere_GLSL(const RenderInfo& ri,
                      const LightingState& ls,
                      Atmosphere* atmosphere,
                      float radius,
                      const Quaternionf& /*planetOrientation*/,
                      const Frustum& frustum,
                      const GLContext& context)
{
    // Currently, we just skip rendering an atmosphere when there are no
    // light sources, even though the atmosphere would still the light
    // of planets and stars behind it.
    if (ls.nLights == 0)
        return;

    glDisable(GL_LIGHTING);

    ShaderProperties shadprop;
    shadprop.nLights = ls.nLights;

    shadprop.texUsage |= ShaderProperties::Scattering;
    shadprop.lightModel = ShaderProperties::AtmosphereModel;

    // Get a shader for the current rendering configuration
    CelestiaGLProgram* prog = GetShaderManager().getShader(shadprop);
    if (prog == NULL)
        return;

    prog->use();

    prog->setLightParameters(ls, ri.color, ri.specularColor, Color::Black);
    prog->ambientColor = Vector3f::Zero();

    float atmosphereRadius = radius + -atmosphere->mieScaleHeight * (float) log(AtmosphereExtinctionThreshold);
    float atmScale = atmosphereRadius / radius;

    prog->eyePosition = ls.eyePos_obj / atmScale;
    prog->setAtmosphereParameters(*atmosphere, radius, atmosphereRadius);

#if 0
    // Currently eclipse shadows are ignored when rendering atmospheres
    if (shadprop.shadowCounts != 0)
        prog->setEclipseShadowParameters(ls, radius, planetOrientation);
#endif

    glPushMatrix();
    glScalef(atmScale, atmScale, atmScale);
    glFrontFace(GL_CW);
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_ONE, GL_SRC_ALPHA);

    g_lodSphere->render(context,
                        LODSphereMesh::Normals,
                        frustum,
                        ri.pixWidth,
                        NULL);

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glFrontFace(GL_CCW);
    glPopMatrix();


    glUseProgramObjectARB(0);

    //glActiveTextureARB(GL_TEXTURE0_ARB);
    //glEnable(GL_TEXTURE_2D);
}


static void renderRingSystem(float innerRadius,
                             float outerRadius,
                             float beginAngle,
                             float endAngle,
                             unsigned int nSections)
{
    float angle = endAngle - beginAngle;

    glBegin(GL_QUAD_STRIP);
    for (unsigned int i = 0; i <= nSections; i++)
    {
        float t = (float) i / (float) nSections;
        float theta = beginAngle + t * angle;
        float s = (float) sin(theta);
        float c = (float) cos(theta);
        glTexCoord2f(0, 0.5f);
        glVertex3f(c * innerRadius, 0, s * innerRadius);
        glTexCoord2f(1, 0.5f);
        glVertex3f(c * outerRadius, 0, s * outerRadius);
    }
    glEnd();
}


// Render a planetary ring system
void renderRings_GLSL(RingSystem& rings,
                      RenderInfo& ri,
                      const LightingState& ls,
                      float planetRadius,
                      float planetOblateness,
                      unsigned int textureResolution,
                      bool renderShadow,
                      unsigned int nSections)
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

        if (ringsTex)
            shadprop.texUsage = ShaderProperties::DiffuseTexture;
    }


    // Get a shader for the current rendering configuration
    CelestiaGLProgram* prog = GetShaderManager().getShader(shadprop);
    if (prog == NULL)
        return;

    prog->use();

    prog->eyePosition = ls.eyePos_obj;
    prog->ambientColor = Vector3f(ri.ambientColor.red(), ri.ambientColor.green(),
                                  ri.ambientColor.blue());
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
        texGenS.start(3) = sAxis;
        texGenS[3] = 0.5f;
        Vector4f texGenT;
        texGenT.start(3) = tAxis;
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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (ringsTex != NULL)
        ringsTex->bind();
    else
        glDisable(GL_TEXTURE_2D);

    renderRingSystem(inner, outer, 0, (float) PI * 2.0f, nSections);
    renderRingSystem(inner, outer, (float) PI * 2.0f, 0, nSections);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glUseProgramObjectARB(0);
}



/*! Render a mesh object
 *  Parameters:
 *    tsec : animation clock time in seconds
 */
void renderGeometryShadow_GLSL(Geometry* geometry,
                              FramebufferObject* shadowFbo,
                              const RenderInfo& ri,
                              const LightingState& ls,
                              float geometryScale,
                              const Quaternionf& planetOrientation,
                              double tsec)
{
    glDisable(GL_LIGHTING);

    shadowFbo->bind();
    glViewport(0, 0, shadowFbo->width(), shadowFbo->height());
    glClear(GL_DEPTH_BUFFER_BIT);

    // Write only to the depth buffer
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_TRUE);

    // Set up the camera for drawing from the light source direction

    // Render backfaces only in order to reduce self-shadowing artifacts
    glCullFace(GL_FRONT);

    GLSL_RenderContext rc(ls, geometryScale, planetOrientation);

    rc.setPointScale(ri.pointScale);

    int lightIndex = 0;
    Vector3f viewDir = -ls.lights[lightIndex].direction_obj;
    Vector3f upDir = viewDir.unitOrthogonal();
    Vector3f rightDir = upDir.cross(viewDir);


    glUseProgramObjectARB(0);

    geometry->render(rc, tsec);

    shadowFbo->unbind();

    // Re-enable the color buffer
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}




FramebufferObject::FramebufferObject(GLuint width, GLuint height, unsigned int attachments) :
    m_width(width),
    m_height(height),
    m_colorTexId(0),
    m_depthTexId(0),
    m_fboId(0),
    m_status(GL_FRAMEBUFFER_UNSUPPORTED_EXT)
{
    if (attachments != 0)
    {
        generateFbo(attachments);
    }
}


FramebufferObject::~FramebufferObject()
{
    cleanup();
}


bool
FramebufferObject::isValid() const
{
    return m_status == GL_FRAMEBUFFER_COMPLETE_EXT;
}


GLuint
FramebufferObject::colorTexture() const
{
    return m_colorTexId;
}


GLuint
FramebufferObject::depthTexture() const
{
    return m_depthTexId;
}


void
FramebufferObject::generateColorTexture()
{
    // Create and bind the texture
    glGenTextures(1, &m_colorTexId);
    glBindTexture(GL_TEXTURE_2D, m_colorTexId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Clamp to edge
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    // Set the texture dimensions
    // Do we need to set GL_DEPTH_COMPONENT24 here?
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

    // Unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);
}


void
FramebufferObject::generateDepthTexture()
{
    // Create and bind the texture
    glGenTextures(1, &m_depthTexId);
    glBindTexture(GL_TEXTURE_2D, m_depthTexId);

    // Only nearest sampling is appropriate for depth textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Clamp to edge
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    // Set the texture dimensions
    // Do we need to set GL_DEPTH_COMPONENT24 here?
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);

    // Unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);
}


void
FramebufferObject::generateFbo(unsigned int attachments)
{
    // Create the FBO
    glGenFramebuffersEXT(1, &m_fboId);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fboId);

    glReadBuffer(GL_NONE);

    if ((attachments & ColorAttachment) != 0)
    {
        generateColorTexture();
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_colorTexId, 0);
        m_status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
        if (m_status != GL_FRAMEBUFFER_COMPLETE_EXT)
        {
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
            cleanup();
            return;
        }
    }
    else
    {
        // Depth-only rendering; no color buffer.
        glDrawBuffer(GL_NONE);
    }

    if ((attachments & DepthAttachment) != 0)
    {
        generateDepthTexture();
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, m_depthTexId, 0);
        m_status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
        if (m_status != GL_FRAMEBUFFER_COMPLETE_EXT)
        {
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
            cleanup();
            return;
        }
    }
    else
    {
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, 0, 0);
    }

    // Restore default frame buffer
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}


// Delete all GL objects associated with this framebuffer object
void
FramebufferObject::cleanup()
{
    if (m_fboId != 0)
    {
        glDeleteFramebuffersEXT(1, &m_fboId);
    }

    if (m_colorTexId != 0)
    {
        glDeleteTextures(1, &m_colorTexId);
    }

    if (m_depthTexId != 0)
    {
        glDeleteTextures(1, &m_depthTexId);
    }
}


bool
FramebufferObject::bind()
{
    if (isValid())
    {
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fboId);
        return true;
    }
    else
    {
        return false;
    }
}


bool
FramebufferObject::unbind()
{
    // Restore default frame buffer
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    return true;
}

