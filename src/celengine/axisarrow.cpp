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
#include <vector>
#include <celcompat/numbers.h>
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
#include "timelinephase.h"

using celestia::render::LineRenderer;
namespace gl = celestia::gl;
namespace util = celestia::util;

// draw a simple circle or annulus
#define DRAW_ANNULUS 0

constexpr float shaftLength  = 0.85f;
constexpr float headLength   = 0.10f;
constexpr float shaftRadius  = 0.010f;
constexpr float headRadius   = 0.025f;
constexpr unsigned nSections = 30;

namespace
{

gl::VertexObject&
GetArrowVAO()
{
    static bool initialized = false;

    static std::unique_ptr<gl::VertexObject> vo;
    static std::unique_ptr<gl::Buffer> bo;

    if (initialized)
        return *vo;

    initialized = true;

    vo = std::make_unique<gl::VertexObject>();
    bo = std::make_unique<gl::Buffer>();

    // circle at bottom of a shaft
    std::vector<Eigen::Vector3f> circle;
    // arrow shaft
    std::vector<Eigen::Vector3f> shaft;
    // annulus
    std::vector<Eigen::Vector3f> annulus;
    // head of the arrow
    std::vector<Eigen::Vector3f> head;

    for (unsigned i = 0; i <= nSections; i++)
    {
        float c, s;
        celmath::sincos((i * 2.0f * celestia::numbers::pi_v<float>) / nSections, c, s);

        // circle at bottom
        Eigen::Vector3f v0(shaftRadius * c, shaftRadius * s, 0.0f);
        if (i > 0)
            circle.push_back(v0);
        circle.push_back(Eigen::Vector3f::Zero());
        circle.push_back(v0);

        // shaft
        Eigen::Vector3f v1(shaftRadius * c, shaftRadius * s, shaftLength);
        Eigen::Vector3f v1prev;
        if (i > 0)
        {
            shaft.push_back(v0); // left triangle

            shaft.push_back(v0); // right
            shaft.push_back(v1prev);
            shaft.push_back(v1);
        }
        shaft.push_back(v0); // left
        shaft.push_back(v1);
        v1prev = v1;

        // annulus
        Eigen::Vector3f v2(headRadius * c, headRadius * s, shaftLength);
#if DRAW_ANNULUS
        Eigen::Vector3f v2prev;
        if (i > 0)
        {
            annulus.push_back(v2);

            annulus.push_back(v2);
            annulus.push_back(v2prev);
            annulus.push_back(v1);
        }
        annulus.push_back(v2);
        annulus.push_back(v1);
        v2prev = v1;
#else
        Eigen::Vector3f v3(0.0f, 0.0f, shaftLength);
        if (i > 0)
            annulus.push_back(v2);
        annulus.push_back(v2);
        annulus.push_back(v3);
#endif

        // head
        Eigen::Vector3f v4(0.0f, 0.0f, shaftLength + headLength);
        if (i > 0)
            head.push_back(v2);
        head.push_back(v4);
        head.push_back(v2);
    }

    circle.push_back(circle[1]);
    shaft.push_back(shaft[0]);
#if DRAW_ANNULUS
    annulus.push_back(annulus[0]);
#else
    annulus.push_back(annulus[1]);
#endif
    head.push_back(head[1]);

    std::vector<Eigen::Vector3f> arrow;
    arrow.reserve(circle.size() + shaft.size() + annulus.size() + head.size());
    std::copy(circle.begin(), circle.end(), std::back_inserter(arrow));
    std::copy(shaft.begin(), shaft.end(), std::back_inserter(arrow));
    std::copy(annulus.begin(), annulus.end(), std::back_inserter(arrow));
    std::copy(head.begin(), head.end(), std::back_inserter(arrow));

    bo->bind().setData(arrow, gl::Buffer::BufferUsage::StaticDraw);

    vo->setCount(static_cast<int>(arrow.size())).addVertexBuffer(
        *bo,
        CelestiaGLProgram::VertexCoordAttributeIndex,
        3,
        gl::VertexObject::DataType::Float);

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
    shadprop.texUsage = ShaderProperties::VertexColors;
    shadprop.lightModel = ShaderProperties::UnlitModel;
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
    shadprop.texUsage = ShaderProperties::VertexColors;
    shadprop.lightModel = ShaderProperties::UnlitModel;
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
    Eigen::Matrix4f xModelView = modelView * celmath::rotate(Eigen::AngleAxisf(90.0_deg, Eigen::Vector3f::UnitY()));
    glVertexAttrib4f(CelestiaGLProgram::ColorAttributeIndex, 1.0f, 0.0f, 0.0f, opacity);
    prog->setMVPMatrices(projection, xModelView);
    GetArrowVAO().draw();

    // y-axis
    Eigen::Matrix4f yModelView = modelView * celmath::rotate(Eigen::AngleAxisf(180.0_deg, Eigen::Vector3f::UnitY()));
    glVertexAttrib4f(CelestiaGLProgram::ColorAttributeIndex, 0.0f, 1.0f, 0.0f, opacity);
    prog->setMVPMatrices(projection, yModelView);
    GetArrowVAO().draw();

    // z-axis
    Eigen::Matrix4f zModelView = modelView * celmath::rotate(Eigen::AngleAxisf(-90.0_deg, Eigen::Vector3f::UnitX()));
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
    auto phase = body.getTimeline()->findPhase(tdb);
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
    Star* sun = nullptr;
    while (b != nullptr)
    {
        Selection center = b->getOrbitFrame(tdb)->getCenter();
        if (center.star() != nullptr)
            sun = center.star();
        b = center.body();
    }

    if (sun != nullptr)
        return Selection(sun).getPosition(tdb).offsetFromKm(body.getPosition(tdb));

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
    auto phase = body.getTimeline()->findPhase(tdb);
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
    return (Eigen::Quaterniond(Eigen::AngleAxis<double>(celestia::numbers::pi, Eigen::Vector3d::UnitY())) * body.getEclipticToBodyFixed(tdb)).conjugate();
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
