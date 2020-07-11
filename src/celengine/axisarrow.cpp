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
#include <celmath/mathlib.h>
#include "axisarrow.h"
#include "body.h"
#include "frame.h"
#include "render.h"
#include "selection.h"
#include "shadermanager.h"
#include "timelinephase.h"
#include "vecgl.h"
#include "vertexobject.h"

using namespace Eigen;
using namespace std;
using namespace celestia;
using namespace celmath;
using namespace celgl;

// draw a simple circle or annulus
#define DRAW_ANNULUS 0

constexpr const float shaftLength  = 0.85f;
constexpr const float headLength   = 0.10f;
constexpr const float shaftRadius  = 0.010f;
constexpr const float headRadius   = 0.025f;
constexpr const unsigned nSections = 30;


static size_t initArrowAndLetters(VertexObject &vo)
{
    static_assert(sizeof(Vector3f) == 3*sizeof(GLfloat), "sizeof(Vector3f) != 3*sizeof(GLfloat)");
    static size_t count = 0; // number of vertices in the arrow

    vo.bind();
    if (vo.initialized())
        return count;

    // circle at bottom of a shaft
    vector<Vector3f> circle;
    // arrow shaft
    vector<Vector3f> shaft;
    // annulus
    vector<Vector3f> annulus;
    // head of the arrow
    vector<Vector3f> head;

    for (unsigned i = 0; i <= nSections; i++)
    {
        float c, s;
        sincos((i * 2.0f * (float)PI) / nSections, c, s);

        // circle at bottom
        Vector3f v0(shaftRadius * c, shaftRadius * s, 0.0f);
        if (i > 0)
            circle.push_back(v0);
        circle.push_back(Vector3f::Zero());
        circle.push_back(v0);

        // shaft
        Vector3f v1(shaftRadius * c, shaftRadius * s, shaftLength);
        Vector3f v1prev;
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
        Vector3f v2(headRadius * c, headRadius * s, shaftLength);
#if DRAW_ANNULUS
        Vector3f v2prev;
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
        Vector3f v3(0.0f, 0.0f, shaftLength);
        if (i > 0)
            annulus.push_back(v2);
        annulus.push_back(v2);
        annulus.push_back(v3);
#endif

        // head
        Vector3f v4(0.0f, 0.0f, shaftLength + headLength);
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


    GLfloat lettersVtx[] =
    {
        // X
        0,    0,    0,
        1,    0,    1,
        1,    0,    0,
        0,    0,    1,
        // Y
        0,    0,    1,
        0.5f, 0,    0.5f,
        1,    0,    1,
        0.5f, 0,    0.5f,
        0.5f, 0,    0,
        0.5f, 0,    0.5f,
        // Z
        0,    0,    1,
        1,    0,    1,
        0,    0,    0,
        1,    0,    0
    };

    GLintptr offset = 0;
    count = circle.size() + shaft.size() + annulus.size() + head.size();
    GLsizeiptr size = sizeof(lettersVtx) + count * sizeof(GLfloat) * 3;
    GLsizeiptr s = 0;

    vo.allocate(size);

    s = circle.size() * sizeof(Vector3f);
    vo.setBufferData(circle.data(), offset, s);
    offset += s;

    s = shaft.size() * sizeof(Vector3f);
    vo.setBufferData(shaft.data(), offset, s);
    offset += s;

    s = annulus.size() * sizeof(Vector3f);
    vo.setBufferData(annulus.data(), offset, s);
    offset += s;

    s = head.size() * sizeof(Vector3f);
    vo.setBufferData(head.data(), offset, s);
    offset += s;

    vo.setBufferData(lettersVtx, offset, sizeof(lettersVtx));

    vo.setVertices(3, GL_FLOAT, GL_FALSE, 0, 0);

    return count;
}

static void RenderArrow(VertexObject& vo)
{
    auto count = initArrowAndLetters(vo);
    vo.draw(GL_TRIANGLES, count);
    vo.unbind();
}

// Draw letter x in xz plane
static void RenderX(VertexObject& vo)
{
    auto offset = initArrowAndLetters(vo);
    vo.draw(GL_LINES, 4, offset);
    vo.unbind();
}


// Draw letter y in xz plane
static void RenderY(VertexObject& vo)
{
    auto offset = initArrowAndLetters(vo);
    vo.draw(GL_LINES, 6, offset+4);
    vo.unbind();
}


// Draw letter z in xz plane
static void RenderZ(VertexObject& vo)
{
    auto offset = initArrowAndLetters(vo);
    vo.draw(GL_LINE_STRIP, 4, offset+10);
    vo.unbind();
}


/****** ArrowReferenceMark base class ******/

ArrowReferenceMark::ArrowReferenceMark(const Body& _body) :
    body(_body),
    size(1.0),
    color(1.0f, 1.0f, 1.0f),
#ifdef USE_HDR
    opacity(0.0f)
#else
    opacity(1.0f)
#endif
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
ArrowReferenceMark::setColor(Color _color)
{
    color = _color;
}


void
ArrowReferenceMark::render(Renderer* renderer,
                           const Vector3f& position,
                           float /* discSize */,
                           double tdb,
                           const Matrices& m) const
{
    Vector3d v = getDirection(tdb);
    if (v.norm() < 1.0e-12)
    {
        // Skip rendering of zero-length vectors
        return;
    }

    v.normalize();
    Quaterniond q;
    q.setFromTwoVectors(Vector3d::UnitZ(), v);

    if (opacity == 1.0f)
    {
        // Enable depth buffering
        renderer->enableDepthTest();
        renderer->enableDepthMask();
        renderer->disableBlending();
    }
    else
    {
        renderer->enableDepthTest();
        renderer->disableDepthMask();
        renderer->enableBlending();
#ifdef USE_HDR
        renderer->setBlendingFactors(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
#else
        renderer->setBlendingFactors(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
    }

    Affine3f transform = Translation3f(position) * q.cast<float>() * Scaling(size);
    Matrix4f mv = (*m.modelview) * transform.matrix();

    CelestiaGLProgram* prog = renderer->getShaderManager().getShader(shadprop);
    if (prog == nullptr)
        return;
    prog->use();
    prog->MVPMatrix = (*m.projection) * mv;

    glVertexAttrib4f(CelestiaGLProgram::ColorAttributeIndex,
		     color.red(), color.green(), color.blue(), opacity);

    auto &vo = renderer->getVertexObject(VOType::AxisArrow, GL_ARRAY_BUFFER, 0, GL_STATIC_DRAW);
    RenderArrow(vo);

    renderer->disableDepthTest();
    renderer->disableDepthMask();
    renderer->enableBlending();
    renderer->setBlendingFactors(GL_SRC_ALPHA, GL_ONE);
}


/****** AxesReferenceMark base class ******/

AxesReferenceMark::AxesReferenceMark(const Body& _body) :
    body(_body),
    size(),
#ifdef USE_HDR
    opacity(0.0f)
#else
    opacity(1.0f)
#endif
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
#ifdef USE_HDR
    opacity = 1.0f - opacity;
#endif
}


void
AxesReferenceMark::render(Renderer* renderer,
                          const Vector3f& position,
                          float /* discSize */,
                          double tdb,
                          const Matrices& m) const
{
    Quaterniond q = getOrientation(tdb);

    if (opacity == 1.0f)
    {
        // Enable depth buffering
        renderer->enableDepthTest();
        renderer->enableDepthMask();
        renderer->disableBlending();
    }
    else
    {
        renderer->enableDepthTest();
        renderer->disableDepthMask();
        renderer->enableBlending();
#ifdef USE_HDR
        renderer->setBlendingFactors(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
#else
        renderer->setBlendingFactors(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
    }

    Affine3f transform = Translation3f(position) * q.cast<float>() * Scaling(size);
    Matrix4f mvp = (*m.projection) * (*m.modelview) * transform.matrix();

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

    auto &vo = renderer->getVertexObject(VOType::AxisArrow, GL_ARRAY_BUFFER, 0, GL_STATIC_DRAW);

    Affine3f labelTransform = Translation3f(Vector3f(0.1f, 0.0f, 0.75f)) * Scaling(labelScale);
    Matrix4f labelTransformMatrix = labelTransform.matrix();
    Matrix4f t;

    // x-axis
    t = mvp * vecgl::rotate(AngleAxisf(90.0_deg, Vector3f::UnitY()));
    glVertexAttrib4f(CelestiaGLProgram::ColorAttributeIndex, 1.0f, 0.0f, 0.0f, opacity);
    prog->MVPMatrix = t;
    RenderArrow(vo);
    prog->MVPMatrix = t * labelTransformMatrix;
    RenderX(vo);

    // y-axis
    t = mvp * vecgl::rotate(AngleAxisf(180.0_deg, Vector3f::UnitY()));
    glVertexAttrib4f(CelestiaGLProgram::ColorAttributeIndex, 0.0f, 1.0f, 0.0f, opacity);
    prog->MVPMatrix = t;
    RenderArrow(vo);
    prog->MVPMatrix = t * labelTransformMatrix;
    RenderY(vo);

    // z-axis
    t = mvp *vecgl::rotate(AngleAxisf(-90.0_deg, Vector3f::UnitX()));
    glVertexAttrib4f(CelestiaGLProgram::ColorAttributeIndex, 0.0f, 0.0f, 1.0f, opacity);
    prog->MVPMatrix = t;
    RenderArrow(vo);
    prog->MVPMatrix = t * labelTransformMatrix;
    RenderZ(vo);

    renderer->disableDepthTest();
    renderer->disableDepthMask();
    renderer->enableBlending();
    renderer->setBlendingFactors(GL_SRC_ALPHA, GL_ONE);
}


/****** VelocityVectorArrow implementation ******/

VelocityVectorArrow::VelocityVectorArrow(const Body& _body) :
    ArrowReferenceMark(_body)
{
    setTag("velocity vector");
    setColor(Color(0.6f, 0.6f, 0.9f));
    setSize(body.getRadius() * 2.0f);
}

Vector3d
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

Vector3d
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

    return Vector3d::Zero();
}


/****** SpinVectorArrow implementation ******/

SpinVectorArrow::SpinVectorArrow(const Body& _body) :
    ArrowReferenceMark(_body)
{
    setTag("spin vector");
    setColor(Color(0.6f, 0.6f, 0.6f));
    setSize(body.getRadius() * 2.0f);
}

Vector3d
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


Vector3d
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

Quaterniond
BodyAxisArrows::getOrientation(double tdb) const
{
    return (Quaterniond(AngleAxis<double>(PI, Vector3d::UnitY())) * body.getEclipticToBodyFixed(tdb)).conjugate();
}


/****** FrameAxisArrows implementation ******/

FrameAxisArrows::FrameAxisArrows(const Body& _body) :
    AxesReferenceMark(_body)
{
    setTag("frame axes");
    setOpacity(0.5);
    setSize(body.getRadius() * 2.0f);
}

Quaterniond
FrameAxisArrows::getOrientation(double tdb) const
{
    return body.getEclipticToFrame(tdb).conjugate();
}
