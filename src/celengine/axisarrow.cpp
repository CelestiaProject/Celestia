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
#include <celrender/referencemarkrenderer.h>
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

using namespace std::string_view_literals;

using celestia::render::LineRenderer;
namespace gl = celestia::gl;
namespace math = celestia::math;
namespace render = celestia::render;

/****** ArrowReferenceMark base class ******/

ArrowReferenceMark::ArrowReferenceMark(const Body& _body) :
    body(_body)
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
ArrowReferenceMark::render(render::ReferenceMarkRenderer* refMarkRenderer,
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
    ps.depthMask = true;

    refMarkRenderer->renderer().setPipelineState(ps);

    Eigen::Affine3f transform = Eigen::Translation3f(position) * q.cast<float>() * Eigen::Scaling(size);
    Eigen::Matrix4f mv = (*m.modelview) * transform.matrix();

    const render::ArrowRenderer& arrowRenderer = refMarkRenderer->arrowRenderer();
    CelestiaGLProgram* prog = arrowRenderer.program();
    if (prog == nullptr)
        return;
    prog->use();
    prog->setMVPMatrices(*m.projection, mv);

    glVertexAttrib4f(CelestiaGLProgram::ColorAttributeIndex,
		             color.red(), color.green(), color.blue(), 1.0f);

    arrowRenderer.vertexObject()->draw();
}


/****** AxesReferenceMark base class ******/

AxesReferenceMark::AxesReferenceMark(const Body& _body) :
    body(_body)
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
AxesReferenceMark::render(render::ReferenceMarkRenderer* refMarkRenderer,
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
    refMarkRenderer->renderer().setPipelineState(ps);

    Eigen::Affine3f transform = Eigen::Translation3f(position) * q.cast<float>() * Eigen::Scaling(size);
    Eigen::Matrix4f projection = *m.projection;
    Eigen::Matrix4f modelView = (*m.modelview) * transform.matrix();

    float labelScale = 0.1f;

    render::ArrowRenderer& arrowRenderer = refMarkRenderer->arrowRenderer();
    CelestiaGLProgram* prog = arrowRenderer.program();
    if (prog == nullptr)
        return;
    prog->use();

    Eigen::Affine3f labelTransform = Eigen::Translation3f(Eigen::Vector3f(0.1f, 0.0f, 0.75f)) * Eigen::Scaling(labelScale);
    Eigen::Matrix4f labelTransformMatrix = labelTransform.matrix();

    // x-axis
    Eigen::Matrix4f xModelView = modelView * math::YRot90Matrix<float>;
    glVertexAttrib4f(CelestiaGLProgram::ColorAttributeIndex, 1.0f, 0.0f, 0.0f, opacity);
    prog->setMVPMatrices(projection, xModelView);
    arrowRenderer.vertexObject()->draw();

    // y-axis
    Eigen::Matrix4f yModelView = modelView * math::YRot180Matrix<float>;
    glVertexAttrib4f(CelestiaGLProgram::ColorAttributeIndex, 0.0f, 1.0f, 0.0f, opacity);
    prog->setMVPMatrices(projection, yModelView);
    arrowRenderer.vertexObject()->draw();

    // z-axis
    Eigen::Matrix4f zModelView = modelView * math::XRot270Matrix<float>;
    glVertexAttrib4f(CelestiaGLProgram::ColorAttributeIndex, 0.0f, 0.0f, 1.0f, opacity);
    prog->setMVPMatrices(projection, zModelView);
    arrowRenderer.vertexObject()->draw();

    LineRenderer& lr = arrowRenderer.lineRenderer();
    lr.clear();
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
    setColor(Color(0.6f, 0.6f, 0.9f));
    setSize(body.getRadius() * 2.0f);
}

Eigen::Vector3d
VelocityVectorArrow::getDirection(double tdb) const
{
    const TimelinePhase& phase = body.getTimeline()->findPhase(tdb);
    return phase.orbitFrame()->getOrientation(tdb).conjugate() * phase.orbit()->velocityAtTime(tdb);
}

std::string_view
VelocityVectorArrow::defaultTag() const
{
    return "velocity vector"sv;
}


/****** SunDirectionArrow implementation ******/

SunDirectionArrow::SunDirectionArrow(const Body& _body) :
    ArrowReferenceMark(_body)
{
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

std::string_view
SunDirectionArrow::defaultTag() const
{
    return "sun direction"sv;
}


/****** SpinVectorArrow implementation ******/

SpinVectorArrow::SpinVectorArrow(const Body& _body) :
    ArrowReferenceMark(_body)
{
    setColor(Color(0.6f, 0.6f, 0.6f));
    setSize(body.getRadius() * 2.0f);
}

Eigen::Vector3d
SpinVectorArrow::getDirection(double tdb) const
{
    const TimelinePhase& phase = body.getTimeline()->findPhase(tdb);
    return phase.bodyFrame()->getOrientation(tdb).conjugate() * phase.rotationModel()->angularVelocityAtTime(tdb);
}

std::string_view
SpinVectorArrow::defaultTag() const
{
    return "spin vector"sv;
}


/****** BodyToBodyDirectionArrow implementation ******/

/*! Create a new body-to-body direction arrow pointing from the origin body toward
 *  the specified target object.
 */
BodyToBodyDirectionArrow::BodyToBodyDirectionArrow(const Body& _body, const Selection& _target) :
    ArrowReferenceMark(_body),
    target(_target)
{
    setColor(Color(0.0f, 0.5f, 0.0f));
    setSize(body.getRadius() * 2.0f);
}

Eigen::Vector3d
BodyToBodyDirectionArrow::getDirection(double tdb) const
{
    return target.getPosition(tdb).offsetFromKm(body.getPosition(tdb));
}

std::string_view
BodyToBodyDirectionArrow::defaultTag() const
{
    return "body to body"sv;
}


/****** BodyAxisArrows implementation ******/

BodyAxisArrows::BodyAxisArrows(const Body& _body) :
    AxesReferenceMark(_body)
{
    setOpacity(1.0);
    setSize(body.getRadius() * 2.0f);
}

Eigen::Quaterniond
BodyAxisArrows::getOrientation(double tdb) const
{
    return (math::YRot180<double> * body.getEclipticToBodyFixed(tdb)).conjugate();
}

std::string_view
BodyAxisArrows::defaultTag() const
{
    return "body axes"sv;
}


/****** FrameAxisArrows implementation ******/

FrameAxisArrows::FrameAxisArrows(const Body& _body) :
    AxesReferenceMark(_body)
{
    setOpacity(0.5);
    setSize(body.getRadius() * 2.0f);
}

Eigen::Quaterniond
FrameAxisArrows::getOrientation(double tdb) const
{
    return body.getEclipticToFrame(tdb).conjugate();
}

std::string_view
FrameAxisArrows::defaultTag() const
{
    return "frame axes"sv;
}
