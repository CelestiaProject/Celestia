// visibleregion.cpp
//
// Visible region reference mark for ellipsoidal bodies.
//
// Copyright (C) 2008, the Celestia Development Team
// Initial version by Chris Laurel, claurel@gmail.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstdio>
#include <cmath>
#include <celmath/intersect.h>
#include "visibleregion.h"
#include "body.h"
#include "selection.h"
#include "gl.h"
#include "vecgl.h"
#include "render.h"



/*! Construct a new reference mark that shows the outline of the
 *  region on the surface of a body in which the target object is
 *  visible. The following are assumed:
 *     - target is a point
 *     - the body is an ellipsoid
 *  
 *  This reference mark is useful in a few situations. When the
 *  body is a planet or moon and target is the sun, the outline of
 *  the visible region is the terminator. If target is a satellite,
 *  the outline is its circle of visibility.
 */
VisibleRegion::VisibleRegion(const Body& body, const Selection& target) :
    m_body(body),
    m_target(target),
    m_color(1.0f, 1.0f, 0.0f),
#ifdef USE_HDR
    m_opacity(0.0f)
#else
    m_opacity(1.0f)
#endif
{
    setTag("visible region");
}


VisibleRegion::~VisibleRegion()
{
}


Color
VisibleRegion::color() const
{
    return m_color;
}


void
VisibleRegion::setColor(Color color)
{
    m_color = color;
}


float
VisibleRegion::opacity() const
{
    return m_opacity;
}


void
VisibleRegion::setOpacity(float opacity)
{
    m_opacity = opacity;
#ifdef USE_HDR
    m_opacity = 1.0f - opacity;
#endif
}


template <typename T> static Vector3<T>
ellipsoidTangent(const Vector3<T>& recipSemiAxes,
                 const Vector3<T>& w,
                 const Vector3<T>& e,
                 const Vector3<T>& e_,
                 T ee)
{
    // We want to find t such that -E(1-t) + Wt is the direction of a ray
    // tangent to the ellipsoid.  A tangent ray will intersect the ellipsoid
    // at exactly one point.  Finding the intersection between a ray and an
    // ellipsoid ultimately requires using the quadratic formula, which has
    // one solution when the discriminant (b^2 - 4ac) is zero.  The code below
    // computes the value of t that results in a discriminant of zero.
    Vector3<T> w_(w.x * recipSemiAxes.x, w.y * recipSemiAxes.y, w.z * recipSemiAxes.z);
    T ww = w_ * w_;
    T ew = w_ * e_;

    // Before elimination of terms:
    // double a =  4 * square(ee + ew) - 4 * (ee + 2 * ew + ww) * (ee - 1.0f);
    // double b = -8 * ee * (ee + ew)  - 4 * (-2 * (ee + ew) * (ee - 1.0f));
    // double c =  4 * ee * ee         - 4 * (ee * (ee - 1.0f));

    // Simplify the below expression and eliminate the ee^2 terms; this
    // prevents precision errors, as ee tends to be a very large value.
    //T a =  4 * square(ee + ew) - 4 * (ee + 2 * ew + ww) * (ee - 1);
    T a =  4 * (square(ew) - ee * ww + ee + 2 * ew + ww);
    T b = -8 * (ee + ew);
    T c =  4 * ee;

    T t = 0;
    T discriminant = b * b - 4 * a * c;

    if (discriminant < 0)
        t = (-b + (T) sqrt(-discriminant)) / (2 * a); // Bad!
    else
        t = (-b + (T) sqrt(discriminant)) / (2 * a);

    // V is the direction vector.  We now need the point of intersection,
    // which we obtain by solving the quadratic equation for the ray-ellipse
    // intersection.  Since we already know that the discriminant is zero,
    // the solution is just -b/2a
    Vector3<T> v = -e * (1 - t) + w * t;
    Vector3<T> v_(v.x * recipSemiAxes.x, v.y * recipSemiAxes.y, v.z * recipSemiAxes.z);
    T a1 = v_ * v_;
    T b1 = (T) 2 * v_ * e_;
    T t1 = -b1 / (2 * a1);

    return e + v * t1;
}


// Get a vector orthogonal to the specified one
template <typename T> static Vector3<T>
orthogonalUnitVector(const Vector3<T>& v)
{
    Vec3d w;

    if (abs(v.x) < abs(v.y) && abs(v.x) < abs(v.z))
        w = Vec3d(1, 0, 0) ^ v;
    else if (abs(v.y) < abs(v.z))
        w = Vec3d(0, 1, 0) ^ v;
    else
        w = Vec3d(0, 0, 1) ^ v;

    w.normalize();

    return w;
}


void
VisibleRegion::render(Renderer* /* renderer */,
                      const Point3f& /* pos */,
                      float discSizeInPixels,
                      double tdb) const
{
    // Don't render anything if the current time is not within the
    // target object's time window.
    if (m_target.body() != NULL)
    {
        if (!m_target.body()->extant(tdb))
            return;
    }

    // Fade in the terminator when the planet is small
    const float minDiscSize = 5.0f;
    const float fullOpacityDiscSize = 10.0f;
    float opacity = (discSizeInPixels - minDiscSize) / (fullOpacityDiscSize - minDiscSize);

    // Don't render the terminator if the it's smaller than the minimum size
    if (opacity <= 0.0f)
        return;
    opacity = min(opacity, 1.0f) * m_opacity;

    // Base the amount of subdivision on the apparent size
    unsigned int nSections = (unsigned int) (30.0f + discSizeInPixels * 0.5f);
    nSections = min(nSections, 360u);

    Quatd q = m_body.getEclipticToBodyFixed(tdb);
    Quatf qf((float) q.w, (float) q.x, (float) q.y, (float) q.z);

    // The outline can't be rendered exactly on the planet sphere, or
    // there will be z-fighting problems. Render it at a height above the
    // planet that will place it about one pixel away from the planet.
    float scale = (discSizeInPixels + 1) / discSizeInPixels;
    scale = max(scale, 1.0001f);

    Vec3f semiAxes = m_body.getSemiAxes();

    // Enable depth buffering
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_BLEND);
#ifdef USE_HDR
    glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
#else
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    glPushMatrix();
    glRotate(~qf);

    double maxSemiAxis = m_body.getRadius();

    // In order to avoid precision problems and extremely large values, scale
    // the target position and semiaxes such that the largest semiaxis is 1.0.
    Vec3d lightDir = m_body.getPosition(tdb) - m_target.getPosition(tdb);
    lightDir = lightDir * (astro::microLightYearsToKilometers(1.0) / maxSemiAxis);
    lightDir = lightDir * (~q).toMatrix3();

    // Another measure to prevent precision problems: if the distance to the
    // object is much greater than the largest semi axis, clamp it to 1e4 times
    // the radius, as body-to-target rays at that distance are nearly parallel anyhow.
    if (lightDir.length() > 10000.0)
        lightDir *= (10000.0 / lightDir.length());

    // Pick two orthogonal axes both normal to the light direction
    Vec3d lightDirNorm = lightDir;
    lightDirNorm.normalize();

    Vec3d uAxis = orthogonalUnitVector(lightDirNorm);
    Vec3d vAxis = uAxis ^ lightDirNorm;

    Vec3d recipSemiAxes(maxSemiAxis / semiAxes.x, maxSemiAxis / semiAxes.y, maxSemiAxis / semiAxes.z);
    Vec3d e = -lightDir;
    Vec3d e_(e.x * recipSemiAxes.x, e.y * recipSemiAxes.y, e.z * recipSemiAxes.z);
    double ee = e_ * e_;

    glColor4f(m_color.red(), m_color.green(), m_color.blue(), opacity);
    glBegin(GL_LINE_LOOP);

    for (unsigned i = 0; i < nSections; i++)
    {
        double theta = (double) i / (double) (nSections) * 2.0 * PI;
        Vec3d w = cos(theta) * uAxis + sin(theta) * vAxis;
        
        Vec3d toCenter = ellipsoidTangent(recipSemiAxes, w, e, e_, ee);
        toCenter *= maxSemiAxis * scale;
        glVertex3f((GLfloat) toCenter.x, (GLfloat) toCenter.y, (GLfloat) toCenter.z);
    }

    glEnd();

    glPopMatrix();

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
}


float
VisibleRegion::boundingSphereRadius() const
{
    return m_body.getRadius();
}
