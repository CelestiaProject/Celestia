// renderglsl.cpp
//
// Functions for rendering objects using dynamically generated GLSL shaders.
//
// Copyright (C) 2006-2007, Chris Laurel <claurel@shatters.net>
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

#include <celutil/debug.h>
#include <celmath/frustum.h>
#include <celmath/distance.h>
#include <celmath/intersect.h>
#include <celutil/utf8.h>
#include <celutil/util.h>
#include "gl.h"
#include "astro.h"
#include "glext.h"
#include "vecgl.h"
#include "glshader.h"
#include "shadermanager.h"
#include "spheremesh.h"
#include "lodspheremesh.h"
#include "model.h"
#include "regcombine.h"
#include "vertexprog.h"
#include "texmanager.h"
#include "meshmanager.h"
#include "render.h"
#include "renderinfo.h"
#include "renderglsl.h"


using namespace std;


const double AtmosphereExtinctionThreshold = 0.05;


// Render a planet sphere with GLSL shaders
void renderSphere_GLSL(const RenderInfo& ri,
                       const LightingState& ls,
                       RingSystem* rings,
                       Atmosphere* atmosphere,
                       float cloudTexOffset,
                       float radius,
                       unsigned int textureRes,
                       int renderFlags,
                       const Mat4f& planetMat,
                       const Frustum& frustum,
                       const GLContext& context)
{
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

    if (rings != NULL && (renderFlags & Renderer::ShowRingShadows) != 0)
    {
        Texture* ringsTex = rings->texture.find(textureRes);
        if (ringsTex != NULL)
        {
            glx::glActiveTextureARB(GL_TEXTURE0_ARB + nTextures);
            ringsTex->bind();
            nTextures++;

            // Tweak the texture--set clamp to border and a border color with
            // a zero alpha.
            float bc[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, bc);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                            GL_CLAMP_TO_BORDER_ARB);
            glx::glActiveTextureARB(GL_TEXTURE0_ARB);

            shadprop.texUsage |= ShaderProperties::RingShadowTexture;
        }
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
                glx::glActiveTextureARB(GL_TEXTURE0_ARB + nTextures);
                cloudTex->bind();
                glx::glActiveTextureARB(GL_TEXTURE0_ARB);
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
            unsigned int nShadows = (unsigned int) min((size_t) MaxShaderShadows, ls.shadows[li]->size());
            shadprop.setShadowCountForLight(li, nShadows);
            totalShadows += nShadows;
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
        float ringWidth = rings->outerRadius - rings->innerRadius;
        prog->ringRadius = rings->innerRadius / radius;
        prog->ringWidth = radius / ringWidth;
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

    if (shadprop.shadowCounts != 0)
        prog->setEclipseShadowParameters(ls, radius, planetMat);

    glColor(ri.color);

    unsigned int attributes = LODSphereMesh::Normals;
    if (ri.bumpTex != NULL)
        attributes |= LODSphereMesh::Tangents;
    g_lodSphere->render(context,
                        attributes,
                        frustum, ri.pixWidth,
                        textures[0], textures[1], textures[2], textures[3]);

    glx::glUseProgramObjectARB(0);
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
                         const Mat4f& planetMat,
                         double tsec)
{
    glDisable(GL_LIGHTING);

    GLSL_RenderContext rc(ls, geometryScale, planetMat);

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
        Mesh::Material m;
        m.diffuse = ri.color;
        m.specular = ri.specularColor;
        m.specularPower = ri.specularPower;
        m.maps[Mesh::DiffuseMap] = texOverride;
        rc.setMaterial(&m);
        rc.lock();
        geometry->render(rc, tsec);
    }
    else
    {
        geometry->render(rc, tsec);
    }

    glx::glUseProgramObjectARB(0);
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
                               const Mat4f& /* planetMat */,
                               double tsec)
{
    glDisable(GL_LIGHTING);

    GLSLUnlit_RenderContext rc(geometryScale);

    rc.setPointScale(ri.pointScale);

    // Handle material override; a texture specified in an ssc file will
    // override all materials specified in the model file.
    if (texOverride != InvalidResource)
    {
        Mesh::Material m;
        m.diffuse = ri.color;
        m.specular = ri.specularColor;
        m.specularPower = ri.specularPower;
        m.maps[Mesh::DiffuseMap] = texOverride;
        rc.setMaterial(&m);
        rc.lock();
        geometry->render(rc, tsec);
    }
    else
    {
        geometry->render(rc, tsec);
    }

    glx::glUseProgramObjectARB(0);
}


// Render the cloud sphere for a world a cloud layer defined
void renderClouds_GLSL(const RenderInfo& ri,
                       const LightingState& ls,
                       Atmosphere* atmosphere,
                       Texture* cloudTex,
                       Texture* cloudNormalMap,
                       float texOffset,
                       RingSystem* rings,
                       float radius,
                       unsigned int textureRes,
                       int renderFlags,
                       const Mat4f& planetMat,
                       const Frustum& frustum,
                       const GLContext& context)
{
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

    if (rings != NULL && (renderFlags & Renderer::ShowRingShadows) != 0)
    {
        Texture* ringsTex = rings->texture.find(textureRes);
        if (ringsTex != NULL)
        {
            glx::glActiveTextureARB(GL_TEXTURE0_ARB + nTextures);
            ringsTex->bind();
            nTextures++;

            // Tweak the texture--set clamp to border and a border color with
            // a zero alpha.
            float bc[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, bc);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                            GL_CLAMP_TO_BORDER_ARB);
            glx::glActiveTextureARB(GL_TEXTURE0_ARB);

            shadprop.texUsage |= ShaderProperties::RingShadowTexture;
        }
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
    }

    // Set the shadow information.
    // Track the total number of shadows; if there are too many, we'll have
    // to fall back to multipass.
    unsigned int totalShadows = 0;
    for (unsigned int li = 0; li < ls.nLights; li++)
    {
        if (ls.shadows[li] && !ls.shadows[li]->empty())
        {
            unsigned int nShadows = (unsigned int) min((size_t) MaxShaderShadows, ls.shadows[li]->size());
            shadprop.setShadowCountForLight(li, nShadows);
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
    prog->ambientColor = Vec3f(ri.ambientColor.red(), ri.ambientColor.green(),
                               ri.ambientColor.blue());
    prog->textureOffset = texOffset;

    float cloudRadius = radius + atmosphere->cloudHeight;

    if (shadprop.hasScattering())
    {
        prog->setAtmosphereParameters(*atmosphere, radius, cloudRadius);
    }

    if (shadprop.texUsage & ShaderProperties::RingShadowTexture)
    {
        float ringWidth = rings->outerRadius - rings->innerRadius;
        prog->ringRadius = rings->innerRadius / cloudRadius;
        prog->ringWidth = 1.0f / (ringWidth / cloudRadius);
    }

    if (shadprop.shadowCounts != 0)
        prog->setEclipseShadowParameters(ls, cloudRadius, planetMat);

    unsigned int attributes = LODSphereMesh::Normals;
    if (cloudNormalMap != NULL)
        attributes |= LODSphereMesh::Tangents;
    g_lodSphere->render(context,
                        attributes,
                        frustum, ri.pixWidth,
                        textures[0], textures[1], textures[2], textures[3]);

    prog->textureOffset = 0.0f;

    glx::glUseProgramObjectARB(0);
}


// Render the sky sphere for a world with an atmosphere
void
renderAtmosphere_GLSL(const RenderInfo& ri,
                      const LightingState& ls,
                      Atmosphere* atmosphere,
                      float radius,
                      const Mat4f& /*planetMat*/,
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
    prog->ambientColor = Vec3f(0.0f, 0.0f, 0.0f);

    float atmosphereRadius = radius + -atmosphere->mieScaleHeight * (float) log(AtmosphereExtinctionThreshold);
    float atmScale = atmosphereRadius / radius;

    prog->eyePosition = Point3f(ls.eyePos_obj.x / atmScale, ls.eyePos_obj.y / atmScale, ls.eyePos_obj.z / atmScale);
    prog->setAtmosphereParameters(*atmosphere, radius, atmosphereRadius);

#if 0
    // Currently eclipse shadows are ignored when rendering atmospheres
    if (shadprop.shadowCounts != 0)
        prog->setEclipseShadowParameters(ls, radius, planetMat);
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


    glx::glUseProgramObjectARB(0);

    //glx::glActiveTextureARB(GL_TEXTURE0_ARB);
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
                shadprop.setShadowCountForLight(li, 1);
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
    prog->ambientColor = Vec3f(ri.ambientColor.red(), ri.ambientColor.green(),
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
        Vec3f axis = Vec3f(0, 1, 0) ^ light.direction_obj;
        float cosAngle = Vec3f(0.0f, 1.0f, 0.0f) * light.direction_obj;
        /*float angle = (float) acos(cosAngle);     Unused*/
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
        Vec3f sAxis = axis * 0.5f;
        Vec3f tAxis = (axis ^ light.direction_obj) * 0.5f * tScale;
        Vec4f texGenS(sAxis.x, sAxis.y, sAxis.z, 0.5f);
        Vec4f texGenT(tAxis.x, tAxis.y, tAxis.z, 0.5f);

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

    glx::glUseProgramObjectARB(0);
}
