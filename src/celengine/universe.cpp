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
#include "3dsmesh.h"
#include "meshmanager.h"
#include "universe.h"

using namespace std;

#define ANGULAR_RES 3.5e-6

Universe::Universe() :
    starCatalog(NULL),
    solarSystemCatalog(NULL),
    deepSkyCatalog(NULL),
    asterisms(NULL),
    /*boundaries(NULL),*/
    markers(NULL)
{
    markers = new MarkerList();
}

Universe::~Universe()
{
    delete markers;
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


DeepSkyCatalog* Universe::getDeepSkyCatalog() const
{
    return deepSkyCatalog;
}

void Universe::setDeepSkyCatalog(DeepSkyCatalog* catalog)
{
    deepSkyCatalog = catalog;
}


AsterismList* Universe::getAsterisms() const
{
    return asterisms;
}

void Universe::setAsterisms(AsterismList* _asterisms)
{
    asterisms = _asterisms;
}

ConstellationBoundaries* Universe::getBoundaries() const
{
    return boundaries;
}

void Universe::setBoundaries(ConstellationBoundaries* _boundaries)
{
    boundaries = _boundaries;
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


// A more general version of the method above--return the solar system
// that contains an object, or NULL if there is no solar sytstem.
SolarSystem* Universe::getSolarSystem(const Selection& sel) const
{
    switch (sel.getType())
    {
    case Selection::Type_Star:
        return getSolarSystem(sel.star());

    case Selection::Type_Body:
        {
            PlanetarySystem* system = sel.body()->getSystem();
            while (system != NULL)
            {
                Body* parent = system->getPrimaryBody();
                if (parent != NULL)
                    system = parent->getSystem();
                else
                    return getSolarSystem(Selection(system->getStar()));
            }
            return NULL;
        }

    case Selection::Type_Location:
        return getSolarSystem(Selection(sel.location()->getParentBody()));

    default:
        return NULL;
    }
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


MarkerList* Universe::getMarkers() const
{
    return markers;
}


void Universe::markObject(const Selection& sel,
                          float size,
                          Color color,
                          Marker::Symbol symbol,
                          int priority)
{
    for (MarkerList::iterator iter = markers->begin();
         iter != markers->end(); iter++)
    {
        if (iter->getObject() == sel)
        {
            // Handle the case when the object is already marked.  If the
            // priority is higher than the existing marker, replace it.
            // Otherwise, do nothing.
            if (priority > iter->getPriority())
            {
                markers->erase(iter);
                break;
            }
            else
            {
                return;
            }
        }
    }

    Marker marker(sel);
    marker.setColor(color);
    marker.setSymbol(symbol);
    marker.setSize(size);
    marker.setPriority(priority);
    markers->insert(markers->end(), marker);
}


void Universe::unmarkObject(const Selection& sel, int priority)
{
    for (MarkerList::iterator iter = markers->begin();
         iter != markers->end(); iter++)
    {
        if (iter->getObject() == sel)
        {
            if (priority >= iter->getPriority())
                markers->erase(iter);
            break;
        }
    }
}


void Universe::unmarkAll()
{
    markers->erase(markers->begin(), markers->end());
}


bool Universe::isMarked(const Selection& sel, int priority) const
{
    for (MarkerList::iterator iter = markers->begin();
         iter != markers->end(); iter++)
    {
        if (iter->getObject() == sel)
            return iter->getPriority() >= priority;
    }

    return false;
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
    double sinAngle2Closest;
    double closestDistance;
    double closestApproxDistance;
    Body* closestBody;
    Ray3d pickRay;
    double jd;
    float atanTolerance;
};

static bool ApproxPlanetPickTraversal(Body* body, void* info)
{
    PlanetPickInfo* pickInfo = (PlanetPickInfo*) info;

    // Reject invisible bodies
    if (body->getClassification() == Body::Invisible)
        return true;

    Point3d bpos = body->getHeliocentricPosition(pickInfo->jd);
    Vec3d bodyDir = bpos - pickInfo->pickRay.origin;
    double distance = bodyDir.length();

    // Check the apparent radius of the orbit against our tolerance factor.
    // This check exists to make sure than when picking a distant, we select
    // the planet rather than one of its satellites.
    float appOrbitRadius = (float) (body->getOrbit()->getBoundingRadius() /
                                    distance);
    
    if ((pickInfo->atanTolerance > ANGULAR_RES ? pickInfo->atanTolerance: 
        ANGULAR_RES) > appOrbitRadius)
        return true;

    bodyDir.normalize();
    Vec3d bodyMiss = bodyDir - pickInfo->pickRay.direction;
    double sinAngle2 = sqrt(bodyMiss * bodyMiss)/2.0;

    if (sinAngle2 <= pickInfo->sinAngle2Closest)
    {
        pickInfo->sinAngle2Closest = sinAngle2 > ANGULAR_RES ? sinAngle2 : 
	                                         ANGULAR_RES;
        pickInfo->closestBody = body;
        pickInfo->closestApproxDistance = distance;
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
    if (body->getClassification() != Body::Invisible &&
        testIntersection(pickInfo->pickRay, Sphered(bpos, radius), distance))
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
            // Transform rotate the pick ray into object coordinates
            Quatf qf = body->getOrientation();
            Quatd qd(qf.w, qf.x, qf.y, qf.z);
            Mat3d m = conjugate(qd * body->getEclipticalToGeographic(pickInfo->jd)).toMatrix3();
            Ray3d r(Point3d(0, 0, 0) + (pickInfo->pickRay.origin - bpos),
                    pickInfo->pickRay.direction);
            r = r * m;

            // The mesh vertices are normalized, then multiplied by a scale
            // factor.  Thus, the ray needs to be multiplied by the inverse of
            // the mesh scale factor.
            double s = 1.0 / radius;
            r.origin.x *= s;
            r.origin.y *= s;
            r.origin.z *= s;
            r.direction *= s;

            Mesh* mesh = GetMeshManager()->find(body->getMesh());
            if (mesh != NULL)
            {
                if (!mesh->pick(r, distance))
                    distance = -1.0;
            }
        }
        // Make also sure that the pickRay does not intersect the body in the
        // opposite hemisphere! Hence, need again the "bodyMiss" angle

        Vec3d bodyDir = bpos - pickInfo->pickRay.origin;
        bodyDir.normalize();
        Vec3d bodyMiss = bodyDir - pickInfo->pickRay.direction;
        double sinAngle2 = sqrt(bodyMiss * bodyMiss)/2.0;


        if (sinAngle2 < sin(PI/4.0) && distance > 0.0 && 
            distance  <= pickInfo->closestDistance)
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
    double sinTol2 = (sin(tolerance/2.0) >  ANGULAR_RES ? 
	              sin(tolerance/2.0) : ANGULAR_RES);
    PlanetPickInfo pickInfo;

    // Transform the pick direction
    pickInfo.pickRay = Ray3d(astro::heliocentricPosition(origin, 
    solarSystem.getCenter()), Vec3d(direction.x, direction.y, direction.z));
                             
    pickInfo.sinAngle2Closest = 1.0;
    pickInfo.closestDistance = 1.0e50;
    pickInfo.closestApproxDistance = 1.0e50;
    pickInfo.closestBody = NULL;
    pickInfo.jd = when;
    pickInfo.atanTolerance = (float) atan(tolerance);

    // First see if there's a planet|moon that the pick ray intersects.
    // Select the closest planet|moon intersected.

    solarSystem.getPlanets()->traverse(ExactPlanetPickTraversal,
                                       (void*) &pickInfo);

    if (pickInfo.closestBody != NULL)
    {
        // Retain that body
        Body* closestBody = pickInfo.closestBody;

	// Check if there is a satellite in front of the primary body that is
	// sufficiently close to the pickRay 

	solarSystem.getPlanets()->traverse(ApproxPlanetPickTraversal,
					 (void*) &pickInfo);
        if (pickInfo.closestBody == closestBody)
	  return  Selection(closestBody); 
        // Nothing else around, select the body and return

        // Are we close enough to the satellite and is it in front of the body?
	if ((pickInfo.sinAngle2Closest <= sinTol2) && 
            (pickInfo.closestDistance > pickInfo.closestApproxDistance)) 
        return Selection(pickInfo.closestBody);
            // Yes, select the satellite
	else
	    return  Selection(closestBody); 
           //  No, select the primary body 
    }

    // If no planet was intersected by the pick ray, choose the planet|moon
    // with the smallest angular separation from the pick ray.  Very distant
    // planets are likley to fail the intersection test even if the user
    // clicks on a pixel where the planet's disc has been rendered--in order
    // to make distant planets visible on the screen at all, their apparent
    // size has to be greater than their actual disc size.

    solarSystem.getPlanets()->traverse(ApproxPlanetPickTraversal,
                                       (void*) &pickInfo);
    if (pickInfo.sinAngle2Closest <= sinTol2)
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
    double sinAngle2Closest;
};

StarPicker::StarPicker(const Point3f& _pickOrigin, const Vec3f& _pickRay,
                       float angle) :
    pickedStar(NULL),
    pickOrigin(_pickOrigin),
    pickRay(_pickRay),
    sinAngle2Closest(sin(angle/2.0) > ANGULAR_RES ? sin(angle/2.0) : 
                                      ANGULAR_RES )
{
}

void StarPicker::process(const Star& star, float distance, float appMag)
{
    Vec3f starDir = star.getPosition() - pickOrigin;
    starDir.normalize();

    Vec3f starMiss = starDir - pickRay;
    Vec3d sMd = Vec3d(starMiss.x, starMiss.y, starMiss.z); 
    
    double sinAngle2 = sqrt(sMd * sMd)/2.0;

    if (sinAngle2 <= sinAngle2Closest)
    {
        sinAngle2Closest = sinAngle2 > ANGULAR_RES ? sinAngle2 : ANGULAR_RES;
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
    double sinAngle2Closest;
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
    sinAngle2Closest(sin(angle/2.0) > ANGULAR_RES ? sin(angle/2.0) : 
                                      ANGULAR_RES )
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
                sinAngle2Closest = ANGULAR_RES; 
                // An exact hit--set the angle to "zero"
            }
        }
    }
    else
    {
        // We don't have an exact hit; check to see if we're close enough
        float distance = starDir.length();
        starDir.normalize();
        Vec3f starMiss = starDir - pickDir;
        Vec3d sMd = Vec3d(starMiss.x, starMiss.y, starMiss.z ); 
    
        double sinAngle2 = sqrt(sMd * sMd)/2.0;

        if (sinAngle2 <= sinAngle2Closest &&
            (closestStar == NULL || distance < closestDistance))
        {
            closestStar = &star;
            closestDistance = distance;
            sinAngle2Closest = sinAngle2 > ANGULAR_RES ? sinAngle2 : ANGULAR_RES;
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
    Vec3f Missf = n - direction;
    Vec3d Miss = Vec3d(Missf.x, Missf.y, Missf.z); 
    double sinAngle2 = sqrt(Miss * Miss)/2.0;

    if (sinAngle2 <= ANGULAR_RES)
    {
        rotation.setAxisAngle(Vec3f(1, 0, 0), 0);
    }
    else if (sinAngle2 >= 1.0 - 0.5 * ANGULAR_RES * ANGULAR_RES)
    {
        rotation.setAxisAngle(Vec3f(1, 0, 0), (float) PI);
    }
    else
    {
        Vec3f axis = direction ^ n;
        axis.normalize();
        rotation.setAxisAngle(axis, (float) 2.0 * asin(sinAngle2));
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


Selection Universe::pickDeepSkyObject(const UniversalCoord& origin,
                                      const Vec3f& direction,
                                      float faintestMag,
                                      float tolerance)
{
    Selection sel;
    Point3d p = (Point3d) origin;
    Ray3d pickRay(Point3d(p.x * 1e-6, p.y * 1e-6, p.z * 1e-6),
                  Vec3d(direction.x, direction.y, direction.z));
    double closestDistance = 1.0e30;

    if (deepSkyCatalog != NULL)
    {
        for (DeepSkyCatalog::const_iterator iter = deepSkyCatalog->begin();
             iter != deepSkyCatalog->end(); iter++)
        {
            double distance = 0.0;
            if (testIntersection(pickRay, 
                                 Sphered((*iter)->getPosition(), (*iter)->getRadius()),
                                 distance))
            {
                // Don't select an object that contains the origin
                if (pickRay.origin.distanceTo((*iter)->getPosition()) > (*iter)->getRadius() &&
                    distance < closestDistance)
                {
                    closestDistance = distance;
                    sel = Selection(*iter);
                }
            }
        }
    }

    return sel;
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

    if (sel.empty())
        sel = pickDeepSkyObject(origin, direction, faintestMag, tolerance);

    return sel;
}


// Select an object by name, with the following priority:
//   1. Try to look up the name in the star catalog
//   2. Search the deep sky catalog for a matching name.
//   3. Check the solar systems for planet names; we don't make any decisions
//      about which solar systems are relevant, and let the caller pass them
//      to us to search.
Selection Universe::find(const string& s,
                         PlanetarySystem** solarSystems,
                         int nSolarSystems)
{
    if (deepSkyCatalog != NULL)
    {
        for (DeepSkyCatalog::const_iterator iter = deepSkyCatalog->begin();
             iter != deepSkyCatalog->end(); iter++)
        {
            if (compareIgnoringCase((*iter)->getName(), s) == 0)
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

    if (sel.getType() == Selection::Type_DeepSky)
        return Selection();

    const PlanetarySystem* worlds = NULL;
    if (sel.getType() == Selection::Type_Body)
    {
        worlds = sel.body()->getSatellites();
    }
    else if (sel.getType() == Selection::Type_Star)
    {
        SolarSystem* ssys = getSolarSystem(sel.star());
        if (ssys != NULL)
            worlds = ssys->getPlanets();
    }
    else if (sel.getType() == Selection::Type_Location)
    {
        if (sel.location()->getParentBody() != NULL)
        {
            worlds = sel.location()->getParentBody()->getSatellites();
        }
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
            if (nextPos == string::npos)
            {
                if (worlds->getPrimaryBody() != NULL)
                {
                    Location* loc =
                        worlds->getPrimaryBody()->findLocation(name);
                    if (loc != NULL)
                        return Selection(loc);
                }
            }
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

std::vector<std::string> Universe::getCompletion(const string& s,
                         PlanetarySystem** solarSystems,
                         int nSolarSystems)
{
    std::vector<std::string> completion;

    // solar bodies first
    for (int i = 0; i < nSolarSystems; i++)
    {
        if (solarSystems[i] != NULL)
        {
            std::vector<std::string> bodies = solarSystems[i]->getCompletion(s);
            completion.insert(completion.end(), bodies.begin(), bodies.end());
        }
    }

    // Deep sky object
    if (deepSkyCatalog != NULL)
    {
        for (DeepSkyCatalog::const_iterator iter = deepSkyCatalog->begin();
             iter != deepSkyCatalog->end(); iter++)
        {
            if (compareIgnoringCase((*iter)->getName(), s, s.length()) == 0)
                completion.push_back(Selection(*iter).getName());
        }
    }

    // finaly stars;
    std::vector<std::string> stars = starCatalog->getCompletion(s);
    completion.insert(completion.end(), stars.begin(), stars.end());

    return completion;
}

std::vector<std::string> Universe::getCompletionPath(const string& s,
                             PlanetarySystem* solarSystems[],
                             int nSolarSystems)
{
    std::vector<std::string> completion;
    string::size_type pos = s.rfind('/', s.length());

    if (pos == string::npos)
        return getCompletion(s, solarSystems, nSolarSystems);

    string base(s, 0, pos);
    Selection sel = findPath(base, solarSystems, nSolarSystems);

    if (sel.empty())
        return completion;

    if (sel.getType() == Selection::Type_DeepSky)
    {
        completion.push_back(sel.deepsky()->getName());
        return completion;
    }

    PlanetarySystem* worlds = NULL;
    if (sel.getType() == Selection::Type_Body)
    {
        worlds = sel.body()->getSatellites();
    }
    else if (sel.getType() == Selection::Type_Star)
    {
        SolarSystem* ssys = getSolarSystem(sel.star());
        if (ssys != NULL)
            worlds = ssys->getPlanets();
    }

    if (worlds != NULL)
    {
        return worlds->getCompletion(s.substr(pos + 1), false);
    }

    return completion;
}


// Return the closest solar system to position, or NULL if there are no planets
// with in one light year.
SolarSystem* Universe::getNearestSolarSystem(const UniversalCoord& position) const
{
    Point3f pos = (Point3f) position;
    Point3f lyPos(pos.x * 1.0e-6f, pos.y * 1.0e-6f, pos.z * 1.0e-6f);
    ClosestStarFinder closestFinder(1.0f);
    starCatalog->findCloseStars(closestFinder, lyPos, 1.0f);
    return getSolarSystem(closestFinder.closestStar);
}
