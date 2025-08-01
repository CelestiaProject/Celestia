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

#include "visibleregion.h"

#include <algorithm>

#include <Eigen/Geometry>

#include <celcompat/numbers.h>
#include <celmath/mathlib.h>
#include <celrender/linerenderer.h>
#include <celrender/referencemarkrenderer.h>
#include "body.h"
#include "glsupport.h"
#include "render.h"

using namespace std::string_view_literals;

namespace math = celestia::math;
namespace render = celestia::render;

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
    m_target(target)
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
}

constexpr const unsigned maxSections = 360;

void
VisibleRegion::render(render::ReferenceMarkRenderer* refMarkRenderer,
                      const Eigen::Vector3f& position,
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
    opacity = std::min(opacity, 1.0f) * m_opacity;

    // Base the amount of subdivision on the apparent size
    auto nSections = static_cast<unsigned int>(30.0f + discSizeInPixels * 0.5f);
    nSections = std::min(nSections, maxSections);

    Eigen::Quaterniond q = m_body.getEclipticToBodyFixed(tdb);
    Eigen::Quaternionf qf = q.cast<float>();

    // The outline can't be rendered exactly on the planet sphere, or
    // there will be z-fighting problems. Render it at a height above the
    // planet that will place it about one pixel away from the planet.
    float scale = (discSizeInPixels + 1) / discSizeInPixels;
    scale = std::max(scale, 1.0001f);

    Eigen::Vector3f semiAxes = m_body.getSemiAxes();
    double maxSemiAxis = m_body.getRadius();

    // In order to avoid precision problems and extremely large values, scale
    // the target position and semiaxes such that the largest semiaxis is 1.0.
    Eigen::Vector3d lightDir = m_body.getPosition(tdb).offsetFromKm(m_target.getPosition(tdb));
    lightDir = lightDir / maxSemiAxis;
    lightDir = q * lightDir;

    // Another measure to prevent precision problems: if the distance to the
    // object is much greater than the largest semi axis, clamp it to 1e4 times
    // the radius, as body-to-target rays at that distance are nearly parallel anyhow.
    if (lightDir.norm() > 10000.0)
        lightDir *= (10000.0 / lightDir.norm());

    // Pick two orthogonal axes both normal to the light direction
    Eigen::Vector3d lightDirNorm = lightDir.normalized();

    Eigen::Vector3d uAxis = lightDirNorm.unitOrthogonal();
    Eigen::Vector3d vAxis = uAxis.cross(lightDirNorm);

    Eigen::Vector3d recipSemiAxes = maxSemiAxis * semiAxes.cast<double>().cwiseInverse();
    Eigen::Vector3d e = -lightDir;
    Eigen::Vector3d e_ = e.cwiseProduct(recipSemiAxes);
    double ee = e_.squaredNorm();

    render::LineRenderer& lr = refMarkRenderer->visibleRegionRenderer();
    lr.clear();
    lr.startUpdate();

    for (unsigned i = 0; i <= nSections + 1; i++)
    {
        double stheta;
        double ctheta;
        math::sincos(static_cast<double>(i) / static_cast<double>(nSections) * 2.0 * celestia::numbers::pi, stheta, ctheta);
        Eigen::Vector3d w = ctheta * uAxis + stheta * vAxis;

        Eigen::Vector3d toCenter = math::ellipsoidTangent(recipSemiAxes, w, e, e_, ee);
        toCenter *= maxSemiAxis * scale;
        lr.addVertex(Eigen::Vector3f(toCenter.cast<float>()));
    }

    Eigen::Affine3f transform = Eigen::Translation3f(position) * qf.conjugate();
    Eigen::Matrix4f modelView = (*m.modelview) * transform.matrix();

    Renderer::PipelineState ps;
    ps.blending = true;
    ps.blendFunc = {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA};
    ps.depthMask = true;
    ps.depthTest = true;
    ps.smoothLines = true;
    refMarkRenderer->renderer().setPipelineState(ps);

    lr.render({ m.projection, &modelView }, Color(m_color, opacity), nSections+1, 0);
    lr.finish();
}

float
VisibleRegion::boundingSphereRadius() const
{
    return m_body.getRadius();
}

std::string_view
VisibleRegion::defaultTag() const
{
    return "visible region"sv;
}
