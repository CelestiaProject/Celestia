// planetgrid.cpp
//
// Longitude/latitude grids for ellipsoidal bodies.
//
// Copyright (C) 2008-2009, the Celestia Development Team
// Initial version by Chris Laurel, claurel@gmail.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cmath>
#include <Eigen/Geometry>
#include <celmath/intersect.h>
#include <celutil/debug.h>
#include "body.h"
#include "planetgrid.h"
#include "render.h"
#include "vecgl.h"

using namespace Eigen;
using namespace celmath;
using namespace celestia;


unsigned int PlanetographicGrid::circleSubdivisions = 100;
float* PlanetographicGrid::xyCircle = nullptr;
float* PlanetographicGrid::xzCircle = nullptr;


PlanetographicGrid::PlanetographicGrid(const Body& _body) :
    body(_body)
{
    if (xyCircle == nullptr)
        InitializeGeometry();
    setTag("planetographic grid");
    setIAULongLatConvention();
}


static void longLatLabel(const string& labelText,
                         double longitude,
                         double latitude,
                         const Vector3d& viewRayOrigin,
                         const Vector3d& viewNormal,
                         const Vector3d& bodyCenter,
                         const Quaterniond& bodyOrientation,
                         const Vector3f& semiAxes,
                         float labelOffset,
                         Renderer* renderer)
{
    double theta = degToRad(longitude);
    double phi = degToRad(latitude);
    Vector3d pos(cos(phi) * cos(theta) * semiAxes.x(),
                 sin(phi) * semiAxes.y(),
                 -cos(phi) * sin(theta) * semiAxes.z());

    float nearDist = renderer->getNearPlaneDistance();

    pos = pos * (1.0 + labelOffset);

    double boundingRadius = semiAxes.maxCoeff();

    // Draw the label only if it isn't obscured by the body ellipsoid
    double t = 0.0;
    if (testIntersection(Ray3d(viewRayOrigin, pos - viewRayOrigin), Ellipsoidd(semiAxes.cast<double>()), t) && t >= 1.0)
    {
        // Compute the position of the label
        Vector3d labelPos = bodyCenter +
                            bodyOrientation.conjugate() * pos * (1.0 + labelOffset);

        // Calculate the intersection of the eye-to-label ray with the plane perpendicular to
        // the view normal that touches the front of the objects bounding sphere
        double planetZ = viewNormal.dot(bodyCenter) - boundingRadius;
        if (planetZ < -nearDist * 1.001)
            planetZ = -nearDist * 1.001;
        double z = viewNormal.dot(labelPos);
        labelPos *= planetZ / z;

        renderer->addObjectAnnotation(nullptr, labelText,
                                      Renderer::PlanetographicGridLabelColor,
                                      labelPos.cast<float>());
    }
}


void
PlanetographicGrid::render(Renderer* renderer,
                           const Eigen::Vector3f& pos,
                           float discSizeInPixels,
                           double tdb,
                           const Matrices& m) const
{
    ShaderProperties shadprop;
    shadprop.texUsage = ShaderProperties::VertexColors;
    shadprop.lightModel = ShaderProperties::UnlitModel;
    auto *prog = renderer->getShaderManager().getShader(shadprop);
    if (prog == nullptr)
        return;

    // Compatibility
    Quaterniond q = Quaterniond(AngleAxis<double>(PI, Vector3d::UnitY())) * body.getEclipticToBodyFixed(tdb);
    Quaternionf qf = q.cast<float>();

    // The grid can't be rendered exactly on the planet sphere, or
    // there will be z-fighting problems. Render it at a height above the
    // planet that will place it about one pixel away from the planet.
    float scale = (discSizeInPixels + 1) / discSizeInPixels;
    scale = max(scale, 1.001f);
    float offset = scale - 1.0f;

    Vector3f semiAxes = body.getSemiAxes();
    Vector3d posd = pos.cast<double>();
    Vector3d viewRayOrigin = q * -posd;

    // Calculate the view normal; this is used for placement of the long/lat
    // label text.
    Vector3f vn  = renderer->getCameraOrientation().conjugate() * -Vector3f::UnitZ();
    Vector3d viewNormal = vn.cast<double>();

    // Enable depth buffering
    renderer->enableDepthTest();
    renderer->enableDepthMask();
    renderer->disableBlending();

    Affine3f transform = Translation3f(pos) * qf.conjugate() * Scaling(scale * semiAxes);
    Matrix4f projection = *m.projection;
    Matrix4f modelView = *m.modelview * transform.matrix();

    glEnableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);
    glVertexAttribPointer(CelestiaGLProgram::VertexCoordAttributeIndex,
                          3, GL_FLOAT, GL_FALSE, 0, xzCircle);

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

    prog->use();

    for (float latitude = -90.0f + latitudeStep; latitude < 90.0f; latitude += latitudeStep)
    {
        float phi = degToRad(latitude);
        auto r = (float) std::cos(phi);

        if (latitude == 0.0f)
        {
            glVertexAttrib(CelestiaGLProgram::ColorAttributeIndex,
                           Renderer::PlanetEquatorColor);
            glLineWidth(2.0f);
        }
        else
        {
            glVertexAttrib(CelestiaGLProgram::ColorAttributeIndex,
                           Renderer::PlanetographicGridColor);
        }
        prog->setMVPMatrices(projection, modelView * vecgl::translate(0.0f, sin(phi), 0.0f) * vecgl::scale(r));
        glDrawArrays(GL_LINE_LOOP, 0, circleSubdivisions);

        glLineWidth(1.0f);

        if (showCoordinateLabels)
        {
            if (latitude != 0.0f && abs(latitude) < 90.0f)
            {
                char ns;
                if (latitude < 0.0f)
                    ns = northDirection == NorthNormal ? 'S' : 'N';
                else
                    ns = northDirection == NorthNormal ? 'N' : 'S';
                string buf;
                buf = fmt::sprintf("%d%c", (int) fabs(latitude), ns);
                longLatLabel(buf, 0.0, latitude, viewRayOrigin, viewNormal, posd, q, semiAxes, offset, renderer);
                longLatLabel(buf, 180.0, latitude, viewRayOrigin, viewNormal, posd, q, semiAxes, offset, renderer);
            }
        }
    }

    glVertexAttribPointer(CelestiaGLProgram::VertexCoordAttributeIndex,
                          3, GL_FLOAT, GL_FALSE, 0, xyCircle);

    glVertexAttrib(CelestiaGLProgram::ColorAttributeIndex,
                   Renderer::PlanetographicGridColor);
    for (float longitude = 0.0f; longitude <= 180.0f; longitude += longitudeStep)
    {
        prog->setMVPMatrices(projection, modelView * vecgl::rotate(AngleAxisf(degToRad(longitude), Vector3f::UnitY())));
        glDrawArrays(GL_LINE_LOOP, 0, circleSubdivisions);

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

            string buf;
            buf = fmt::sprintf("%d%c", (int) showLongitude, ew);
            longLatLabel(buf, longitude, 0.0, viewRayOrigin, viewNormal, posd, q, semiAxes, offset, renderer);
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

                buf = fmt::sprintf("%d%c", showLongitude, ew);
                longLatLabel(buf, -longitude, 0.0, viewRayOrigin, viewNormal, posd, q, semiAxes, offset, renderer);
            }
        }
    }

    glDisableVertexAttribArray(CelestiaGLProgram::VertexCoordAttributeIndex);

    renderer->enableBlending();
    renderer->setBlendingFactors(GL_SRC_ALPHA, GL_ONE);
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
PlanetographicGrid::InitializeGeometry()
{
    xyCircle = new float[circleSubdivisions * 3];
    xzCircle = new float[circleSubdivisions * 3];
    for (unsigned int i = 0; i < circleSubdivisions; i++)
    {
        float theta = (float) (2.0 * PI) * (float) i / (float) circleSubdivisions;
        float s, c;
        sincos(theta, s, c);
        xyCircle[i * 3 + 0] = c;
        xyCircle[i * 3 + 1] = s;
        xyCircle[i * 3 + 2] = 0.0f;
        xzCircle[i * 3 + 0] = c;
        xzCircle[i * 3 + 1] = 0.0f;
        xzCircle[i * 3 + 2] = s;
    }
}
