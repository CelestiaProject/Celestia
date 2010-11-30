// sensorgeometry.cpp
//
// Copyright (C) 2010, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "sensorgeometry.h"
#include "rendcontext.h"
#include "texmanager.h"
#include "astro.h"
#include "body.h"
#include "vecgl.h"
#include "celmath/mathlib.h"
#include "celmath/intersect.h"
#include <Eigen/Core>
#include <algorithm>
#include <cassert>

using namespace Eigen;
using namespace std;


SensorGeometry::SensorGeometry() :
    m_observer(NULL),
    m_target(NULL),
    m_range(0.0),
    m_horizontalFov(degToRad(5.0)),
    m_verticalFov(degToRad(5.0)),
    m_frustumColor(1.0f, 1.0f, 1.0f),
    m_frustumOpacity(0.25f),
    m_gridOpacity(1.0f)
{
}


SensorGeometry::~SensorGeometry()
{
}


bool
SensorGeometry::pick(const Ray3d& /* r */, double& /* distance */) const
{
    return false;
}


void
SensorGeometry::setFOVs(double horizontalFov, double verticalFov)
{
    m_horizontalFov = horizontalFov;
    m_verticalFov = verticalFov;
}


/** Render the sensor geometry.
  */
void
SensorGeometry::render(RenderContext& rc, double tsec)
{
    if (m_target == NULL || m_observer == NULL)
    {
        return;
    }

    double jd = astro::secsToDays(tsec) + astro::J2000;

    UniversalCoord obsPos = m_observer->getPosition(jd);
    UniversalCoord targetPos = m_target->getPosition(jd);

    Vector3d pos = targetPos.offsetFromKm(obsPos);

    if (pos.norm() > m_range)
    {
        pos = pos.normalized() * m_range;
    }

    Quaterniond q = m_observer->getOrientation(jd);

    const unsigned int sectionCount = 40;
    const unsigned int sliceCount = 6;
    Vector3d profile[sectionCount];
    Vector3d footprint[sectionCount];

    Quaterniond obsOrientation = m_observer->getOrientation(jd).conjugate();
    Quaterniond targetOrientation = m_target->getOrientation(jd).conjugate();
    Vector3d origin = targetOrientation.conjugate() * -pos;
    Ellipsoidd targetEllipsoid(m_target->getSemiAxes().cast<double>());

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glDisable(GL_TEXTURE_2D);

    glPushMatrix();

    // 'Undo' the rotation of the parent body. We are assuming that the observer is
    // the body to which the sensor geometry is attached.
    glRotate(obsOrientation.conjugate());

    // Compute the profile of the frustum; the profile is extruded over the range
    // of the sensor (or to the intersection) when rendering.
    for (unsigned int i = 0; i < sectionCount; ++i)
    {
        double t = double(i) / double(sectionCount);
        double theta = t * PI * 2.0;

        profile[i] = obsOrientation * Vector3d(cos(theta) * m_horizontalFov, -sin(theta) * m_verticalFov, 1.0).normalized();
    }

    // Compute the 'footprint' of the sensor by finding the intersection of all rays with
    // the target body. The rendering will not be correct unless the sensor frustum
    for (unsigned int i = 0; i < sectionCount; ++i)
    {
        Vector3d direction = profile[i];
        Vector3d testDirection = targetOrientation.conjugate() * direction;

        double distance = 0.0;
        testIntersection(Ray3d(origin, testDirection), targetEllipsoid, distance);

        footprint[i] = distance * direction;
    }

    // Draw the frustum
    glColor4f(m_frustumColor.red(), m_frustumColor.green(), m_frustumColor.blue(), m_frustumOpacity);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3d(0.0, 0.0, 0.0);
    for (unsigned int i = 0; i < sectionCount; ++i)
    {
        glVertex3dv(footprint[i].data());
    }
    glVertex3dv(footprint[0].data());
    glEnd();

    glEnable(GL_LINE_SMOOTH);

    // Draw the footprint outline
    glColor4f(m_frustumColor.red(), m_frustumColor.green(), m_frustumColor.blue(), m_gridOpacity);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    for (unsigned int i = 0; i < sectionCount; ++i)
    {
        glVertex3dv(footprint[i].data());
    }
    glEnd();
    glLineWidth(1.0f);

    for (unsigned int slice = 1; slice < sliceCount; ++slice)
    {
        double t = double(slice) / double(sliceCount);

        glBegin(GL_LINE_LOOP);
        for (unsigned int i = 0; i < sectionCount; ++i)
        {
            Vector3d v = footprint[i] * t;
            glVertex3dv(v.data());
        }
        glEnd();
    }

    // NOTE: section count should be evenly divisible by 8
    unsigned int step = sectionCount / 8;
    glBegin(GL_LINES);
    for (unsigned int i = 0; i < sectionCount; i += step)
    {
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3dv(footprint[i].data());
    }
    glEnd();

    glPopMatrix();

    glEnable(GL_LIGHTING);
}


bool
SensorGeometry::isOpaque() const
{
    return false;
}


bool
SensorGeometry::isNormalized() const
{
    return false;
}
