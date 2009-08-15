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
#include <celmath/mathlib.h>
#include <GL/glew.h>
#include "vecgl.h"
#include "axisarrow.h"
#include "selection.h"
#include "frame.h"
#include "body.h"
#include "timelinephase.h"

using namespace Eigen;
using namespace std;


static const unsigned int MaxArrowSections = 100;

static void RenderArrow(float shaftLength,
                        float headLength,
                        float shaftRadius,
                        float headRadius,
                        unsigned int nSections)
{
    float sintab[MaxArrowSections];
    float costab[MaxArrowSections];
	
    unsigned int i;

    nSections = min(MaxArrowSections, nSections);
	
    // Initialize the trig tables
    for (i = 0; i < nSections; i++)
    {
        double theta = (i * 2.0 * PI) / nSections;
        sintab[i] = (float) sin(theta);
        costab[i] = (float) cos(theta);
    }
	
    // Render the circle at the botton of the arrow shaft
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.0f, 0.0f, 0.0f);
    for (i = 0; i <= nSections; i++)
    {
        unsigned int n = (nSections - i) % nSections;
        glVertex3f(shaftRadius * costab[n], shaftRadius * sintab[n], 0.0f);
    }
    glEnd();
	
    // Render the arrow shaft
    glBegin(GL_QUAD_STRIP);
    for (i = 0; i <= nSections; i++)
    {
        unsigned int n = i % nSections;
        glVertex3f(shaftRadius * costab[n], shaftRadius * sintab[n], shaftLength);
        glVertex3f(shaftRadius * costab[n], shaftRadius * sintab[n], 0.0f);
    }
    glEnd();
	
    // Render the annulus
    glBegin(GL_QUAD_STRIP);
    for (i = 0; i <= nSections; i++)
    {
        unsigned int n = i % nSections;
        glVertex3f(headRadius * costab[n],  headRadius * sintab[n], shaftLength);		
        glVertex3f(shaftRadius * costab[n], shaftRadius * sintab[n], shaftLength);
    }
    glEnd();
	
    // Render the head of the arrow
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.0f, 0.0f, shaftLength + headLength);
    for (i = 0; i <= nSections; i++)
    {
        unsigned int n = i % nSections;
        glVertex3f(headRadius * costab[n], headRadius * sintab[n], shaftLength);
    }
    glEnd();
}


// Draw letter x in xz plane
static void RenderX()
{
    glBegin(GL_LINES);
    glVertex3f(0, 0, 0);
    glVertex3f(1, 0, 1);
    glVertex3f(1, 0, 0);
    glVertex3f(0, 0, 1);
    glEnd();
}


// Draw letter y in xz plane
static void RenderY()
{
    glBegin(GL_LINES);
    glVertex3f(0, 0, 1);
    glVertex3f(0.5f, 0, 0.5f);
    glVertex3f(1, 0, 1);
    glVertex3f(0.5f, 0, 0.5f);
    glVertex3f(0.5f, 0, 0);
    glVertex3f(0.5f, 0, 0.5f);
    glEnd();
}


// Draw letter z in xz plane
static void RenderZ()
{
    glBegin(GL_LINE_STRIP);
    glVertex3f(0, 0, 1);
    glVertex3f(1, 0, 1);
    glVertex3f(0, 0, 0);
    glVertex3f(1, 0, 0);
    glEnd();
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
ArrowReferenceMark::render(Renderer* /* renderer */,
                           const Vector3f& /* position */,
                           float /* discSize */,
                           double tdb) const
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
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
    else
    {
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
#ifdef USE_HDR
        glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
#else
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
    }

    glPushMatrix();
    glRotate(q.cast<float>());
    glScalef(size, size, size);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    float shaftLength = 0.85f;
    float headLength = 0.10f;
    float shaftRadius = 0.010f;
    float headRadius = 0.025f;
    unsigned int nSections = 30;
	
    glColor4f(color.red(), color.green(), color.blue(), opacity);
    RenderArrow(shaftLength, headLength, shaftRadius, headRadius, nSections);

    glPopMatrix();

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
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
AxesReferenceMark::render(Renderer* /* renderer */,
                          const Vector3f& /* position */,
                          float /* discSize */,
                          double tdb) const
{
    Quaterniond q = getOrientation(tdb);

    if (opacity == 1.0f)
    {
        // Enable depth buffering
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
    else
    {
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
#ifdef USE_HDR
        glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
#else
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
    }

    glDisable(GL_TEXTURE_2D);

    glPushMatrix();
    glRotate(q.cast<float>());
    glScalef(size, size, size);
	
    glDisable(GL_LIGHTING);

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

    float shaftLength = 0.85f;
    float headLength = 0.10f;
    float shaftRadius = 0.010f;
    float headRadius = 0.025f;
    unsigned int nSections = 30;
    float labelScale = 0.1f;
	
    // x-axis
    glPushMatrix();
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glColor4f(1.0f, 0.0f, 0.0f, opacity);
    RenderArrow(shaftLength, headLength, shaftRadius, headRadius, nSections);
    glTranslatef(0.1f, 0.0f, 0.75f);
    glScalef(labelScale, labelScale, labelScale);
    RenderX();
    glPopMatrix();

    // y-axis
    glPushMatrix();
    glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
    glColor4f(0.0f, 1.0f, 0.0f, opacity);
    RenderArrow(shaftLength, headLength, shaftRadius, headRadius, nSections);
    glTranslatef(0.1f, 0.0f, 0.75f);
    glScalef(labelScale, labelScale, labelScale);
    RenderY();
    glPopMatrix();

    // z-axis
    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    glColor4f(0.0f, 0.0f, 1.0f, opacity);
    RenderArrow(shaftLength, headLength, shaftRadius, headRadius, nSections);
    glTranslatef(0.1f, 0.0f, 0.75f);
    glScalef(labelScale, labelScale, labelScale);
    RenderZ();
    glPopMatrix();
	
    glPopMatrix();

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
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
    const TimelinePhase* phase = body.getTimeline()->findPhase(tdb);
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
    Star* sun = NULL;
    while (b != NULL)
    {
        Selection center = b->getOrbitFrame(tdb)->getCenter();
        if (center.star() != NULL)
            sun = center.star();
        b = center.body();
    }

    if (sun != NULL)
    {
        return Selection(sun).getPosition(tdb).offsetFromKm(body.getPosition(tdb));
    }
    else
    {
        return Vector3d::Zero();
    }
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
    const TimelinePhase* phase = body.getTimeline()->findPhase(tdb);
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
