// planetgrid.cpp
//
// Longitude/latitude grids for ellipsoidal bodies.
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
#include "planetgrid.h"
#include "body.h"
#include "gl.h"
#include "vecgl.h"
#include "render.h"


unsigned int PlanetographicGrid::circleSubdivisions = 100;
float* PlanetographicGrid::xyCircle = NULL;
float* PlanetographicGrid::xzCircle = NULL;


PlanetographicGrid::PlanetographicGrid(const Body& _body) :
    body(_body),
    minLongitudeStep(10.0f),
    minLatitudeStep(10.0f),
    longitudeConvention(Westward),
    northDirection(NorthNormal)
{
    if (xyCircle == NULL)
        InitializeGeometry();
    setTag("planetographic grid");
    setIAULongLatConvention();
}


PlanetographicGrid::~PlanetographicGrid()
{
}


static void longLatLabel(const string& labelText,
                         double longitude,
                         double latitude,
                         const Vec3d& viewRayOrigin,
                         const Vec3d& viewNormal,
                         const Vec3d& bodyCenter,
                         const Quatd& bodyOrientation,
                         const Vec3f& semiAxes,
                         float labelOffset,
                         Renderer* renderer)
{
    double theta = degToRad(longitude);
    double phi = degToRad(latitude);
    Vec3d pos(cos(phi) * cos(theta) * semiAxes.x,
              sin(phi) * semiAxes.y,
              -cos(phi) * sin(theta) * semiAxes.z);
    
    float nearDist = renderer->getNearPlaneDistance();
    
    pos = pos * (1.0 + labelOffset);
    
    double boundingRadius = max(semiAxes.x, max(semiAxes.y, semiAxes.z));

    // Draw the label only if it isn't obscured by the body ellipsoid
    double t = 0.0;
    if (testIntersection(Ray3d(Point3d(0.0, 0.0, 0.0) + viewRayOrigin, pos - viewRayOrigin),
                         Ellipsoidd(Vec3d(semiAxes.x, semiAxes.y, semiAxes.z)), t) && t >= 1.0)
    {
        // Compute the position of the label
        Vec3d labelPos = bodyCenter +
                         (1.0 + labelOffset) * pos * bodyOrientation.toMatrix3();
        
        // Calculate the intersection of the eye-to-label ray with the plane perpendicular to
        // the view normal that touches the front of the objects bounding sphere
        double planetZ = viewNormal * bodyCenter - boundingRadius;
        if (planetZ < -nearDist * 1.001)
            planetZ = -nearDist * 1.001;
        double z = viewNormal * labelPos;
        labelPos *= planetZ / z;
        
        renderer->addObjectAnnotation(NULL, labelText,
                                      Renderer::PlanetographicGridLabelColor,
                                      Point3f((float) labelPos.x, (float) labelPos.y, (float) labelPos.z));                             
    }
}


void
PlanetographicGrid::render(Renderer* renderer,
                           const Point3f& pos,
                           float discSizeInPixels,
                           double tdb) const
{
    Quatd q = Quatd::yrotation(PI) * body.getEclipticToBodyFixed(tdb);
    Quatf qf((float) q.w, (float) q.x, (float) q.y, (float) q.z);

    // The grid can't be rendered exactly on the planet sphere, or
    // there will be z-fighting problems. Render it at a height above the
    // planet that will place it about one pixel away from the planet.
    float scale = (discSizeInPixels + 1) / discSizeInPixels;
    scale = max(scale, 1.001f);
    float offset = scale - 1.0f;

    Vec3f semiAxes = body.getSemiAxes();

    Vec3d posd(pos.x, pos.y, pos.z);
    Vec3d viewRayOrigin = Vec3d(-pos.x, -pos.y, -pos.z)  * (~q).toMatrix3();
    
    // Calculate the view normal; this is used for placement of the long/lat
    // label text.
    Vec3f vn  = Vec3f(0.0f, 0.0f, -1.0f) * renderer->getCameraOrientation().toMatrix3();
    Vec3d viewNormal(vn.x, vn.y, vn.z);

    // Enable depth buffering
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    glDisable(GL_TEXTURE_2D);

    glPushMatrix();
    glRotate(~qf);
    glScale(scale * semiAxes);

    glEnableClientState(GL_VERTEX_ARRAY);

    glVertexPointer(3, GL_FLOAT, 0, xzCircle);

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
        float r = (float) cos(phi);

        if (latitude == 0.0f)
        {
            glColor(Renderer::PlanetEquatorColor);
            glLineWidth(2.0f);
        }
        else
        {
            glColor(Renderer::PlanetographicGridColor);
        }
        glPushMatrix();
        glTranslatef(0.0f, (float) sin(phi), 0.0f);
        glScalef(r, r, r);
        glDrawArrays(GL_LINE_LOOP, 0, circleSubdivisions);
        glPopMatrix();
        glLineWidth(1.0f);
        
        if (showCoordinateLabels)
        {
            if (latitude != 0.0f && abs(latitude) < 90.0f)
            {
                char buf[64];

                char ns;
                if (latitude < 0.0f)
                    ns = northDirection == NorthNormal ? 'S' : 'N';
                else
                    ns = northDirection == NorthNormal ? 'N' : 'S';
                sprintf(buf, "%d%c", (int) fabs((double) latitude), ns);
                longLatLabel(buf, 0.0, latitude, viewRayOrigin, viewNormal, posd, q, semiAxes, offset, renderer);
                longLatLabel(buf, 180.0, latitude, viewRayOrigin, viewNormal, posd, q, semiAxes, offset, renderer);
            }
        }
    }

    glVertexPointer(3, GL_FLOAT, 0, xyCircle);

    for (float longitude = 0.0f; longitude <= 180.0f; longitude += longitudeStep)
    {
        glColor(Renderer::PlanetographicGridColor);
        glPushMatrix();
        glRotatef(longitude, 0.0f, 1.0f, 0.0f);
        glDrawArrays(GL_LINE_LOOP, 0, circleSubdivisions);
        glPopMatrix();

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

            char buf[64];
            sprintf(buf, "%d%c", (int) showLongitude, ew);
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

                sprintf(buf, "%d%c", showLongitude, ew);       
                longLatLabel(buf, -longitude, 0.0, viewRayOrigin, viewNormal, posd, q, semiAxes, offset, renderer);
            }
        }
    }

    glDisableClientState(GL_VERTEX_ARRAY);

    glPopMatrix();

    glDisable(GL_LIGHTING);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
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
        if (body.getAngularVelocity(astro::J2000).y >= 0.0)
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
        xyCircle[i * 3 + 0] = (float) cos(theta);
        xyCircle[i * 3 + 1] = (float) sin(theta);
        xyCircle[i * 3 + 2] = 0.0f;
        xzCircle[i * 3 + 0] = (float) cos(theta);
        xzCircle[i * 3 + 1] = 0.0f;
        xzCircle[i * 3 + 2] = (float) sin(theta);
    }
}
