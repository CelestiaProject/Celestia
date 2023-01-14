// atmosphererenderer.cpp
//
// Copyright (C) 2001-present, Celestia Development Team
// Original version Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include <celengine/atmosphere.h>
#include <celengine/lightenv.h>
#include <celengine/lodspheremesh.h>
#include <celengine/render.h>
#include <celengine/renderinfo.h>
#include <celengine/shadermanager.h>
#include <celengine/vecgl.h>
#include <celutil/color.h>
#include <celutil/indexlist.h>
#include "atmosphererenderer.h"

using celestia::util::BuildIndexList;
using celestia::util::IndexListCapacity;
using ushort = unsigned short;

namespace
{
constexpr int MaxSkyRings = 32;
constexpr int MaxSkySlices = 180;
constexpr int MinSkySlices = 30;

constexpr int MaxVertices = MaxSkySlices * (MaxSkyRings + 1);
constexpr int MaxIndices = IndexListCapacity(MaxSkySlices,  MaxSkyRings + 1);
}

namespace celestia::render
{
AtmosphereRenderer::AtmosphereRenderer(Renderer &renderer) :
    m_renderer(renderer)
{
}

void AtmosphereRenderer::initGL()
{
    if (m_initialized)
        return;

    m_initialized = true;

    m_skyVertices.reserve(MaxVertices);
    m_skyIndices.reserve(MaxIndices);
    m_skyContour.reserve(MaxSkySlices + 1);

    m_vo = std::make_unique<IndexedVertexObject>(
        MaxVertices * sizeof(SkyVertex),
        GL_STREAM_DRAW,
        GL_UNSIGNED_SHORT,
        MaxIndices * sizeof(unsigned short));
}

void AtmosphereRenderer::deinitGL()
{
    m_initialized = false;
    m_vo = nullptr;
}

void
AtmosphereRenderer::computeLegacy(
    const Atmosphere         &atmosphere,
    const LightingState      &ls,
    const Eigen::Vector3f    &center,
    const Eigen::Quaternionf &orientation,
    const Eigen::Vector3f    &semiAxes,
    const Eigen::Vector3f    &sunDirection,
    float                     pixSize,
    bool                      lit)
{
    // Gradually fade in the atmosphere if it's thickness on screen is just
    // over one pixel.
    float fade = std::clamp(pixSize - 2.0f, 0.0f, 1.0f);

    Eigen::Matrix3f rot = orientation.toRotationMatrix();
    Eigen::Matrix3f irot = orientation.conjugate().toRotationMatrix();

    Eigen::Vector3f eyePos(Eigen::Vector3f::Zero());
    float radius = semiAxes.maxCoeff();
    Eigen::Vector3f eyeVec = center - eyePos;
    eyeVec = rot * eyeVec;
    double centerDist = eyeVec.norm();

    float height = atmosphere.height / radius;
    Eigen::Vector3f recipSemiAxes = semiAxes.cwiseInverse();

    // ellipDist is not the true distance from the surface unless the
    // planet is spherical.  Computing the true distance requires finding
    // the roots of a sixth degree polynomial, and isn't actually what we
    // want anyhow since the atmosphere region is just the planet ellipsoid
    // multiplied by a uniform scale factor.  The value that we do compute
    // is the distance to the surface along a line from the eye position to
    // the center of the ellipsoid.
    float ellipDist = (eyeVec.cwiseProduct(recipSemiAxes)).norm() - 1.0f;
    bool within = ellipDist < height;

    // Adjust the tesselation of the sky dome/ring based on distance from the
    // planet surface.
    int nSlices = MaxSkySlices;
    if (ellipDist < 0.25f)
    {
        nSlices = MinSkySlices + std::max(0, static_cast<int>((ellipDist * 4.0f)) * (MaxSkySlices - MinSkySlices));
        nSlices &= ~1;
    }

    int nRings = std::min(1 + static_cast<int>(pixSize) / 5, 6);
    int nHorizonRings = nRings;
    if (within)
        nRings += 12;

    float horizonHeight = height;
    if (within)
    {
        if (ellipDist <= 0.0f)
            horizonHeight = 0.0f;
        else
            horizonHeight *= std::max(std::pow(ellipDist / height, 0.33f), 0.001f);
    }

    Eigen::Vector3f e = -eyeVec;
    Eigen::Vector3f e_ = e.cwiseProduct(recipSemiAxes);
    float ee = e_.dot(e_);

    // Compute the cosine of the altitude of the sun.  This is used to compute
    // the degree of sunset/sunrise coloration.
    float cosSunAltitude = 0.0f;
    {
        // Check for a sun either directly behind or in front of the viewer
        float cosSunAngle = sunDirection.dot(e) / static_cast<float>(centerDist);
        if (cosSunAngle < -1.0f + 1.0e-6f)
        {
            cosSunAltitude = 0.0f;
        }
        else if (cosSunAngle > 1.0f - 1.0e-6f)
        {
            cosSunAltitude = 0.0f;
        }
        else
        {
            Eigen::Vector3f v = (rot * -sunDirection) * static_cast<float>(centerDist);
            Eigen::Vector3f tangentPoint = center + irot * celmath::ellipsoidTangent(recipSemiAxes, v, e, e_, ee);
            Eigen::Vector3f tangentDir = (tangentPoint - eyePos).normalized();
            cosSunAltitude = sunDirection.dot(tangentDir);
        }
    }

    Eigen::Vector3f normal = eyeVec / static_cast<float>(centerDist);

    Eigen::Vector3f uAxis, vAxis;
    if (std::abs(normal.x()) < std::abs(normal.y()) && std::abs(normal.x()) < std::abs(normal.z()))
    {
        uAxis = Eigen::Vector3f::UnitX().cross(normal);
    }
    else if (abs(eyeVec.y()) < abs(normal.z()))
    {
        uAxis = Eigen::Vector3f::UnitY().cross(normal);
    }
    else
    {
        uAxis = Eigen::Vector3f::UnitZ().cross(normal);
    }
    uAxis.normalize();
    vAxis = uAxis.cross(normal);

    // Compute the contour of the ellipsoid
    for (int i = 0; i <= nSlices; i++)
    {
        SkyContourPoint p;
        // We want rays with an origin at the eye point and tangent to the the
        // ellipsoid.
        float theta = static_cast<float>(i) / static_cast<float>(nSlices) * 2.0f * numbers::pi_v<float>;
        Eigen::Vector3f w = std::cos(theta) * uAxis + std::sin(theta) * vAxis;
        w *= static_cast<float>(centerDist);

        Eigen::Vector3f toCenter = celmath::ellipsoidTangent(recipSemiAxes, w, e, e_, ee);
        p.v = irot * toCenter;
        p.centerDist = p.v.norm();
        p.eyeDir = p.v + (center - eyePos);
        p.eyeDist = p.eyeDir.norm();
        p.eyeDir.normalize();

        float skyCapDist = std::hypot(p.eyeDist, horizonHeight * radius);
        p.cosSkyCapAltitude = p.eyeDist / skyCapDist;
        m_skyContour.push_back(p);
    }

    Eigen::Vector3f botColor = atmosphere.lowerColor.toVector3();
    Eigen::Vector3f topColor = atmosphere.upperColor.toVector3();
    Eigen::Vector3f sunsetColor = atmosphere.sunsetColor.toVector3();

    if (within)
    {
        Eigen::Vector3f skyColor = atmosphere.skyColor.toVector3();
        if (ellipDist < 0.0f)
            topColor = skyColor;
        else
            topColor = skyColor + (topColor - skyColor) * (ellipDist / height);
    }

    if (ls.nLights == 0 && lit)
    {
        botColor = topColor = sunsetColor = Eigen::Vector3f::Zero();
    }

    Eigen::Vector3f zenith = m_skyContour[0].v + m_skyContour[nSlices / 2].v;
    zenith.normalize();
    zenith *= m_skyContour[0].centerDist * (1.0f + horizonHeight * 2.0f);

    float minOpacity = within ? (1.0f - ellipDist / height) * 0.75f : 0.0f;
    float sunset = cosSunAltitude < 0.9f ? 0.0f : (cosSunAltitude - 0.9f) * 10.0f;

    // Build the list of vertices
    for (int i = 0; i <= nRings; i++)
    {
        SkyVertex vtx;
        float h = std::min(1.0f, static_cast<float>(i) / static_cast<float>(nHorizonRings));
        float hh = std::sqrt(h);
        float u = i <= nHorizonRings ? 0.0f :
            static_cast<float>(i - nHorizonRings) / static_cast<float>(nRings - nHorizonRings);
        float r = celmath::lerp(h, 1.0f - (horizonHeight * 0.05f), 1.0f + horizonHeight);

        for (int j = 0; j < nSlices; j++)
        {
            Eigen::Vector3f v;
            if (i <= nHorizonRings)
                v = m_skyContour[j].v * r;
            else
                v = vecgl::mix(m_skyContour[j].v, zenith, u) * r;
            Eigen::Vector3f p = center + v;

            Eigen::Vector3f viewDir = p.normalized();
            float cosSunAngle = viewDir.dot(sunDirection);
            float cosAltitude = viewDir.dot(m_skyContour[j].eyeDir);
            float brightness = 1.0f;
            float coloration = 0.0f;
            if (lit)
            {
                if (sunset > 0.0f && cosSunAngle > 0.7f && cosAltitude > 0.98f)
                {
                    coloration =  (1.0f / 0.30f) * (cosSunAngle - 0.70f);
                    coloration *= 50.0f * (cosAltitude - 0.98f);
                    coloration *= sunset;
                }

                cosSunAngle = m_skyContour[j].v.dot(sunDirection) / m_skyContour[j].centerDist;
                if (cosSunAngle <= -0.2f)
                    brightness = 0.0f;
                else if (cosSunAngle >= 0.3f)
                    brightness = 1.0f;
                else
                    brightness = (cosSunAngle + 0.2f) * 2.0f;
            }

            memcpy(&vtx.position[0], p.data(), vtx.position.size() * sizeof(vtx.position[0]));

            float atten = 1.0f - hh;
            Eigen::Vector3f color = vecgl::mix(botColor, topColor, hh);
            brightness *= minOpacity + (1.0f - minOpacity) * fade * atten;
            if (coloration != 0.0f)
                color = vecgl::mix(color, sunsetColor, coloration);

            Color(brightness * color.x(),
                  brightness * color.y(),
                  brightness * color.z(),
                  fade * (minOpacity + (1.0f - minOpacity)) * atten).get(&vtx.color[0]);
            m_skyVertices.push_back(vtx);
        }
    }
    m_skyContour.clear();

    // Create the index list
    BuildIndexList(static_cast<ushort>(nRings), static_cast<ushort>(nSlices), m_skyIndices);
}

void
AtmosphereRenderer::initVertexObject()
{
    m_vo->bind();
    m_vo->setVertexAttribArray(
        CelestiaGLProgram::VertexCoordAttributeIndex,
        3,
        GL_FLOAT,
        false,
        sizeof(SkyVertex),
        offsetof(SkyVertex, position));
    m_vo->setVertexAttribArray(
        CelestiaGLProgram::ColorAttributeIndex,
        4,
        GL_UNSIGNED_BYTE,
        true,
        sizeof(SkyVertex),
        offsetof(SkyVertex, color));
}

void
AtmosphereRenderer::renderLegacy(
    const Atmosphere         &atmosphere,
    const LightingState      &ls,
    const Eigen::Vector3f    &center,
    const Eigen::Quaternionf &orientation,
    const Eigen::Vector3f    &semiAxes,
    const Eigen::Vector3f    &sunDirection,
    float                     pixSize,
    bool                      lit,
    const Matrices           &m)
{
    computeLegacy(atmosphere, ls, center, orientation, semiAxes, sunDirection, pixSize, lit);

    ShaderProperties shadprop;
    shadprop.texUsage = ShaderProperties::VertexColors;
    shadprop.lightModel = ShaderProperties::UnlitModel;
    auto *prog = m_renderer.getShaderManager().getShader(shadprop);
    if (prog == nullptr)
        return;

    Renderer::PipelineState ps;
    ps.depthTest = true;
    ps.blending = true;
    ps.blendFunc = {GL_ONE, GL_ONE_MINUS_SRC_ALPHA};
    m_renderer.setPipelineState(ps);

    if (m_vo->initialized())
        m_vo->bindWritable();
    else
        initVertexObject();

    m_vo->allocate(nullptr, nullptr); // allocate or orphan GPU memory buffers

    m_vo->setBufferData(m_skyVertices.data(), 0, m_skyVertices.size() * sizeof(SkyVertex));
    m_vo->setIndexBufferData(m_skyIndices.data(), 0, m_skyIndices.size() * sizeof(unsigned short));

    prog->use();
    prog->setMVPMatrices(*m.projection, *m.modelview);
    m_vo->draw(GL_TRIANGLE_STRIP, static_cast<int>(m_skyIndices.size()), 0);
    m_vo->unbind();

    m_skyIndices.clear();
    m_skyVertices.clear();
}

void
AtmosphereRenderer::render(
    const RenderInfo         &ri,
    const Atmosphere         &atmosphere,
    const LightingState      &ls,
    const Eigen::Quaternionf &/*planetOrientation*/,
    float                     radius,
    const celmath::Frustum   &frustum,
    const Matrices           &m)
{
    // Currently, we just skip rendering an atmosphere when there are no
    // light sources, even though the atmosphere would still the light
    // of planets and stars behind it.
    if (ls.nLights == 0)
        return;

    ShaderProperties shadprop;
    shadprop.nLights = static_cast<ushort>(ls.nLights);

    shadprop.texUsage |= ShaderProperties::Scattering;
    shadprop.lightModel = ShaderProperties::AtmosphereModel;

    // Get a shader for the current rendering configuration
    CelestiaGLProgram* prog = m_renderer.getShaderManager().getShader(shadprop);
    if (prog == nullptr)
        return;

    prog->use();

    prog->setLightParameters(ls, ri.color, ri.specularColor, Color::Black);
    prog->ambientColor = Eigen::Vector3f::Zero();

    float atmosphereRadius = radius + -atmosphere.mieScaleHeight * std::log(AtmosphereExtinctionThreshold);
    float atmScale = atmosphereRadius / radius;

    prog->eyePosition = ls.eyePos_obj / atmScale;
    prog->setAtmosphereParameters(atmosphere, radius, atmosphereRadius);

#if 0
    // Currently eclipse shadows are ignored when rendering atmospheres
    if (shadprop.shadowCounts != 0)
        prog->setEclipseShadowParameters(ls, radius, planetOrientation);
#endif

    prog->setMVPMatrices(*m.projection, (*m.modelview) * vecgl::scale(atmScale));

    glFrontFace(GL_CW);

    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_ONE, GL_SRC_ALPHA};
    ps.depthTest = true;
    m_renderer.setPipelineState(ps);

    g_lodSphere->render(LODSphereMesh::Normals,
                        frustum,
                        ri.pixWidth,
                        nullptr);

    glFrontFace(GL_CCW);
}

} // namespace celestia::render
