// universe.cpp
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// A container for catalogs of galaxies, stars, and planets.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celmath/mathlib.h>
#include <celmath/vecmath.h>
#include <celmath/intersect.h>
#include "astro.h"
#include "universe.h"

using namespace std;


Universe::Universe() :
    starCatalog(NULL),
    solarSystemCatalog(NULL),
    galaxyCatalog(NULL),
    asterisms(NULL)
{
}

Universe::~Universe()
{
    // TODO: Clean up!
}


StarDatabase* Universe::getStarCatalog() const
{
    return starCatalog;
}

void Universe::setStarCatalog(StarDatabase* catalog)
{
    starCatalog = catalog;
}


SolarSystemCatalog* Universe::getSolarSystemCatalog() const
{
    return solarSystemCatalog;
}

void Universe::setSolarSystemCatalog(SolarSystemCatalog* catalog)
{
    solarSystemCatalog = catalog;
}


GalaxyList* Universe::getGalaxyCatalog() const
{
    return galaxyCatalog;
}

void Universe::setGalaxyCatalog(GalaxyList* catalog)
{
    galaxyCatalog = catalog;
}


AsterismList* Universe::getAsterisms() const
{
    return asterisms;
}

void Universe::setAsterisms(AsterismList* _asterisms)
{
    asterisms = _asterisms;
}


// Return the planetary system of a star, or NULL if it has no planets.
SolarSystem* Universe::getSolarSystem(const Star* star) const
{
    if (star == NULL)
        return NULL;

    uint32 starNum = star->getCatalogNumber();
    SolarSystemCatalog::iterator iter = solarSystemCatalog->find(starNum);
    if (iter == solarSystemCatalog->end())
        return NULL;
    else
        return iter->second;
}


// Create a new solar system for a star and return a pointer to it; if it
// already has a solar system, just return a pointer to the existing one.
SolarSystem* Universe::createSolarSystem(Star* star) const
{
    SolarSystem* solarSystem = getSolarSystem(star);
    if (solarSystem != NULL)
        return solarSystem;

    solarSystem = new SolarSystem(star);
    solarSystemCatalog->insert(SolarSystemCatalog::
                               value_type(star->getCatalogNumber(),
                                          solarSystem));

    return solarSystem;
}


class ClosestStarFinder : public StarHandler
{
public:
    ClosestStarFinder(float _maxDistance);
    ~ClosestStarFinder() {};
    void process(const Star& star, float distance, float appMag);

public:
    float maxDistance;
    float closestDistance;
    Star* closestStar;
};

ClosestStarFinder::ClosestStarFinder(float _maxDistance) :
    maxDistance(_maxDistance), closestDistance(_maxDistance), closestStar(NULL)
{
}

void ClosestStarFinder::process(const Star& star, float distance, float appMag)
{
    if (distance < closestDistance)
    {
        closestStar = const_cast<Star*>(&star);
        closestDistance = distance;
    }
}


struct PlanetPickInfo
{
    float cosClosestAngle;
    double closestDistance;
    Body* closestBody;
    Ray3d pickRay;
    double jd;
    float atanTolerance;
};

static bool ApproxPlanetPickTraversal(Body* body, void* info)
{
    PlanetPickInfo* pickInfo = (PlanetPickInfo*) info;

    Point3d bpos = body->getHeliocentricPosition(pickInfo->jd);
    Vec3d bodyDir = bpos - pickInfo->pickRay.origin;

    // Check the apparent radius of the orbit against our tolerance factor.
    // This check exists to make sure than when picking a distant, we select
    // the planet rather than one of its satellites.
    float appOrbitRadius = (float) (body->getOrbit()->getBoundingRadius() /
                                    bodyDir.length());
    if (pickInfo->atanTolerance > appOrbitRadius)
        return true;

    bodyDir.normalize();
    double cosAngle = bodyDir * pickInfo->pickRay.direction;
    if (cosAngle > pickInfo->cosClosestAngle)
    {
        pickInfo->cosClosestAngle = cosAngle;
        pickInfo->closestBody = body;
    }

    return true;
}


// Perform an intersection test between the pick ray and a body
static bool ExactPlanetPickTraversal(Body* body, void* info)
{
    PlanetPickInfo* pickInfo = reinterpret_cast<PlanetPickInfo*>(info);
    Point3d bpos = body->getHeliocentricPosition(pickInfo->jd);
    float radius = body->getRadius();
    double distance = -1.0;

    // Test for intersection with the bounding sphere
    if (testIntersection(pickInfo->pickRay, Sphered(bpos, radius), distance))
    {
        if (body->getMesh() == InvalidResource)
        {
            // There's no mesh, so the object is an ellipsoid.  If it's
            // oblate, do a ray intersection test to see if the object was
            // picked.  Otherwise, the object is spherical and we've already
            // done all the work we need to.
            if (body->getOblateness() != 0.0f)
            {
                // Oblate sphere; use ray ellipsoid intersection calculation
                Vec3d ellipsoidAxes(radius,
                                    radius * (1.0 - body->getOblateness()),
                                    radius);
                // Transform rotate the pick ray into object coordinates
                Mat3d m = conjugate(body->getEclipticalToEquatorial(pickInfo->jd)).toMatrix3();
                Ray3d r(Point3d(0, 0, 0) + (pickInfo->pickRay.origin - bpos),
                        pickInfo->pickRay.direction);
                r = r * m;
                if (!testIntersection(r, Ellipsoidd(ellipsoidAxes), distance))
                    distance = -1.0;
            }
        }
        else
        {
            // TODO: Check for intersection with the object mesh.  For now,
            // we just use the bounding sphere.
        }

        if (distance > 0.0 && distance < pickInfo->closestDistance)
        {
            pickInfo->closestDistance = distance;
            pickInfo->closestBody = body;
        }
    }

    return true;
}


Selection Universe::pickPlanet(SolarSystem& solarSystem,
                               const UniversalCoord& origin,
                               const Vec3f& direction,
                               double when,
                               float faintestMag,
                               float tolerance)
{
    PlanetPickInfo pickInfo;

    // Transform the pick direction
    pickInfo.pickRay = Ray3d(astro::heliocentricPosition(origin, solarSystem.getCenter()), Vec3d(direction.x, direction.y, direction.z));
                             
    pickInfo.cosClosestAngle = -1.0f;
    pickInfo.closestDistance = 1.0e50;
    pickInfo.closestBody = NULL;
    pickInfo.jd = when;
    pickInfo.atanTolerance = (float) atan(tolerance);

    // First see if there's a planet that the pick ray intersects.
    // Select the closest planet intersected.
    solarSystem.getPlanets()->traverse(ExactPlanetPickTraversal,
                                       (void*) &pickInfo);
    if (pickInfo.closestBody != NULL)
        return Selection(pickInfo.closestBody);

    // If no planet was intersected by the pick ray, choose the planet
    // with the smallest angular separation from the pick ray.  Very distant
    // planets are likley to fail the intersection test even if the user
    // clicks on a pixel where the planet's disc has been rendered--in order
    // to make distant planets visible on the screen at all, their apparent
    // size has to be greater than their actual disc size.
    solarSystem.getPlanets()->traverse(ApproxPlanetPickTraversal,
                                       (void*) &pickInfo);
    if (pickInfo.cosClosestAngle > cos(tolerance))
        return Selection(pickInfo.closestBody);
    else
        return Selection();
}


// StarPicker is a callback class for StarDatabase::findVisibleStars
class StarPicker : public StarHandler
{
public:
    StarPicker(const Point3f&, const Vec3f&, float);
    ~StarPicker() {};

    void process(const Star&, float, float);

public:
    const Star* pickedStar;
    Point3f pickOrigin;
    Vec3f pickRay;
    float cosAngleClosest;
};

StarPicker::StarPicker(const Point3f& _pickOrigin, const Vec3f& _pickRay,
                       float angle) :
    pickedStar(NULL),
    pickOrigin(_pickOrigin),
    pickRay(_pickRay),
    cosAngleClosest((float) cos(angle))
{
}

void StarPicker::process(const Star& star, float distance, float appMag)
{
    Vec3f starDir = star.getPosition() - pickOrigin;
    starDir.normalize();

    float cosAngle = starDir * pickRay;
    if (cosAngle > cosAngleClosest)
    {
        cosAngleClosest = cosAngle;
        pickedStar = &star;
    }
}


class CloseStarPicker : public StarHandler
{
public:
    CloseStarPicker(const UniversalCoord& pos,
                    const Vec3f& dir,
                    float _maxDistance,
                    float angle);
    ~CloseStarPicker() {};
    void process(const Star& star, float distance, float appMag);

public:
    UniversalCoord pickOrigin;
    Vec3f pickDir;
    float maxDistance;
    const Star* closestStar;
    float closestDistance;
    float cosAngleClosest;
};


CloseStarPicker::CloseStarPicker(const UniversalCoord& pos,
                                 const Vec3f& dir,
                                 float _maxDistance,
                                 float angle) :
    pickOrigin(pos),
    pickDir(dir),
    maxDistance(_maxDistance),
    closestStar(NULL),
    closestDistance(0.0f),
    cosAngleClosest((float) cos(angle))
{
}

void CloseStarPicker::process(const Star& star,
                              float lowPrecDistance,
                              float appMag)
{
    if (lowPrecDistance > maxDistance)
        return;

    Point3d hPos = astro::heliocentricPosition(pickOrigin, star.getPosition());
    Vec3f starDir((float) -hPos.x, (float) -hPos.y, (float) -hPos.z);
    float distance = 0.0f;

     if (testIntersection(Ray3f(Point3f(0, 0, 0), pickDir),
                         Spheref(Point3f(starDir.x, starDir.y, starDir.z),
                                 star.getRadius()), distance))
    {
        if (distance > 0.0f)
        {
            if (closestStar == NULL || distance < closestDistance)
            {
                closestStar = &star;
                closestDistance = starDir.length();
                cosAngleClosest = 1.0f; // An exact hit--set the angle to zero
            }
        }
    }
    else
    {
        // We don't have an exact hit; check to see if we're close enough
        float distance = starDir.length();
        starDir *= (1.0f / distance);
        float cosAngle = starDir * pickDir;
        if (cosAngle > cosAngleClosest &&
            (closestStar == NULL || distance < closestDistance))
        {
            closestStar = &star;
            closestDistance = distance;
            cosAngleClosest = cosAngle;
        }
    }
}


Selection Universe::pickStar(const UniversalCoord& origin,
                             const Vec3f& direction,
                             float faintestMag,
                             float tolerance)
{
    Point3f o = (Point3f) origin;
    o.x *= 1e-6f;
    o.y *= 1e-6f;
    o.z *= 1e-6f;

    // Use a high precision pick test for any stars that are close to the
    // observer.  If this test fails, use a low precision pick test for stars
    // which are further away.  All this work is necessary because the low
    // precision pick test isn't reliable close to a star and the high
    // precision test isn't nearly fast enough to use on our database of
    // over 100k stars.
    CloseStarPicker closePicker(origin, direction, 1.0f, tolerance);
    starCatalog->findCloseStars(closePicker, o, 1.0f);
    if (closePicker.closestStar != NULL)
        return Selection(const_cast<Star*>(closePicker.closestStar));

    // Find visible stars expects an orientation, but we just have a direction
    // vector.  Convert the direction vector into an orientation by computing
    // the rotation required to map (0, 0, -1) to the direction.
    Quatf rotation;
    Vec3f n(0, 0, -1);
    float epsilon = 1.0e-6f;
    float cosAngle = n * direction;
    if (cosAngle > 1.0f - epsilon)
    {
        rotation.setAxisAngle(Vec3f(1, 0, 0), 0);
    }
    else if (cosAngle < epsilon - 1.0f)
    {
        rotation.setAxisAngle(Vec3f(1, 0, 0), (float) PI);
    }
    else
    {
        Vec3f axis = direction ^ n;
        axis.normalize();
        rotation.setAxisAngle(axis, (float) acos(cosAngle));
    }

    StarPicker picker(o, direction, tolerance);
    starCatalog->findVisibleStars(picker,
                                  o,
                                  rotation,
                                  tolerance, 1.0f,
                                  faintestMag);
    if (picker.pickedStar != NULL)
        return Selection(const_cast<Star*>(picker.pickedStar));
    else
        return Selection();
}


Selection Universe::pick(const UniversalCoord& origin,
                         const Vec3f& direction,
                         double when,
                         float faintestMag,
                         float tolerance)
{
    Selection sel;

    SolarSystem* closestSolarSystem = getNearestSolarSystem(origin);
    if (closestSolarSystem != NULL)
    {
        sel = pickPlanet(*closestSolarSystem,
                         origin, direction,
                         when,
                         faintestMag,
                         tolerance);
    }

    if (sel.empty())
        sel = pickStar(origin, direction, faintestMag, tolerance);

    return sel;
}


// Select an object by name, with the following priority:
//   1. Try to look up the name in the star catalog
//   2. Search the galaxy catalog for a matching name.
//   3. Check the solar systems for planet names; we don't make any decisions
//      about which solar systems are relevant, and let the caller pass them
//      to us to search.
Selection Universe::find(const string& s,
                         PlanetarySystem** solarSystems,
                         int nSolarSystems)
{
    if (galaxyCatalog != NULL)
    {
        for (GalaxyList::const_iterator iter = galaxyCatalog->begin();
             iter != galaxyCatalog->end(); iter++)
        {
            if ((*iter)->getName() == s)
                return Selection(*iter);
        }
    }

    Star* star = starCatalog->find(s);
    if (star != NULL)
        return Selection(star);

    for (int i = 0; i < nSolarSystems; i++)
    {
        if (solarSystems[i] != NULL)
        {
            Body* body = solarSystems[i]->find(s, true);
            if (body != NULL)
                return Selection(body);
        }
    }

    return Selection();
}


// Find an object from a path, for example Sol/Earth/Moon or Upsilon And/b
// Currently, 'absolute' paths starting with a / are not supported nor are
// paths that contain galaxies.  The caller may pass in a list of solar systems
// to search for objects--this is roughly analgous to the PATH environment
// variable in Unix and Windows.  Typically, the solar system will be one
// in which the user is currently located.
Selection Universe::findPath(const string& s,
                             PlanetarySystem* solarSystems[],
                             int nSolarSystems)
{
    string::size_type pos = s.find('/', 0);

    if (pos == string::npos)
        return find(s, solarSystems, nSolarSystems);

    string base(s, 0, pos);
    Selection sel = find(base, solarSystems, nSolarSystems);
    if (sel.empty())
        return sel;

    // Don't support paths relative to a galaxy . . . for now.
    if (sel.galaxy != NULL)
        return Selection();

    const PlanetarySystem* worlds = NULL;
    if (sel.body != NULL)
    {
        worlds = sel.body->getSatellites();
    }
    else if (sel.star != NULL)
    {
        SolarSystem* ssys = getSolarSystem(sel.star);
        if (ssys != NULL)
            worlds = ssys->getPlanets();
    }

    while (worlds != NULL)
    {
        string::size_type nextPos = s.find('/', pos + 1);
        string::size_type len;
        if (nextPos == string::npos)
            len = s.size() - pos - 1;
        else
            len = nextPos - pos - 1;

        string name = string(s, pos + 1, len);
        
        Body* body = worlds->find(name);
        if (body == NULL)
        {
            return Selection();
        }
        else if (nextPos == string::npos)
        {
            return Selection(body);
        }
        else
        {
            worlds = body->getSatellites();
            pos = nextPos;
        }
    }

    return Selection();
}


// Return the closest solar system to position, or NULL if there are no planets
// with in one light year.
SolarSystem* Universe::getNearestSolarSystem(const UniversalCoord& position) const
{
    Point3f pos = (Point3f) position;
    Point3f lyPos(pos.x * 1e-6, pos.y * 1e-6, pos.z * 1e-6);
    ClosestStarFinder closestFinder(1.0f);
    starCatalog->findCloseStars(closestFinder, lyPos, 1.0f);
    return getSolarSystem(closestFinder.closestStar);
}
