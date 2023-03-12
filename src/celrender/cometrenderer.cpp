// cometrenderer.h
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "cometrenderer.h"

#include <algorithm>
#include <cmath>

#include <Eigen/Geometry>

#include <celcompat/numbers.h>
#include <celengine/astro.h>
#include <celengine/body.h>
#include <celengine/glsupport.h>
#include <celengine/observer.h>
#include <celengine/render.h>
#include <celengine/shadermanager.h>
#include <celmath/mathlib.h>
#include <celmath/vecgl.h>
#include <celutil/indexlist.h>
#include <celrender/gl/buffer.h>
#include <celrender/gl/vertexobject.h>


using celestia::util::BuildIndexList;
using celestia::util::IndexListCapacity;
using ushort = unsigned short;

namespace celestia::render
{

namespace
{

constexpr int MaxCometTailPoints = 120;
constexpr int MaxCometTailSlices = 48;
constexpr int MaxVertices = MaxCometTailPoints*MaxCometTailSlices;
constexpr int MaxIndices = IndexListCapacity(MaxCometTailSlices, MaxCometTailPoints);

// Distance from the Sun at which comet tails will start to fade out
constexpr float CometTailAttenDistSol = astro::AUtoKilometers(5.0f);

} // end unnamed namespace

CometRenderer::CometRenderer(Renderer &renderer) :
    m_renderer(renderer),
    m_vertices(std::make_unique<CometTailVertex[]>(MaxVertices)),
    m_indices(std::make_unique<ushort[]>(MaxIndices))
{
}

CometRenderer::~CometRenderer() = default;

void
CometRenderer::initGL()
{
    if (m_initialized)
        return;

    m_initialized = true;

    m_prog = m_renderer.getShaderManager().getShader("comet");
    m_brightnessLoc = m_prog->attribIndex("in_Brightness");

    m_vo = std::make_unique<gl::VertexObject>();
    m_bo = std::make_unique<gl::Buffer>(gl::Buffer::TargetHint::Array);
    m_io = std::make_unique<gl::Buffer>(gl::Buffer::TargetHint::ElementArray);

    m_vo->addVertexBuffer(
            *m_bo,
            CelestiaGLProgram::VertexCoordAttributeIndex,
            3,
            gl::VertexObject::DataType::Float,
            false,
            sizeof(CometTailVertex),
            offsetof(CometTailVertex, point))
        .addVertexBuffer(
            *m_bo,
            CelestiaGLProgram::NormalAttributeIndex,
            3,
            gl::VertexObject::DataType::Float,
            false,
            sizeof(CometTailVertex),
            offsetof(CometTailVertex, normal))
        .addVertexBuffer(
            *m_bo,
            m_brightnessLoc,
            1,
            gl::VertexObject::DataType::Float,
            false,
            sizeof(CometTailVertex),
            offsetof(CometTailVertex, brightness))
        .setIndexBuffer(*m_io, 0, gl::VertexObject::IndexType::UnsignedShort);
}

void
CometRenderer::deinitGL()
{
    m_initialized = false;
    m_vo = nullptr;
    m_bo = nullptr;
    m_io = nullptr;
}

void
CometRenderer::render(const Body &body,
                      const Observer &observer,
                      const Eigen::Vector3f &pos,
                      float dustTailLength,
                      float discSizeInPixels,
                      const Matrices &m)
{
    if (m_prog == nullptr)
        return;

    double now = observer.getTime();

    Eigen::Vector3f cometPoints[MaxCometTailPoints];
#if 0
    Eigen::Vector3d pos0 = body.getOrbit(now)->positionAtTime(now);
    Eigen::Vector3d pos1 = body.getOrbit(now)->positionAtTime(now - 0.01);
    Eigen::Vector3d vd = pos1 - pos0;
#endif

    // Adjust the amount of triangles used for the comet tail based on
    // the screen size of the comet.
    float lod = std::clamp(discSizeInPixels / 1000.0f, 0.2f, 1.0f);
    auto nTailPoints = static_cast<int>(MaxCometTailPoints * lod);
    auto nTailSlices = static_cast<int>(MaxCometTailSlices * lod);

    float irradiance_max = 0.0f;
    // Find the sun with the largest irrradiance of light onto the comet
    // as function of the comet's position;
    // irradiance = sun's luminosity / square(distanceFromSun);
    Eigen::Vector3d sunPos(Eigen::Vector3d::Zero());
    for (const auto star : m_renderer.getNearStars())
    {
        if (star->getVisibility())
        {
            Eigen::Vector3d p = star->getPosition(now).offsetFromKm(observer.getPosition());
            float distanceFromSun = static_cast<float>((pos.cast<double>() - p).norm());
            float irradiance = star->getBolometricLuminosity() / celmath::square(distanceFromSun);

            if (irradiance > irradiance_max)
            {
                irradiance_max = irradiance;
                sunPos = p;
            }
        }
    }

    float fadeDistance = 1.0f / (CometTailAttenDistSol * std::sqrt(irradiance_max));

    // direction to sun with dominant light irradiance:
    Eigen::Vector3f sunDir = (pos.cast<double>() - sunPos).cast<float>().normalized();

    float dustTailRadius = dustTailLength * 0.1f;

    Eigen::Vector3f origin = -sunDir * (body.getRadius() * 100);

    for (int i = 0; i < nTailPoints; i++)
    {
        float alpha = static_cast<float>(i) / static_cast<float>(nTailPoints);
        alpha = alpha * alpha;
        cometPoints[i] = origin + sunDir * (dustTailLength * alpha);
    }

    // We need three axes to define the coordinate system for rendering the
    // comet. The first axis is the sun-to-comet direction, and the other
    // two are chose orthogonal to each other and the primary axis.
    Eigen::Vector3f v = (cometPoints[1] - cometPoints[0]).normalized();
    Eigen::Quaternionf q = body.getEclipticToEquatorial(now).cast<float>();
    Eigen::Vector3f u = v.unitOrthogonal();
    Eigen::Vector3f w = u.cross(v);

    for (int i = 0; i < nTailPoints; i++)
    {
        float brightness = 1.0f - static_cast<float>(i) / static_cast<float>(nTailPoints - 1);
        Eigen::Vector3f v0, v1;
        float w0, w1;
        // Special case for the first vertex in the comet tail
        if (i == 0)
        {
            v0 = cometPoints[1] - cometPoints[0];
            v0.normalize();
            v1 = v0;
            w0 = 1.0f;
            w1 = 0.0f;
        }
        else
        {
            v0 = cometPoints[i] - cometPoints[i - 1];
            float sectionLength = v0.norm();
            v0.normalize();

            if (i == nTailPoints - 1)
            {
                v1 = v0;
            }
            else
            {
                v1 = (cometPoints[i + 1] - cometPoints[i]).normalized();
                q.setFromTwoVectors(v0, v1);
                Eigen::Matrix3f rot = q.toRotationMatrix();
                u = rot * u;
                v = rot * v;
                w = rot * w;
            }
            float dr = dustTailRadius / static_cast<float>(nTailPoints) / sectionLength;
            w0 = std::atan(dr);
            float d = std::sqrt(1.0f + w0 * w0);
            w1 = 1.0f / d;
            w0 = w0 / d;
        }

        float radius = static_cast<float>(i) / static_cast<float>(nTailPoints) * dustTailRadius;
        for (int j = 0; j < nTailSlices; j++)
        {
            float theta = 2.0f * numbers::pi_v<float> * static_cast<float>(j) / static_cast<float>(nTailSlices);
            float s, c;
            celmath::sincos(theta, s, c);
            CometTailVertex& vtx = m_vertices[i * nTailSlices + j];
            vtx.normal = u * (s * w1) + w * (c * w1) + v * w0;
            vtx.normal.normalize();
            s *= radius;
            c *= radius;

            vtx.point = cometPoints[i] + u * s + w * c;
            vtx.brightness = brightness;
        }
    }

    BuildIndexList(static_cast<ushort>(nTailPoints-1), static_cast<ushort>(nTailSlices), m_indices.get());

    // If fadeDistFromSun = x/x0 >= 1.0, comet tail starts fading,
    // i.e. fadeFactor quickly transits from 1 to 0.
    float fadeFactor = 0.5f * (1.0f - std::tanh(fadeDistance - 1.0f / fadeDistance));

    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE};
    ps.depthTest = true;
    m_renderer.setPipelineState(ps);

    m_prog->use();
    m_prog->setMVPMatrices(*m.projection, (*m.modelview) * celmath::translate(pos));
    m_prog->vec3Param("color") = body.getCometTailColor().toVector3();
    m_prog->vec3Param("viewDir") = pos.normalized();
    m_prog->floatParam("fadeFactor") = fadeFactor;

    m_bo->bind().invalidateData().setData(
        util::array_view<CometTailVertex>(m_vertices.get(), MaxVertices),
        gl::Buffer::BufferUsage::StreamDraw);

    m_io->bind().invalidateData().setData(
        util::array_view<ushort>(m_indices.get(), MaxIndices),
        gl::Buffer::BufferUsage::StreamDraw);

    glDisable(GL_CULL_FACE);
    int count = IndexListCapacity(nTailSlices, nTailPoints);
    m_vo->draw(gl::VertexObject::Primitive::TriangleStrip, count);
    glEnable(GL_CULL_FACE);
}
}
