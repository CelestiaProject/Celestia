// axisarrow.cpp
//
// Copyright (C) 2007, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <celmath/mathlib.h>
#include "gl.h"
#include "vecgl.h"
#include "axisarrow.h"

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


void RenderAxisArrows(const Quatf& orientation, float scale, float opacity)
{
    glPushMatrix();
    glRotate(orientation);
    glScalef(scale, scale, scale);
	
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
}


void RenderSunDirectionArrow(const Vec3f& direction, float scale, float opacity)
{
    glPushMatrix();
    glRotate(Quatf::vecToVecRotation(Vec3f(0.0f, 0.0f, 1.0f), direction));
    glScalef(scale, scale, scale);
	
    glDisable(GL_LIGHTING);

    float shaftLength = 0.85f;
    float headLength = 0.10f;
    float shaftRadius = 0.010f;
    float headRadius = 0.025f;
    unsigned int nSections = 30;
    float labelScale = 0.1f;
	
    glColor4f(1.0f, 1.0f, 0.0f, opacity);
    RenderArrow(shaftLength, headLength, shaftRadius, headRadius, nSections);
	
    glPopMatrix();
}


void RenderVelocityArrow(const Vec3f& direction, float scale, float opacity)
{
    glPushMatrix();
    glRotate(Quatf::vecToVecRotation(Vec3f(0.0f, 0.0f, 1.0f), direction));
    glScalef(scale, scale, scale);
	
    glDisable(GL_LIGHTING);

    float shaftLength = 0.85f;
    float headLength = 0.10f;
    float shaftRadius = 0.010f;
    float headRadius = 0.025f;
    unsigned int nSections = 30;
    float labelScale = 0.1f;
	
    glColor4f(0.6f, 0.6f, 0.9f, opacity);
    RenderArrow(shaftLength, headLength, shaftRadius, headRadius, nSections);
	
    glPopMatrix();
}
