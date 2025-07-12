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
#include <cmath>

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
#include "body.h"
#include "render.h"

using namespace std::string_view_literals;

namespace astro = celestia::astro;
namespace math = celestia::math;
namespace render = celestia::render;

render::LineRenderer *PlanetographicGrid::latitudeRenderer = nullptr;
render::LineRenderer *PlanetographicGrid::equatorRenderer = nullptr;
render::LineRenderer *PlanetographicGrid::longitudeRenderer = nullptr;
bool PlanetographicGrid::initialized = false;

namespace
{
constexpr unsigned circleSubdivisions = 100;

void longLatLabel(const std::string& labelText,
                  double longitude,
                  double latitude,
                  const Eigen::Vector3d& viewRayOrigin,
                  const Eigen::Vector3d& viewNormal,
                  const Eigen::Vector3d& bodyCenter,
                  const Eigen::Quaterniond& bodyOrientation,
                  const Eigen::Vector3f& semiAxes,
                  float labelOffset,
                  Renderer* renderer)
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

    float nearDist = renderer->getNearPlaneDistance();

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

        renderer->addObjectAnnotation(nullptr, labelText,
                                      Renderer::PlanetographicGridLabelColor,
                                      labelPos.cast<float>(),
                                      Renderer::LabelHorizontalAlignment::Start,
                                      Renderer::LabelVerticalAlignment::Bottom);
    }
}
} // namespace

PlanetographicGrid::PlanetographicGrid(const Body& _body) :
    body(_body)
{
    setIAULongLatConvention();
}

void
PlanetographicGrid::render(Renderer* renderer,
                           const Eigen::Vector3f& pos,
                           float discSizeInPixels,
                           double tdb,
                           const Matrices& m) const
{
    InitializeGeometry(*renderer);

    // Compatibility
    Eigen::Quaterniond q = math::YRot180<double> * body.getEclipticToBodyFixed(tdb);
    Eigen::Quaternionf qf = q.cast<float>();

    // The grid can't be rendered exactly on the planet sphere, or
    // there will be z-fighting problems. Render it at a height above the
    // planet that will place it about one pixel away from the planet.
    float scale = (discSizeInPixels + 1) / discSizeInPixels;
    scale = std::max(scale, 1.001f);
    float offset = scale - 1.0f;

    Eigen::Vector3f semiAxes = body.getSemiAxes();
    Eigen::Vector3d posd = pos.cast<double>();
    Eigen::Vector3d viewRayOrigin = q * -posd;

    // Calculate the view normal; this is used for placement of the long/lat
    // label text.
    Eigen::Vector3f vn = renderer->getCameraOrientationf().conjugate() * -Eigen::Vector3f::UnitZ();
    Eigen::Vector3d viewNormal = vn.cast<double>();

    Renderer::PipelineState ps;
    ps.depthMask = true;
    ps.depthTest = true;
    ps.smoothLines = true;
    renderer->setPipelineState(ps);

    Eigen::Affine3f transform = Eigen::Translation3f(pos) * qf.conjugate() * Eigen::Scaling(scale * semiAxes);
    Eigen::Matrix4f projection = *m.projection;
    Eigen::Matrix4f modelView = *m.modelview * transform.matrix();

    // Only show the coordinate labels if the body is sufficiently large on screen
    bool showCoordinateLabels = false;
    if (discSizeInPixels > 50)
        showCoordinateLabels = true;

    float latitudeStep = minLatitudeStep;
    float longitudeStep = minLongitudeStep;
    if (discSizeInPixels < 200)
    {
        latitudeStep = 30.0f;
        longitudeStep = 30.0f;
    }

    for (float latitude = -90.0f + latitudeStep; latitude < 90.0f; latitude += latitudeStep)
    {
        float phi = math::degToRad(latitude);
        float r = std::cos(phi);

        Eigen::Matrix4f mvcur = modelView * math::translate(0.0f, std::sin(phi), 0.0f) * math::scale(r);
        if (latitude == 0.0f)
        {
            latitudeRenderer->finish();
            equatorRenderer->render({&projection, &mvcur},
                                    Renderer::PlanetEquatorColor,
                                    circleSubdivisions+1);
            equatorRenderer->finish();
        }
        else
        {
            latitudeRenderer->render({&projection, &mvcur},
                                     Renderer::PlanetographicGridColor,
                                     circleSubdivisions+1);
        }

        if (showCoordinateLabels)
        {
            if (latitude != 0.0f && abs(latitude) < 90.0f)
            {
                char ns;
                if (latitude < 0.0f)
                    ns = northDirection == NorthDirection::NorthNormal ? 'S' : 'N';
                else
                    ns = northDirection == NorthDirection::NorthNormal ? 'N' : 'S';

                auto buf = fmt::format("{}{}", static_cast<int>(std::fabs(latitude)), ns);
                longLatLabel(buf, 0.0, latitude, viewRayOrigin, viewNormal, posd, q, semiAxes, offset, renderer);
                longLatLabel(buf, 180.0, latitude, viewRayOrigin, viewNormal, posd, q, semiAxes, offset, renderer);
            }
        }
    }
    latitudeRenderer->finish();
    equatorRenderer->finish();

    for (float longitude = 0.0f; longitude <= 180.0f; longitude += longitudeStep)
    {
        Eigen::Matrix4f mvcur = modelView * math::rotate(Eigen::AngleAxisf(math::degToRad(longitude), Eigen::Vector3f::UnitY()));

        longitudeRenderer->render({&projection, &mvcur},
                                  Renderer::PlanetographicGridColor,
                                  circleSubdivisions+1);

        if (showCoordinateLabels)
        {
            int showLongitude = 0;
            char ew = 'E';

            switch (longitudeConvention)
            {
            case LongitudeConvention::EastWest:
                ew = 'E';
                showLongitude = (int) longitude;
                break;
            case LongitudeConvention::Eastward:
                if (longitude > 0.0f)
                    showLongitude = 360 - static_cast<int>(longitude);
                ew = 'E';
                break;
            case LongitudeConvention::Westward:
                if (longitude > 0.0f)
                    showLongitude = 360 - static_cast<int>(longitude);
                ew = 'W';
                break;
            }

            auto buf = fmt::format("{}{}", showLongitude, ew);
            longLatLabel(buf, longitude, 0.0, viewRayOrigin, viewNormal, posd, q, semiAxes, offset, renderer);
            if (longitude > 0.0f && longitude < 180.0f)
            {
                showLongitude = (int) longitude;
                switch (longitudeConvention)
                {
                case LongitudeConvention::EastWest:
                    ew = 'W';
                    showLongitude = static_cast<int>(longitude);
                    break;
                case LongitudeConvention::Eastward:
                    showLongitude = static_cast<int>(longitude);
                    ew = 'E';
                    break;
                case LongitudeConvention::Westward:
                    showLongitude = static_cast<int>(longitude);
                    ew = 'W';
                    break;
                }

                buf = fmt::format("{}{}", showLongitude, ew);
                longLatLabel(buf, -longitude, 0.0, viewRayOrigin, viewNormal, posd, q, semiAxes, offset, renderer);
            }
        }
    }
    longitudeRenderer->finish();

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

void
PlanetographicGrid::InitializeGeometry(const Renderer &renderer)
{
    using LineRenderer = render::LineRenderer;

    if (initialized)
        return;
    initialized = true;

    latitudeRenderer  = new LineRenderer(renderer, 1.0f, LineRenderer::PrimType::LineStrip, LineRenderer::StorageType::Static);
    equatorRenderer   = new LineRenderer(renderer, 2.0f, LineRenderer::PrimType::LineStrip, LineRenderer::StorageType::Static);
    longitudeRenderer = new LineRenderer(renderer, 1.0f, LineRenderer::PrimType::LineStrip, LineRenderer::StorageType::Static);

    for (unsigned int i = 0; i <= circleSubdivisions + 1; i++)
    {
        float s, c;
        math::sincos((2.0f * celestia::numbers::pi_v<float>) * static_cast<float>(i) / circleSubdivisions, s, c);
        Eigen::Vector3f latitudePoint(c, 0.0f, s);
        Eigen::Vector3f longitudePoint(c, s, 0.0f);
        latitudeRenderer->addVertex(latitudePoint);
        equatorRenderer->addVertex(latitudePoint);
        longitudeRenderer->addVertex(longitudePoint);
    }
}

void
PlanetographicGrid::deinit()
{
    delete latitudeRenderer;
    latitudeRenderer = nullptr;
    delete equatorRenderer;
    equatorRenderer = nullptr;
    delete longitudeRenderer;
    longitudeRenderer = nullptr;
}

std::string_view
PlanetographicGrid::defaultTag() const
{
    return "planetographic grid"sv;
}
