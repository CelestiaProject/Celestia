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

#include <cmath>
#include <Eigen/Geometry>
#include <fmt/format.h>
#include <celcompat/numbers.h>
#include <celmath/intersect.h>
#include <celrender/linerenderer.h>
#include "body.h"
#include "planetgrid.h"
#include "render.h"
#include "vecgl.h"

using namespace celmath;
using namespace celestia;
using celestia::engine::LineRenderer;

namespace
{
constexpr unsigned circleSubdivisions = 100;
LineRenderer *g_latitudeRenderer = nullptr;
LineRenderer *g_equatorRenderer = nullptr;
LineRenderer *g_longitudeRenderer = nullptr;
bool g_initialized = false;

static void longLatLabel(const std::string& labelText,
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
    double theta = degToRad(longitude);
    double phi = degToRad(latitude);
    Eigen::Vector3d pos( cos(phi) * cos(theta) * semiAxes.x(),
                         sin(phi) * semiAxes.y(),
                        -cos(phi) * sin(theta) * semiAxes.z());

    float nearDist = renderer->getNearPlaneDistance();

    pos = pos * (1.0 + labelOffset);

    double boundingRadius = semiAxes.maxCoeff();

    // Draw the label only if it isn't obscured by the body ellipsoid
    double t = 0.0;
    if (testIntersection(Eigen::ParametrizedLine<double, 3>(viewRayOrigin, pos - viewRayOrigin),
                         Ellipsoidd(semiAxes.cast<double>()), t) && t >= 1.0)
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
                                      labelPos.cast<float>());
    }
}
} // namespace

PlanetographicGrid::PlanetographicGrid(Renderer& _renderer, const Body& _body) :
    renderer(_renderer),
    body(_body)
{
    InitializeGeometry(renderer);
    setTag("planetographic grid");
    setIAULongLatConvention();
}

void
PlanetographicGrid::render(const Eigen::Vector3f& pos,
                           float discSizeInPixels,
                           double tdb,
                           const Matrices& m) const
{
    // Compatibility
    Eigen::Quaterniond q(Eigen::AngleAxis(celestia::numbers::pi, Eigen::Vector3d::UnitY()));
    q *= body.getEclipticToBodyFixed(tdb);
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
    Eigen::Vector3f vn  = renderer.getCameraOrientation().conjugate() * -Eigen::Vector3f::UnitZ();
    Eigen::Vector3d viewNormal = vn.cast<double>();

    // Enable depth buffering
    renderer.enableDepthTest();
    renderer.enableDepthMask();
    renderer.disableBlending();

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
        float phi = degToRad(latitude);
        float r = std::cos(phi);

        Eigen::Matrix4f mvcur = modelView * vecgl::translate(0.0f, std::sin(phi), 0.0f) * vecgl::scale(r);
        if (latitude == 0.0f)
        {
            g_equatorRenderer->render({&projection, &mvcur},
                                      Renderer::PlanetEquatorColor,
                                      circleSubdivisions+1);
        }
        else
        {
            g_latitudeRenderer->render({&projection, &mvcur},
                                       Renderer::PlanetographicGridColor,
                                       circleSubdivisions+1);
        }

        if (showCoordinateLabels)
        {
            if (latitude != 0.0f && abs(latitude) < 90.0f)
            {
                char ns;
                if (latitude < 0.0f)
                    ns = northDirection == NorthNormal ? 'S' : 'N';
                else
                    ns = northDirection == NorthNormal ? 'N' : 'S';

                auto buf = fmt::format("{}{}", static_cast<int>(fabs(latitude)), ns);
                longLatLabel(buf, 0.0, latitude, viewRayOrigin, viewNormal, posd, q, semiAxes, offset, &renderer);
                longLatLabel(buf, 180.0, latitude, viewRayOrigin, viewNormal, posd, q, semiAxes, offset, &renderer);
            }
        }
    }

    for (float longitude = 0.0f; longitude <= 180.0f; longitude += longitudeStep)
    {
        Eigen::Matrix4f mvcur =  modelView * vecgl::rotate(Eigen::AngleAxisf(degToRad(longitude), Eigen::Vector3f::UnitY()));

        g_longitudeRenderer->render({&projection, &mvcur},
                                    Renderer::PlanetographicGridColor,
                                    circleSubdivisions+1);

        if (showCoordinateLabels)
        {
            int showLongitude = 0;
            char ew = 'E';

            switch (longitudeConvention)
            {
            case EastWest:
                ew = 'E';
                showLongitude = (int) longitude;
                break;
            case Eastward:
                if (longitude > 0.0f)
                    showLongitude = 360 - (int) longitude;
                ew = 'E';
                break;
            case Westward:
                if (longitude > 0.0f)
                    showLongitude = 360 - (int) longitude;
                ew = 'W';
                break;
            }

            auto buf = fmt::format("{}{}", showLongitude, ew);
            longLatLabel(buf, longitude, 0.0, viewRayOrigin, viewNormal, posd, q, semiAxes, offset, &renderer);
            if (longitude > 0.0f && longitude < 180.0f)
            {
                showLongitude = (int) longitude;
                switch (longitudeConvention)
                {
                case EastWest:
                    ew = 'W';
                    showLongitude = (int) longitude;
                    break;
                case Eastward:
                    showLongitude = (int) longitude;
                    ew = 'E';
                    break;
                case Westward:
                    showLongitude = (int) longitude;
                    ew = 'W';
                    break;
                }

                buf = fmt::format("{}{}", showLongitude, ew);
                longLatLabel(buf, -longitude, 0.0, viewRayOrigin, viewNormal, posd, q, semiAxes, offset, &renderer);
            }
        }
    }

    renderer.enableBlending();
    renderer.setBlendingFactors(GL_SRC_ALPHA, GL_ONE);
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
        northDirection = NorthNormal;
        longitudeConvention = EastWest;
    }
    else
    {
        if (body.getAngularVelocity(astro::J2000).y() >= 0.0)
        {
            northDirection = NorthNormal;
            longitudeConvention = Westward;
        }
        else
        {
            northDirection = NorthReversed;
            longitudeConvention = Eastward;
        }
    }
}

void
PlanetographicGrid::InitializeGeometry(const Renderer &renderer)
{
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;

    g_latitudeRenderer  = new LineRenderer(renderer, 1.0f, LineRenderer::LINE_STRIP);
    g_equatorRenderer   = new LineRenderer(renderer, 2.0f, LineRenderer::LINE_STRIP);
    g_longitudeRenderer = new LineRenderer(renderer, 1.0f, LineRenderer::LINE_STRIP);

    for (unsigned int i = 0; i <= circleSubdivisions + 1; i++)
    {
        float theta = (float) (2.0 * celestia::numbers::pi) * (float) i / (float) circleSubdivisions;
        float s, c;
        sincos(theta, s, c);
        Eigen::Vector3f latitudePoint(c, 0.0f, s);
        Eigen::Vector3f longitudePoint(c, s, 0.0f);
        g_latitudeRenderer->addVertex(latitudePoint);
        g_equatorRenderer->addVertex(latitudePoint);
        g_longitudeRenderer->addVertex(longitudePoint);
    }
}
