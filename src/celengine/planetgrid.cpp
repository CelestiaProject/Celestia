// planetgrid.cpp
//
// Longitude/latitude grids for ellipsoidal bodies.
//
// Copyright (C) 2008-present, the Celestia Development Team
// Initial version by Chris Laurel, claurel@gmail.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "planetgrid.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iterator>

#include <Eigen/Geometry>
#include <fmt/format.h>

#include <celastro/date.h>
#include <celcompat/numbers.h>
#include <celmath/ellipsoid.h>
#include <celmath/geomutil.h>
#include <celmath/intersect.h>
#include <celmath/mathlib.h>
#include <celmath/vecgl.h>
#include <celrender/linerenderer.h>
#include <celrender/referencemarkrenderer.h>
#include "body.h"
#include "render.h"

using namespace std::string_view_literals;

namespace astro = celestia::astro;
namespace math = celestia::math;
namespace render = celestia::render;

namespace
{
constexpr unsigned int circleSubdivisions = 100U;

void longLatLabel(const std::string& labelText,
                  double longitude,
                  double latitude,
                  const Eigen::Vector3d& viewRayOrigin,
                  const Eigen::Vector3d& viewNormal,
                  const Eigen::Vector3d& bodyCenter,
                  const Eigen::Quaterniond& bodyOrientation,
                  const Eigen::Vector3f& semiAxes,
                  float labelOffset,
                  Renderer& renderer)
{
    double stheta;
    double ctheta;
    math::sincos(math::degToRad(longitude), stheta, ctheta);
    double sphi;
    double cphi;
    math::sincos(math::degToRad(latitude), sphi, cphi);
    Eigen::Vector3d pos( cphi * ctheta * semiAxes.x(),
                         sphi * semiAxes.y(),
                        -cphi * stheta * semiAxes.z());

    float nearDist = renderer.getNearPlaneDistance();

    pos *= 1.0 + labelOffset;

    double boundingRadius = semiAxes.maxCoeff();

    // Draw the label only if it isn't obscured by the body ellipsoid
    if (double t = 0.0; math::testIntersection(Eigen::ParametrizedLine<double, 3>(viewRayOrigin, pos - viewRayOrigin),
                                               math::Ellipsoidd(semiAxes.cast<double>()), t) && t >= 1.0)
    {
        // Compute the position of the label
        Eigen::Vector3d labelPos = bodyCenter +
                                   bodyOrientation.conjugate() * pos * (1.0 + labelOffset);

        // Calculate the intersection of the eye-to-label ray with the plane perpendicular to
        // the view normal that touches the front of the objects bounding sphere
        double planetZ = std::max(viewNormal.dot(bodyCenter) - boundingRadius, -nearDist * 1.001);
        double z = viewNormal.dot(labelPos);
        labelPos *= planetZ / z;

        renderer.addObjectAnnotation(nullptr, labelText,
                                     Renderer::PlanetographicGridLabelColor,
                                     labelPos.cast<float>(),
                                     Renderer::LabelHorizontalAlignment::Start,
                                     Renderer::LabelVerticalAlignment::Bottom);
    }
}

void
latitudeLabel(std::string& buf,
              int latitude,
              PlanetographicGrid::NorthDirection northDirection)
{
    buf.clear();
    char ns = (latitude < 0) != (northDirection == PlanetographicGrid::NorthDirection::NorthNormal)
        ? 'N'
        : 'S';

    fmt::format_to(std::back_inserter(buf), "{}{}", std::abs(latitude), ns);
}

void
longitudeLabel(std::string& buf,
               int longitude,
               PlanetographicGrid::LongitudeConvention longitudeConvention)
{
    buf.clear();
    auto it = std::back_inserter(buf);
    switch (longitudeConvention)
    {
    case PlanetographicGrid::LongitudeConvention::EastWest:
        if (longitude >= 0)
            fmt::format_to(it, "{}E", longitude);
        else
            fmt::format_to(it, "{}W", -longitude);
        break;

    case PlanetographicGrid::LongitudeConvention::Eastward:
        fmt::format_to(it, "{}E", longitude > 0 ? 360 - longitude : std::abs(longitude));
        break;

    case PlanetographicGrid::LongitudeConvention::Westward:
        fmt::format_to(it, "{}W", longitude > 0 ? 360 - longitude : std::abs(longitude));

    default:
        assert(0);
        break;
    }
}

class RenderDetails
{
public:
    explicit RenderDetails(render::ReferenceMarkRenderer&,
                           const Body&,
                           const Eigen::Vector3f&,
                           float,
                           double,
                           const Matrices&);

    RenderDetails(const RenderDetails&) = delete;
    RenderDetails& operator=(const RenderDetails&) = delete;

    void renderLatitude(int, std::string&, bool, PlanetographicGrid::NorthDirection) const;
    void renderLongitude(int, std::string&, bool, PlanetographicGrid::LongitudeConvention) const;

private:
    Eigen::Matrix4f modelView;
    Eigen::Matrix4f projection;
    Eigen::Quaterniond q;
    Eigen::Vector3d posd;
    Eigen::Vector3d viewRayOrigin;
    Eigen::Vector3d viewNormal;
    Eigen::Vector3f semiAxes;
    float offset;
    Renderer& renderer;
    render::PlanetGridRenderer& planetGridRenderer;
};

RenderDetails::RenderDetails(render::ReferenceMarkRenderer& refMarkRenderer,
                             const Body& body,
                             const Eigen::Vector3f& pos,
                             float discSizeInPixels,
                             double tdb,
                             const Matrices& m) :
    projection(*m.projection),
    q(math::YRot180<double> * body.getEclipticToBodyFixed(tdb)), // Celestia coordinates
    posd(pos.cast<double>()),
    semiAxes(body.getSemiAxes()),
    renderer(refMarkRenderer.renderer()),
    planetGridRenderer(refMarkRenderer.planetGridRenderer())
{
    Eigen::Quaternionf qf = q.cast<float>();

    // The grid can't be rendered exactly on the planet sphere, or
    // there will be z-fighting problems. Render it at a height above the
    // planet that will place it about one pixel away from the planet.
    float scale = (discSizeInPixels + 1) / discSizeInPixels;
    scale = std::max(scale, 1.001f);
    offset = scale - 1.0f;

    viewRayOrigin = q * -posd;

    // Calculate the view normal; this is used for placement of the long/lat
    // label text.
    Eigen::Vector3f vn = refMarkRenderer.renderer().getCameraOrientationf().conjugate() * -Eigen::Vector3f::UnitZ();
    viewNormal = vn.cast<double>();

    Eigen::Affine3f transform = Eigen::Translation3f(pos) * qf.conjugate() * Eigen::Scaling(scale * semiAxes);
    modelView = *m.modelview * transform.matrix();
}

void
RenderDetails::renderLatitude(int latitudeStep,
                              std::string& buf,
                              bool showCoordinateLabels,
                              PlanetographicGrid::NorthDirection northDirection) const
{
    render::LineRenderer& latitudeRenderer = planetGridRenderer.latitudeRenderer();
    render::LineRenderer& equatorRenderer = planetGridRenderer.equatorRenderer();

    for (int latitude = -90 + latitudeStep; latitude < 90; latitude += latitudeStep)
    {
        float sphi;
        float r;
        math::sincos(math::degToRad(static_cast<float>(latitude)), sphi, r);

        Eigen::Matrix4f mvcur = modelView * math::translate(0.0f, sphi, 0.0f) * math::scale(r);
        if (latitude == 0)
        {
            latitudeRenderer.finish();
            equatorRenderer.render({&projection, &mvcur},
                                   Renderer::PlanetEquatorColor,
                                   circleSubdivisions+1);
            equatorRenderer.finish();
        }
        else
        {
            latitudeRenderer.render({&projection, &mvcur},
                                    Renderer::PlanetographicGridColor,
                                    circleSubdivisions+1);
        }

        if (showCoordinateLabels && latitude != 0 && std::abs(latitude) < 90)
        {
            latitudeLabel(buf, latitude, northDirection);
            longLatLabel(buf, 0.0, latitude, viewRayOrigin, viewNormal, posd, q, semiAxes, offset, renderer);
            longLatLabel(buf, 180.0, latitude, viewRayOrigin, viewNormal, posd, q, semiAxes, offset, renderer);
        }
    }
    latitudeRenderer.finish();
    equatorRenderer.finish();
}

void
RenderDetails::renderLongitude(int longitudeStep,
                               std::string& buf,
                               bool showCoordinateLabels,
                               PlanetographicGrid::LongitudeConvention longitudeConvention) const
{
    render::LineRenderer& longitudeRenderer = planetGridRenderer.longitudeRenderer();
    for (int longitude = 0; longitude <= 180; longitude += longitudeStep)
    {
        Eigen::Matrix4f mvcur = modelView * math::rotate(Eigen::AngleAxisf(math::degToRad(static_cast<float>(longitude)),
                                                                           Eigen::Vector3f::UnitY()));

        longitudeRenderer.render({&projection, &mvcur},
                                 Renderer::PlanetographicGridColor,
                                 circleSubdivisions + 1);

        if (!showCoordinateLabels)
            continue;

        longitudeLabel(buf, longitude, longitudeConvention);
        longLatLabel(buf, longitude, 0.0, viewRayOrigin, viewNormal, posd, q, semiAxes, offset, renderer);
        if (longitude > 0 && longitude < 180)
        {
            longitudeLabel(buf, -longitude, longitudeConvention);
            longLatLabel(buf, -longitude, 0.0, viewRayOrigin, viewNormal, posd, q, semiAxes, offset, renderer);
        }
    }

    longitudeRenderer.finish();
}

} // namespace

PlanetographicGrid::PlanetographicGrid(const Body& _body) :
    body(_body)
{
    setIAULongLatConvention();
}

void
PlanetographicGrid::render(render::ReferenceMarkRenderer* refMarkRenderer,
                           const Eigen::Vector3f& pos,
                           float discSizeInPixels,
                           double tdb,
                           const Matrices& m) const
{
    RenderDetails renderDetails(*refMarkRenderer, body, pos, discSizeInPixels, tdb, m);

    Renderer& renderer = refMarkRenderer->renderer();

    Renderer::PipelineState ps;
    ps.depthMask = true;
    ps.depthTest = true;
    ps.smoothLines = true;
    renderer.setPipelineState(ps);

    // Only show the coordinate labels if the body is sufficiently large on screen
    bool showCoordinateLabels = discSizeInPixels > 50;

    int stepSize = discSizeInPixels < 200 ? 30 : 10;
    std::string buf;

    renderDetails.renderLatitude(stepSize, buf, showCoordinateLabels, northDirection);
    renderDetails.renderLongitude(stepSize, buf, showCoordinateLabels, longitudeConvention);
}

float
PlanetographicGrid::boundingSphereRadius() const
{
    return body.getRadius();
}

/*! Determine the longitude convention to use based on IAU rules:
 *    Westward for prograde rotators, Eastward for retrograde
 *    rotators, EastWest for the Earth and Moon.
 */
void
PlanetographicGrid::setIAULongLatConvention()
{
    if (body.getName() == "Earth" || body.getName() == "Moon")
    {
        northDirection = NorthDirection::NorthNormal;
        longitudeConvention = LongitudeConvention::EastWest;
    }
    else if (body.getAngularVelocity(astro::J2000).y() >= 0.0)
    {
        northDirection = NorthDirection::NorthNormal;
        longitudeConvention = LongitudeConvention::Westward;
    }
    else
    {
        northDirection = NorthDirection::NorthReversed;
        longitudeConvention = LongitudeConvention::Eastward;
    }
}

std::string_view
PlanetographicGrid::defaultTag() const
{
    return "planetographic grid"sv;
}
