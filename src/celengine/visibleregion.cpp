// visibleregion.cpp
//
// Visible region reference mark for ellipsoidal bodies.
//
// Copyright (C) 2008-present, the Celestia Development Team
// Initial version by Chris Laurel, claurel@gmail.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cmath>
#include <Eigen/Geometry>
#include <celcompat/numbers.h>
#include <celmath/intersect.h>
#include <celrender/linerenderer.h>
#include "body.h"
#include "render.h"
#include "selection.h"
#include "visibleregion.h"

using namespace std;
using namespace Eigen;
using namespace celmath;
using celestia::engine::LineRenderer;


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
    m_opacity(1.0f)
{
    setTag("visible region");
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
}


constexpr const unsigned maxSections = 360;

void
VisibleRegion::render(Renderer* renderer,
                      const Vector3f& position,
                      float discSizeInPixels,
                      double tdb,
                      const Matrices& m) const
{
    // Proper terminator calculation requires double precision floats in GLSL
    // which were introduced in ARB_gpu_shader_fp64 unavailable with GL2.1.
    // Because of this we make calculations on a CPU and stream results to GPU.

    // Don't render anything if the current time is not within the
    // target object's time window.
    if (m_target.body() != nullptr)
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
    auto nSections = (unsigned int) (30.0f + discSizeInPixels * 0.5f);
    nSections = min(nSections, maxSections);

    Quaterniond q = m_body.getEclipticToBodyFixed(tdb);
    Quaternionf qf = q.cast<float>();

    // The outline can't be rendered exactly on the planet sphere, or
    // there will be z-fighting problems. Render it at a height above the
    // planet that will place it about one pixel away from the planet.
    float scale = (discSizeInPixels + 1) / discSizeInPixels;
    scale = max(scale, 1.0001f);

    Vector3f semiAxes = m_body.getSemiAxes();
    double maxSemiAxis = m_body.getRadius();

    // In order to avoid precision problems and extremely large values, scale
    // the target position and semiaxes such that the largest semiaxis is 1.0.
    Vector3d lightDir = m_body.getPosition(tdb).offsetFromKm(m_target.getPosition(tdb));
    lightDir = lightDir / maxSemiAxis;
    lightDir = q * lightDir;

    // Another measure to prevent precision problems: if the distance to the
    // object is much greater than the largest semi axis, clamp it to 1e4 times
    // the radius, as body-to-target rays at that distance are nearly parallel anyhow.
    if (lightDir.norm() > 10000.0)
        lightDir *= (10000.0 / lightDir.norm());

    // Pick two orthogonal axes both normal to the light direction
    Vector3d lightDirNorm = lightDir.normalized();

    Vector3d uAxis = lightDirNorm.unitOrthogonal();
    Vector3d vAxis = uAxis.cross(lightDirNorm);

    Vector3d recipSemiAxes = maxSemiAxis * semiAxes.cast<double>().cwiseInverse();
    Vector3d e = -lightDir;
    Vector3d e_ = e.cwiseProduct(recipSemiAxes);
    double ee = e_.squaredNorm();

    LineRenderer lr(*renderer, 1.0f, LineRenderer::LINE_STRIP);

    for (unsigned i = 0; i <= nSections + 1; i++)
    {
        double theta = (double) i / (double) (nSections) * 2.0 * celestia::numbers::pi;
        Vector3d w = cos(theta) * uAxis + sin(theta) * vAxis;

        Vector3d toCenter = ellipsoidTangent(recipSemiAxes, w, e, e_, ee);
        toCenter *= maxSemiAxis * scale;
        lr.addVertex(Vector3f(toCenter.cast<float>()));
    }

    Affine3f transform = Translation3f(position) * qf.conjugate();
    Matrix4f modelView = (*m.modelview) * transform.matrix();

    // Enable depth buffering
    renderer->enableDepthTest();
    renderer->enableDepthMask();
    renderer->enableBlending();
    renderer->setBlendingFactors(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    lr.render({ m.projection, &modelView }, Color(m_color, opacity), nSections+1, 0);

    renderer->setBlendingFactors(GL_SRC_ALPHA, GL_ONE);
}


float
VisibleRegion::boundingSphereRadius() const
{
    return m_body.getRadius();
}
