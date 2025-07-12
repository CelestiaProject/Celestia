// axisarrow.cpp
//
// Copyright (C) 2007-2009, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <utility>
#include <vector>
#include <celcompat/numbers.h>
#include <celephem/orbit.h>
#include <celephem/rotation.h>
#include <celmath/geomutil.h>
#include <celmath/mathlib.h>
#include <celmath/vecgl.h>
#include <celrender/linerenderer.h>
#include <celrender/gl/buffer.h>
#include <celrender/gl/vertexobject.h>
#include "axisarrow.h"
#include "body.h"
#include "frame.h"
#include "render.h"
#include "selection.h"
#include "shadermanager.h"
#include "timeline.h"
#include "timelinephase.h"

using celestia::render::LineRenderer;
namespace gl = celestia::gl;
namespace math = celestia::math;

namespace
{

constexpr float shaftLength  = 0.85f;
constexpr float headLength   = 0.10f;
constexpr float shaftRadius  = 0.010f;
constexpr float headRadius   = 0.025f;
constexpr unsigned int nSections = 30;

std::vector<Eigen::Vector3f>
GetArrowVertices()
{
    constexpr unsigned int totalVertices = nSections * 3U + 3U;

    std::vector<Eigen::Vector3f> vertices;
    vertices.reserve(totalVertices);
    vertices.emplace_back(Eigen::Vector3f::Zero());
    vertices.emplace_back(0.0f, 0.0f, shaftLength);
    vertices.emplace_back(0.0f, 0.0f, shaftLength + headLength);

    for (unsigned int i = 0U; i < nSections; ++i)
    {
        float s;
        float c;
        math::sincos((static_cast<float>(i) * 2.0f * celestia::numbers::pi_v<float>) / nSections, s, c);

        vertices.emplace_back(shaftRadius * s, shaftRadius * c, 0.0f);
        vertices.emplace_back(shaftRadius * s, shaftRadius * c, shaftLength);
        vertices.emplace_back(headRadius * s, headRadius * c, shaftLength);
    }

    return vertices;
}

std::vector<GLushort>
GetArrowIndices()
{
    constexpr unsigned int circleSize = (nSections + 1U) * 3U;
    constexpr unsigned int shaftSize = 3U + nSections * 6U;
    constexpr unsigned int annulusSize = (nSections + 1U) * 3U;
    constexpr unsigned int headSize = (nSections + 1U) * 3U;

    constexpr unsigned int totalSize = circleSize + shaftSize + annulusSize + headSize;

    std::vector<GLushort> indices(totalSize, GLushort(0));

    GLushort* const circle = indices.data();
    GLushort* const shaft = circle + circleSize;
    GLushort* const annulus = shaft + shaftSize;
    GLushort* const head = annulus + annulusSize;

    GLushort* circlePtr = circle;
    GLushort* shaftPtr = shaft;
    GLushort* annulusPtr = annulus;
    GLushort* headPtr = head;

    constexpr GLushort vZero = 0;
    constexpr GLushort v3 = 1;
    constexpr GLushort v4 = 2;
    for (unsigned int i = 0U; i <= nSections; ++i)
    {
        unsigned int idx = i < nSections ? i : 0;
        auto v0 = static_cast<GLushort>(3U + idx * 3U);
        auto v1 = static_cast<GLushort>(v0 + 1U);
        auto v2 = static_cast<GLushort>(v0 + 2U);

        // Circle at bottom
        if (i > 0)
            *(circlePtr++) = v0;
        *(circlePtr++) = vZero;
        *(circlePtr++) = v1;

        // Shaft
        if (i > 0)
        {
            *(shaftPtr++) = v0; // left triangle

            *(shaftPtr++) = v0; // right
            *(shaftPtr++) = static_cast<GLushort>(idx * 3U + 1U) ; // v1Prev
            *(shaftPtr++) = v1;
        }
        *(shaftPtr++) = v0; // left
        *(shaftPtr++) = v1;

        // Annulus
        if (i > 0)
            *(annulusPtr++) = v2;
        *(annulusPtr++) = v2;
        *(annulusPtr++) = v3;

        // Head
        if (i > 0)
            *(headPtr++) = v2;
        *(headPtr++) = v4;
        *(headPtr++) = v2;
    }

    *circlePtr = circle[1];
    *shaftPtr = shaft[0];
    *annulusPtr = annulus[1];
    *headPtr = head[1];

    return indices;
}

gl::VertexObject&
GetArrowVAO()
{
    static gl::VertexObject* vo = nullptr;

    if (vo)
        return *vo;

    vo = std::make_unique<gl::VertexObject>().release();

    auto vertices = GetArrowVertices();
    auto indices = GetArrowIndices();

    static gl::Buffer* vertexBuffer = std::make_unique<gl::Buffer>(gl::Buffer::TargetHint::Array).release();
    vertexBuffer->setData(vertices, gl::Buffer::BufferUsage::StaticDraw);

    vo->addVertexBuffer(*vertexBuffer,
                        CelestiaGLProgram::VertexCoordAttributeIndex,
                        3,
                        gl::VertexObject::DataType::Float);
    gl::Buffer indexBuffer(gl::Buffer::TargetHint::ElementArray, indices);
    vo->setCount(static_cast<int>(indices.size()));
    vo->setIndexBuffer(std::move(indexBuffer), 0, gl::VertexObject::IndexType::UnsignedShort);

    return *vo;
}

} // anonymous namespace

/****** ArrowReferenceMark base class ******/

ArrowReferenceMark::ArrowReferenceMark(const Body& _body) :
    body(_body),
    size(1.0),
    color(1.0f, 1.0f, 1.0f),
    opacity(1.0f)
{
    shadprop.texUsage = TexUsage::VertexColors;
    shadprop.lightModel = LightingModel::UnlitModel;
}


void
ArrowReferenceMark::setSize(float _size)
{
    size = _size;
}


void
ArrowReferenceMark::setColor(const Color &_color)
{
    color = _color;
}


void
ArrowReferenceMark::render(Renderer* renderer,
                           const Eigen::Vector3f& position,
                           float /* discSize */,
                           double tdb,
                           const Matrices& m) const
{
    Eigen::Vector3d v = getDirection(tdb);
    if (v.norm() < 1.0e-12)
    {
        // Skip rendering of zero-length vectors
        return;
    }

    v.normalize();
    Eigen::Quaterniond q;
    q.setFromTwoVectors(Eigen::Vector3d::UnitZ(), v);

    Renderer::PipelineState ps;
    ps.depthTest = true;
    if (opacity == 1.0f)
    {
        ps.depthMask = true;
    }
    else
    {
        ps.blending = true;
        ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    }
    renderer->setPipelineState(ps);

    Eigen::Affine3f transform = Eigen::Translation3f(position) * q.cast<float>() * Eigen::Scaling(size);
    Eigen::Matrix4f mv = (*m.modelview) * transform.matrix();

    CelestiaGLProgram* prog = renderer->getShaderManager().getShader(shadprop);
    if (prog == nullptr)
        return;
    prog->use();
    prog->setMVPMatrices(*m.projection, mv);

    glVertexAttrib4f(CelestiaGLProgram::ColorAttributeIndex,
		     color.red(), color.green(), color.blue(), opacity);

    GetArrowVAO().draw();
}


/****** AxesReferenceMark base class ******/

AxesReferenceMark::AxesReferenceMark(const Body& _body) :
    body(_body),
    size(),
    opacity(1.0f)
{
    shadprop.texUsage = TexUsage::VertexColors;
    shadprop.lightModel = LightingModel::UnlitModel;
}


void
AxesReferenceMark::setSize(float _size)
{
    size = _size;
}


void
AxesReferenceMark::setOpacity(float _opacity)
{
    opacity = _opacity;
}


void
AxesReferenceMark::render(Renderer* renderer,
                          const Eigen::Vector3f& position,
                          float /* discSize */,
                          double tdb,
                          const Matrices& m) const
{
    Eigen::Quaterniond q = getOrientation(tdb);

    Renderer::PipelineState ps;
    ps.depthTest = true;
    if (opacity == 1.0f)
    {
        ps.depthMask = true;
    }
    else
    {
        ps.blending = true;
        ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    }
    renderer->setPipelineState(ps);

    Eigen::Affine3f transform = Eigen::Translation3f(position) * q.cast<float>() * Eigen::Scaling(size);
    Eigen::Matrix4f projection = *m.projection;
    Eigen::Matrix4f modelView = (*m.modelview) * transform.matrix();

#if 0
    // Simple line axes
    glBegin(GL_LINES);

    glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(-1.0f, 0.0f, 0.0f);

    glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 1.0f);

    glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);

    glEnd();
#endif

    float labelScale = 0.1f;

    CelestiaGLProgram* prog = renderer->getShaderManager().getShader(shadprop);
    if (prog == nullptr)
        return;
    prog->use();

    Eigen::Affine3f labelTransform = Eigen::Translation3f(Eigen::Vector3f(0.1f, 0.0f, 0.75f)) * Eigen::Scaling(labelScale);
    Eigen::Matrix4f labelTransformMatrix = labelTransform.matrix();

    // x-axis
    Eigen::Matrix4f xModelView = modelView * math::YRot90Matrix<float>;
    glVertexAttrib4f(CelestiaGLProgram::ColorAttributeIndex, 1.0f, 0.0f, 0.0f, opacity);
    prog->setMVPMatrices(projection, xModelView);
    GetArrowVAO().draw();

    // y-axis
    Eigen::Matrix4f yModelView = modelView * math::YRot180Matrix<float>;
    glVertexAttrib4f(CelestiaGLProgram::ColorAttributeIndex, 0.0f, 1.0f, 0.0f, opacity);
    prog->setMVPMatrices(projection, yModelView);
    GetArrowVAO().draw();

    // z-axis
    Eigen::Matrix4f zModelView = modelView * math::XRot270Matrix<float>;
    glVertexAttrib4f(CelestiaGLProgram::ColorAttributeIndex, 0.0f, 0.0f, 1.0f, opacity);
    prog->setMVPMatrices(projection, zModelView);
    GetArrowVAO().draw();

    LineRenderer lr(*renderer);
    lr.startUpdate();
    // X
    lr.addSegment({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f});
    lr.addSegment({1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f});
    // Y
    lr.addSegment({0.0f, 0.0f, 1.0f}, {0.5f, 0.0f, 0.5f});
    lr.addSegment({1.0f, 0.0f, 1.0f}, {0.5f, 0.0f, 0.5f});
    lr.addSegment({0.5f, 0.0f, 0.0f}, {0.5f, 0.0f, 0.5f});
    // Z
    lr.addSegment({0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f});
    lr.addSegment({1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f});
    lr.addSegment({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f});

    Eigen::Matrix4f mv;
    // X
    mv = xModelView * labelTransformMatrix;
    lr.render({&projection, &mv}, {1.0f, 0.0f, 0.0f, opacity}, 4, 0);
    // Y
    mv = yModelView * labelTransformMatrix;
    lr.render({&projection, &mv}, {0.0f, 1.0f, 0.0f, opacity}, 6, 4);
    // Z
    mv = zModelView * labelTransformMatrix;
    lr.render({&projection, &mv}, {0.0f, 0.0f, 1.0f, opacity}, 6, 10);

    lr.finish();
}


/****** VelocityVectorArrow implementation ******/

VelocityVectorArrow::VelocityVectorArrow(const Body& _body) :
    ArrowReferenceMark(_body)
{
    setTag("velocity vector");
    setColor(Color(0.6f, 0.6f, 0.9f));
    setSize(body.getRadius() * 2.0f);
}

Eigen::Vector3d
VelocityVectorArrow::getDirection(double tdb) const
{
    const TimelinePhase* phase = body.getTimeline()->findPhase(tdb).get();
    return phase->orbitFrame()->getOrientation(tdb).conjugate() * phase->orbit()->velocityAtTime(tdb);
}


/****** SunDirectionArrow implementation ******/

SunDirectionArrow::SunDirectionArrow(const Body& _body) :
    ArrowReferenceMark(_body)
{
    setTag("sun direction");
    setColor(Color(1.0f, 1.0f, 0.4f));
    setSize(body.getRadius() * 2.0f);
}

Eigen::Vector3d
SunDirectionArrow::getDirection(double tdb) const
{
    const Body* b = &body;
    const Star* sun = nullptr;
    while (b != nullptr)
    {
        Selection center = b->getOrbitFrame(tdb)->getCenter();
        if (center.star() != nullptr)
            sun = center.star();
        b = center.body();
    }

    if (sun != nullptr)
        return sun->getPosition(tdb).offsetFromKm(body.getPosition(tdb));

    return Eigen::Vector3d::Zero();
}


/****** SpinVectorArrow implementation ******/

SpinVectorArrow::SpinVectorArrow(const Body& _body) :
    ArrowReferenceMark(_body)
{
    setTag("spin vector");
    setColor(Color(0.6f, 0.6f, 0.6f));
    setSize(body.getRadius() * 2.0f);
}

Eigen::Vector3d
SpinVectorArrow::getDirection(double tdb) const
{
    const TimelinePhase* phase = body.getTimeline()->findPhase(tdb).get();
    return phase->bodyFrame()->getOrientation(tdb).conjugate() * phase->rotationModel()->angularVelocityAtTime(tdb);
}


/****** BodyToBodyDirectionArrow implementation ******/

/*! Create a new body-to-body direction arrow pointing from the origin body toward
 *  the specified target object.
 */
BodyToBodyDirectionArrow::BodyToBodyDirectionArrow(const Body& _body, const Selection& _target) :
    ArrowReferenceMark(_body),
    target(_target)
{
    setTag("body to body");
    setColor(Color(0.0f, 0.5f, 0.0f));
    setSize(body.getRadius() * 2.0f);
}


Eigen::Vector3d
BodyToBodyDirectionArrow::getDirection(double tdb) const
{
    return target.getPosition(tdb).offsetFromKm(body.getPosition(tdb));
}


/****** BodyAxisArrows implementation ******/

BodyAxisArrows::BodyAxisArrows(const Body& _body) :
    AxesReferenceMark(_body)
{
    setTag("body axes");
    setOpacity(1.0);
    setSize(body.getRadius() * 2.0f);
}

Eigen::Quaterniond
BodyAxisArrows::getOrientation(double tdb) const
{
    return (math::YRot180<double> * body.getEclipticToBodyFixed(tdb)).conjugate();
}


/****** FrameAxisArrows implementation ******/

FrameAxisArrows::FrameAxisArrows(const Body& _body) :
    AxesReferenceMark(_body)
{
    setTag("frame axes");
    setOpacity(0.5);
    setSize(body.getRadius() * 2.0f);
}

Eigen::Quaterniond
FrameAxisArrows::getOrientation(double tdb) const
{
    return body.getEclipticToFrame(tdb).conjugate();
}
