// body.cpp
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <stdlib.h>
#include "mathlib.h"
#include "util.h"
#include "astro.h"
#include "body.h"

using namespace std;


Body::Body(PlanetarySystem* _system) :
    orbit(NULL),
    oblateness(0),
    rotationPeriod(1),
    rotationPhase(0.0f),
    surface(Color(1.0f, 1.0f, 1.0f)),
    rings(NULL),
    satellites(NULL)
{
    system = _system;
}


PlanetarySystem* Body::getSystem() const
{
    return system;
}


string Body::getName() const
{
    return name;
}


void Body::setName(const string _name)
{
    name = _name;
}


Orbit* Body::getOrbit() const
{
    return orbit;
}


void Body::setOrbit(Orbit* _orbit)
{
    if (orbit == NULL)
        delete orbit;
    orbit = _orbit;
}


float Body::getRadius() const
{
    return radius;
}


void Body::setRadius(float _radius)
{
    radius = _radius;
}


float Body::getMass() const
{
    return mass;
}


void Body::setMass(float _mass)
{
    mass = _mass;
}


float Body::getOblateness() const
{
    return oblateness;
}


void Body::setOblateness(float _oblateness)
{
    oblateness = _oblateness;
}


float Body::getObliquity() const
{
    return obliquity;
}


void Body::setObliquity(float _obliquity)
{
    obliquity = _obliquity;
}


float Body::getAlbedo() const
{
    return albedo;
}


void Body::setAlbedo(float _albedo)
{
    albedo = _albedo;
}


float Body::getRotationPeriod() const
{
    return rotationPeriod;
}


void Body::setRotationPeriod(float _rotationPeriod)
{
    rotationPeriod = _rotationPeriod;
}


float Body::getRotationPhase() const
{
    return rotationPhase;
}


void Body::setRotationPhase(float _rotationPhase)
{
    rotationPhase = _rotationPhase;
}


const Surface& Body::getSurface() const
{
    return surface;
}


void Body::setSurface(const Surface& surf)
{
    surface = surf;
}


string Body::getMesh() const
{
    return mesh;
}

void Body::setMesh(const string _mesh)
{
    mesh = _mesh;
}


const PlanetarySystem* Body::getSatellites() const
{
    return satellites;
}

void Body::setSatellites(PlanetarySystem* ssys)
{
    satellites = ssys;
}


RingSystem* Body::getRings() const
{
    return rings;
}

void Body::setRings(const RingSystem& _rings)
{
    if (rings == NULL)
        rings = new RingSystem(_rings);
    else
        *rings = _rings;
}


// Get a matrix which converts from local to heliocentric coordinates
Mat4d Body::getLocalToHeliocentric(double when)
{
    Point3d pos = orbit->positionAtTime(when);
    Mat4d frame = Mat4d::xrotation(-obliquity) * Mat4d::translation(pos);
 
    // Recurse up the hierarchy . . .
    if (system != NULL && system->getPrimaryBody() != NULL)
        frame = frame * system->getPrimaryBody()->getLocalToHeliocentric(when);

    return frame;
}


// Return the position of the center of the body in heliocentric coordinates
Point3d Body::getHeliocentricPosition(double when)
{
    return Point3d(0.0, 0.0, 0.0) * getLocalToHeliocentric(when);
}


Quatd Body::getEclipticalToEquatorial()
{
    Quatd q(1);
    q.xrotate(-obliquity);
    return q;
}


Quatd Body::getEclipticalToGeographic(double when)
{
    return getEquatorialToGeographic(when) * getEclipticalToEquatorial();
}


// The geographic coordinate system has an origin at the center of the
// body, y-axis parallel to the rotation axis, x-axis through the prime
// meridian, and z-axis at a right angle the xy plane.  An object with
// constant geographic coordinates will thus remain fixed with respect
// to a point on the surface of the body.
Quatd Body::getEquatorialToGeographic(double when)
{
    double rotations = when / (double) getRotationPeriod();
    int wholeRotations = (int) rotations;
    double remainder = rotations - wholeRotations;
    
    Quatd q(1);
    q.yrotate(-remainder * 2 * PI - getRotationPhase());
    return q;
}


Mat4d Body::getGeographicToHeliocentric(double when)
{
    return getEquatorialToGeographic(when).toMatrix4() *
        getLocalToHeliocentric(when);
}


#define SOLAR_IRRADIANCE   1367.6
#define SOLAR_POWER           3.8462e26

float Body::getLuminosity(const Star& sun,
                          float distanceFromSun) const
{
    // Compute the total power of the star in Watts
    double power = SOLAR_POWER * sun.getLuminosity();

    // Compute the irradiance at a distance of 1au from the star in W/m^2
    // double irradiance = power / sphereArea(astro::AUtoKilometers(1.0) * 1000);

    // Compute the irradiance at the body's distance from the star
    double satIrradiance = power / sphereArea(distanceFromSun * 1000);

    // Compute the total energy hitting the planet
    double incidentEnergy = satIrradiance * circleArea(radius * 1000);

    double reflectedEnergy = incidentEnergy * albedo;
    
    // Compute the luminosity (i.e. power relative to solar power)
    return (float) (reflectedEnergy / SOLAR_POWER);
}


float Body::getApparentMagnitude(const Star& sun,
                                 float distanceFromSun,
                                 float distanceFromViewer) const
{
    return astro::lumToAppMag(getLuminosity(sun, distanceFromSun),
                              astro::kilometersToLightYears(distanceFromViewer));
}


// Return the apparent magnitude of the body, corrected for the phase.
float Body::getApparentMagnitude(const Star& sun,
                                 const Vec3d& sunPosition,
                                 const Vec3d& viewerPosition) const
{
    double distanceToViewer = viewerPosition.length();
    double distanceToSun = sunPosition.length();
    float illuminatedFraction = (float) (1.0 + (viewerPosition / distanceToViewer) *
                                         (sunPosition / distanceToSun)) / 2.0;

    return astro::lumToAppMag(getLuminosity(sun, distanceToSun) * illuminatedFraction,
                              astro::kilometersToLightYears(distanceToViewer));
}



/**** Implementation of PlanetarySystem ****/

PlanetarySystem::PlanetarySystem(Body* _primary) : primary(_primary)
{
    if (primary != NULL && primary->getSystem() != NULL)
        starNumber = primary->getSystem()->getStarNumber();
    else
        starNumber = Star::InvalidCatalogNumber;
}

PlanetarySystem::PlanetarySystem(uint32 _starNumber) :
    starNumber(_starNumber), primary(NULL)
{
}


Body* PlanetarySystem::find(string _name, bool deepSearch) const
{
    for (vector<Body*>::const_iterator iter = satellites.begin();
         iter != satellites.end(); iter++)
    {
        if (compareIgnoringCase((*iter)->getName(), _name) == 0)
        {
            return *iter;
        }
        else if (deepSearch && (*iter)->getSatellites() != NULL)
        {
            Body* body = (*iter)->getSatellites()->find(_name, deepSearch);
            if (body != NULL)
                return body;
        }
    }

    return NULL;
}


bool PlanetarySystem::traverse(TraversalFunc func, void* info) const
{
    for (int i = 0; i < getSystemSize(); i++)
    {
        Body* body = getBody(i);
        // assert(body != NULL);
        if (!func(body, info))
            return false;
        if (body->getSatellites() != NULL)
        {
            if (!body->getSatellites()->traverse(func, info))
                return false;
        }
    }

    return true;
}

