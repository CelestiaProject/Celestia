// universe.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// A container for catalogs of galaxies, stars, and planets.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "render.h"
#include "astro.h"
#include "asterism.h"
#include "boundaries.h"
#include "meshmanager.h"
#include "universe.h"
#include "timelinephase.h"
#include "frametree.h"
#include <celmath/mathlib.h>
#include <celmath/intersect.h>
#include <celutil/utf8.h>
#include <cassert>

static const double ANGULAR_RES = 3.5e-6;

using namespace Eigen;
using namespace std;
using namespace celmath;


Universe::Universe()
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


DSODatabase* Universe::getDSOCatalog() const
{
    return dsoCatalog;
}

void Universe::setDSOCatalog(DSODatabase* catalog)
{
    dsoCatalog = catalog;
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

// Return the planetary system of a star, or nullptr if it has no planets.
SolarSystem* Universe::getSolarSystem(const Star* star) const
{
    if (star == nullptr)
        return nullptr;

    uint32_t starNum = star->getMainIndexNumber();
    auto iter = solarSystemCatalog->find(starNum);
    if (iter != solarSystemCatalog->end())
        return iter->second;

    return nullptr;
}


// A more general version of the method above--return the solar system
// that contains an object, or nullptr if there is no solar sytstem.
SolarSystem* Universe::getSolarSystem(const Selection& sel) const
{
    switch (sel.getType())
    {
    case Selection::Type_Star:
        return getSolarSystem(sel.star());

    case Selection::Type_Body:
        {
            PlanetarySystem* system = sel.body()->getSystem();
            while (system != nullptr)
            {
                Body* parent = system->getPrimaryBody();
                if (parent != nullptr)
                    system = parent->getSystem();
                else
                    return getSolarSystem(Selection(system->getStar()));
            }
            return nullptr;
        }

    case Selection::Type_Location:
        return getSolarSystem(Selection(sel.location()->getParentBody()));

    default:
        return nullptr;
    }
}


// Create a new solar system for a star and return a pointer to it; if it
// already has a solar system, just return a pointer to the existing one.
SolarSystem* Universe::createSolarSystem(Star* star) const
{
    SolarSystem* solarSystem = getSolarSystem(star);
    if (solarSystem != nullptr)
        return solarSystem;

    solarSystem = new SolarSystem(star);
    solarSystemCatalog->insert(SolarSystemCatalog::
                               value_type(star->getMainIndexNumber(),
                                          solarSystem));

    return solarSystem;
}


MarkerList* Universe::getMarkers() const
{
    return markers;
}


void Universe::markObject(const Selection& sel,
                          const MarkerRepresentation& rep,
                          int priority,
                          bool occludable,
                          MarkerSizing sizing)
{
    auto iter = std::find_if(markers->begin(), markers->end(),
                             [&sel](Marker& m) { return m.object() == sel; });
    if (iter != markers->end())
    {
        // Handle the case when the object is already marked.  If the
        // priority is higher or equal to the existing marker, replace it.
        // Otherwise, do nothing.
        if (priority >= iter->priority())
            markers->erase(iter);
        else
            return;
    }

    Marker marker(sel);
    marker.setRepresentation(rep);
    marker.setPriority(priority);
    marker.setOccludable(occludable);
    marker.setSizing(sizing);
    markers->push_back(marker);
}


void Universe::unmarkObject(const Selection& sel, int priority)
{
    auto iter = std::find_if(markers->begin(), markers->end(),
                            [&sel](Marker& m) { return m.object() == sel; });
    if (iter != markers->end() && priority >= iter->priority())
        markers->erase(iter);
}


void Universe::unmarkAll()
{
    markers->erase(markers->begin(), markers->end());
}


bool Universe::isMarked(const Selection& sel, int priority) const
{
    auto iter = std::find_if(markers->begin(), markers->end(),
                             [&sel](Marker& m) { return m.object() == sel; });
    if (iter != markers->end())
        return iter->priority() >= priority;

    return false;
}


class ClosestStarFinder : public StarHandler
{
public:
    ClosestStarFinder(float _maxDistance, const Universe* _universe);
    ~ClosestStarFinder() = default;
    void process(const Star& star, float distance, float appMag);

public:
    float maxDistance;
    float closestDistance;
    Star* closestStar;
    const Universe* universe;
    bool withPlanets;
};

ClosestStarFinder::ClosestStarFinder(float _maxDistance,
                                     const Universe* _universe) :
    maxDistance(_maxDistance),
    closestDistance(_maxDistance),
    closestStar(nullptr),
    universe(_universe),
    withPlanets(false)
{
}

void ClosestStarFinder::process(const Star& star, float distance, float /*unused*/)
{
    if (distance < closestDistance)
    {
        if (!withPlanets || universe->getSolarSystem(&star))
        {
            closestStar = const_cast<Star*>(&star);
            closestDistance = distance;
        }
    }
}


class NearStarFinder : public StarHandler
{
public:
    NearStarFinder(float _maxDistance, vector<const Star*>& nearStars);
    ~NearStarFinder() = default;
    void process(const Star& star, float distance, float appMag);

private:
    float maxDistance;
    vector<const Star*>& nearStars;
};

NearStarFinder::NearStarFinder(float _maxDistance,
                               vector<const Star*>& _nearStars) :
    maxDistance(_maxDistance),
    nearStars(_nearStars)
{
}

void NearStarFinder::process(const Star& star, float distance, float /*unused*/)
{
    if (distance < maxDistance)
        nearStars.push_back(&star);
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
    auto* pickInfo = (PlanetPickInfo*) info;

    // Reject invisible bodies and bodies that don't exist at the current time
    if (!body->isVisible() || !body->extant(pickInfo->jd) || !body->isClickable())
        return true;

    Vector3d bpos = body->getAstrocentricPosition(pickInfo->jd);
    Vector3d bodyDir = bpos - pickInfo->pickRay.origin;
    double distance = bodyDir.norm();

    // Check the apparent radius of the orbit against our tolerance factor.
    // This check exists to make sure than when picking a distant, we select
    // the planet rather than one of its satellites.
    float appOrbitRadius = (float) (body->getOrbit(pickInfo->jd)->getBoundingRadius() / distance);

    if (std::max((double) pickInfo->atanTolerance, ANGULAR_RES) > appOrbitRadius)
    {
        return true;
    }

    bodyDir.normalize();
    Vector3d bodyMiss = bodyDir - pickInfo->pickRay.direction;
    double sinAngle2 = bodyMiss.norm() / 2.0;

    if (sinAngle2 <= pickInfo->sinAngle2Closest)
    {
        pickInfo->sinAngle2Closest = std::max(sinAngle2, ANGULAR_RES);
        pickInfo->closestBody = body;
        pickInfo->closestApproxDistance = distance;
    }

    return true;
}


// Perform an intersection test between the pick ray and a body
static bool ExactPlanetPickTraversal(Body* body, void* info)
{
    auto* pickInfo = reinterpret_cast<PlanetPickInfo*>(info);
    Vector3d bpos = body->getAstrocentricPosition(pickInfo->jd);
    float radius = body->getRadius();
    double distance = -1.0;

    // Test for intersection with the bounding sphere
    if (body->isVisible() &&
        body->extant(pickInfo->jd) &&
        body->isClickable() &&
        testIntersection(pickInfo->pickRay, Sphered(bpos, radius), distance))
    {
        if (body->getGeometry() == InvalidResource)
        {
            // There's no mesh, so the object is an ellipsoid.  If it's
            // spherical, we've already done all the work we need to. Otherwise,
            // we need to perform a ray-ellipsoid intersection test.
            if (!body->isSphere())
            {
                Vector3d ellipsoidAxes = body->getSemiAxes().cast<double>();

                // Transform rotate the pick ray into object coordinates
                Matrix3d m = body->getEclipticToEquatorial(pickInfo->jd).toRotationMatrix();
                Ray3d r(pickInfo->pickRay.origin - bpos, pickInfo->pickRay.direction);
                r = r.transform(m);
                if (!testIntersection(r, Ellipsoidd(ellipsoidAxes), distance))
                    distance = -1.0;
            }
        }
        else
        {
            // Transform rotate the pick ray into object coordinates
            Quaterniond qd = body->getGeometryOrientation().cast<double>();
            Matrix3d m = (qd * body->getEclipticToBodyFixed(pickInfo->jd)).toRotationMatrix();
            Ray3d r(pickInfo->pickRay.origin - bpos, pickInfo->pickRay.direction);
            r = r.transform(m);

            Geometry* geometry = GetGeometryManager()->find(body->getGeometry());
            float scaleFactor = body->getGeometryScale();
            if (geometry != nullptr && geometry->isNormalized())
                scaleFactor = radius;

            // The mesh vertices are normalized, then multiplied by a scale
            // factor.  Thus, the ray needs to be multiplied by the inverse of
            // the mesh scale factor.
            double is = 1.0 / scaleFactor;
            r.origin *= is;
            r.direction *= is;

            if (geometry != nullptr)
            {
                if (!geometry->pick(r, distance))
                    distance = -1.0;
            }
        }
        // Make also sure that the pickRay does not intersect the body in the
        // opposite hemisphere! Hence, need again the "bodyMiss" angle

        Vector3d bodyDir = bpos - pickInfo->pickRay.origin;
        bodyDir.normalize();
        Vector3d bodyMiss = bodyDir - pickInfo->pickRay.direction;
        double sinAngle2 = bodyMiss.norm() / 2.0;


        if (sinAngle2 < sin(PI/4.0) && distance > 0.0 &&
            distance  <= pickInfo->closestDistance)
        {
            pickInfo->closestDistance = distance;
            pickInfo->closestBody = body;
        }
    }

    return true;
}

// Recursively traverse a frame tree; call the specified callback function for each
// body in the tree. The callback function returns a boolean indicating whether
// traversal should continue.
//
// TODO: This function works, but could use some cleanup:
//   * Make it a member of the frame tree class
//   * Combine info and func into a traversal callback class
static bool traverseFrameTree(FrameTree* frameTree,
                              double tdb,
                              PlanetarySystem::TraversalFunc func,
                              void* info)
{
    for (unsigned int i = 0; i < frameTree->childCount(); i++)
    {
        auto phase = frameTree->getChild(i);
        if (phase->includes(tdb))
        {
            Body* body = phase->body();
            if (!func(body, info))
                return false;

            if (body->getFrameTree() != nullptr)
            {
                if (!traverseFrameTree(body->getFrameTree(), tdb, func, info))
                    return false;
            }
        }
    }

    return true;
}


Selection Universe::pickPlanet(SolarSystem& solarSystem,
                               const UniversalCoord& origin,
                               const Vector3f& direction,
                               double when,
                               float /*faintestMag*/,
                               float tolerance)
{
    double sinTol2 = std::max(sin(tolerance / 2.0), ANGULAR_RES);
    PlanetPickInfo pickInfo;

    Star* star = solarSystem.getStar();
    assert(star != nullptr);

    // Transform the pick ray origin into astrocentric coordinates
    Vector3d astrocentricOrigin = origin.offsetFromKm(star->getPosition(when));

    pickInfo.pickRay = Ray3d(astrocentricOrigin, direction.cast<double>());
    pickInfo.sinAngle2Closest = 1.0;
    pickInfo.closestDistance = 1.0e50;
    pickInfo.closestApproxDistance = 1.0e50;
    pickInfo.closestBody = nullptr;
    pickInfo.jd = when;
    pickInfo.atanTolerance = (float) atan(tolerance);

    // First see if there's a planet|moon that the pick ray intersects.
    // Select the closest planet|moon intersected.
    traverseFrameTree(solarSystem.getFrameTree(), when, ExactPlanetPickTraversal, (void*) &pickInfo);

    if (pickInfo.closestBody != nullptr)
    {
        // Retain that body
        Body* closestBody = pickInfo.closestBody;

        // Check if there is a satellite in front of the primary body that is
        // sufficiently close to the pickRay
        traverseFrameTree(solarSystem.getFrameTree(), when, ApproxPlanetPickTraversal, (void*) &pickInfo);

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
    traverseFrameTree(solarSystem.getFrameTree(), when, ApproxPlanetPickTraversal, (void*) &pickInfo);

    if (pickInfo.sinAngle2Closest <= sinTol2)
        return Selection(pickInfo.closestBody);
    else
        return Selection();
}


// StarPicker is a callback class for StarDatabase::findVisibleStars
class StarPicker : public StarHandler
{
public:
    StarPicker(const Vector3f&, const Vector3f&, double, float);
    ~StarPicker() = default;

    void process(const Star& /*star*/, float /*unused*/, float /*unused*/);

public:
    const Star* pickedStar;
    Vector3f pickOrigin;
    Vector3f pickRay;
    double sinAngle2Closest;
    double when;
};

StarPicker::StarPicker(const Vector3f& _pickOrigin,
                       const Vector3f& _pickRay,
                       double _when,
                       float angle) :
    pickedStar(nullptr),
    pickOrigin(_pickOrigin),
    pickRay(_pickRay),
    sinAngle2Closest(std::max(sin(angle / 2.0), ANGULAR_RES)),
    when(_when)
{
}

void StarPicker::process(const Star& star, float /*unused*/, float /*unused*/)
{
    Vector3f relativeStarPos = star.getPosition() - pickOrigin;
    Vector3f starDir = relativeStarPos.normalized();

    double sinAngle2 = 0.0;

    // Stars with orbits need special handling
    float orbitalRadius = star.getOrbitalRadius();
    if (orbitalRadius != 0.0f)
    {
        float distance = 0.0f;

        // Check for an intersection with orbital bounding sphere; if there's
        // no intersection, then just use normal calculation.  We actually test
        // intersection with a larger sphere to make sure we don't miss a star
        // right on the edge of the sphere.
        if (testIntersection(Ray3f(Vector3f::Zero(), pickRay),
                             Spheref(relativeStarPos, orbitalRadius * 2.0f),
                             distance))
        {
            Vector3d starPos = star.getPosition(when).toLy();
            starDir = (starPos - pickOrigin.cast<double>()).cast<float>().normalized();
        }
    }

    Vector3f starMiss = starDir - pickRay;
    Vector3d sMd = starMiss.cast<double>();
    sinAngle2 = sMd.norm() / 2.0;

    if (sinAngle2 <= sinAngle2Closest)
    {
        sinAngle2Closest = std::max(sinAngle2, ANGULAR_RES);
        pickedStar = &star;
        if (pickedStar->getOrbitBarycenter() != nullptr)
            pickedStar = pickedStar->getOrbitBarycenter();
    }
}


class CloseStarPicker : public StarHandler
{
public:
    CloseStarPicker(const UniversalCoord& pos,
                    const Vector3f& dir,
                    double t,
                    float _maxDistance,
                    float angle);
    ~CloseStarPicker() = default;
    void process(const Star& star, float lowPrecDistance, float appMag);

public:
    UniversalCoord pickOrigin;
    Vector3f pickDir;
    double now;
    float maxDistance;
    const Star* closestStar;
    float closestDistance;
    double sinAngle2Closest;
};


CloseStarPicker::CloseStarPicker(const UniversalCoord& pos,
                                 const Vector3f& dir,
                                 double t,
                                 float _maxDistance,
                                 float angle) :
    pickOrigin(pos),
    pickDir(dir),
    now(t),
    maxDistance(_maxDistance),
    closestStar(nullptr),
    closestDistance(0.0f),
    sinAngle2Closest(std::max(sin(angle/2.0), ANGULAR_RES))
{
}

void CloseStarPicker::process(const Star& star,
                              float lowPrecDistance,
                              float /*unused*/)
{
    if (lowPrecDistance > maxDistance)
        return;

    Vector3d hPos = star.getPosition(now).offsetFromKm(pickOrigin);
    Vector3f starDir = hPos.cast<float>();

    float distance = 0.0f;

     if (testIntersection(Ray3f(Vector3f::Zero(), pickDir),
                          Spheref(starDir, star.getRadius()), distance))
    {
        if (distance > 0.0f)
        {
            if (closestStar == nullptr || distance < closestDistance)
            {
                closestStar = &star;
                closestDistance = starDir.norm();
                sinAngle2Closest = ANGULAR_RES;
                // An exact hit--set the angle to "zero"
            }
        }
    }
    else
    {
        // We don't have an exact hit; check to see if we're close enough
        float distance = starDir.norm();
        starDir.normalize();
        Vector3f starMiss = starDir - pickDir;
        Vector3d sMd = starMiss.cast<double>();

        double sinAngle2 = sMd.norm() / 2.0;

        if (sinAngle2 <= sinAngle2Closest &&
            (closestStar == nullptr || distance < closestDistance))
        {
            closestStar = &star;
            closestDistance = distance;
            sinAngle2Closest = std::max(sinAngle2, ANGULAR_RES);
        }
    }
}


Selection Universe::pickStar(const UniversalCoord& origin,
                             const Vector3f& direction,
                             double when,
                             float faintestMag,
                             float tolerance)
{
    Vector3f o = origin.toLy().cast<float>();

    // Use a high precision pick test for any stars that are close to the
    // observer.  If this test fails, use a low precision pick test for stars
    // which are further away.  All this work is necessary because the low
    // precision pick test isn't reliable close to a star and the high
    // precision test isn't nearly fast enough to use on our database of
    // over 100k stars.
    CloseStarPicker closePicker(origin, direction, when, 1.0f, tolerance);
    starCatalog->findCloseStars(closePicker, o, 1.0f);
    if (closePicker.closestStar != nullptr)
        return Selection(const_cast<Star*>(closePicker.closestStar));

    // Find visible stars expects an orientation, but we just have a direction
    // vector.  Convert the direction vector into an orientation by computing
    // the rotation required to map -Z to the direction.
    Quaternionf rotation;
    rotation.setFromTwoVectors(-Vector3f::UnitZ(), direction);

    StarPicker picker(o, direction, when, tolerance);
    starCatalog->findVisibleStars(picker,
                                  o,
                                  rotation.conjugate(),
                                  tolerance, 1.0f,
                                  faintestMag);
    if (picker.pickedStar != nullptr)
        return Selection(const_cast<Star*>(picker.pickedStar));
    else
        return Selection();
}


class DSOPicker : public DSOHandler
{
public:
    DSOPicker(const Vector3d& pickOrigin, const Vector3d& pickDir, uint64_t renderFlags, float angle);
    ~DSOPicker() = default;

    void process(DeepSkyObject* const &, double, float);

public:
    Vector3d pickOrigin;
    Vector3d pickDir;
    uint64_t renderFlags;

    const DeepSkyObject* pickedDSO;
    double  sinAngle2Closest;
};


DSOPicker::DSOPicker(const Vector3d& pickOrigin,
                     const Vector3d& pickDir,
                     uint64_t renderFlags,
                     float angle) :
    pickOrigin      (pickOrigin),
    pickDir         (pickDir),
    renderFlags     (renderFlags),
    pickedDSO       (nullptr),
    sinAngle2Closest(std::max(sin(angle / 2.0), ANGULAR_RES))
{
}


void DSOPicker::process(DeepSkyObject* const & dso, double /*unused*/, float /*unused*/)
{
    if (!(dso->getRenderMask() & renderFlags) || !dso->isVisible() || !dso->isClickable())
        return;

    Vector3d relativeDSOPos = dso->getPosition() - pickOrigin;
    Vector3d dsoDir = relativeDSOPos;

    double distance2 = 0.0;
    if (testIntersection(Ray3d(Vector3d::Zero(), pickDir),
                         Sphered(relativeDSOPos, (double) dso->getRadius()), distance2))
    {
        Vector3d dsoPos = dso->getPosition();
        dsoDir = dsoPos * 1.0e-6 - pickOrigin;
    }
    dsoDir.normalize();

    Vector3d dsoMissd   = dsoDir - pickDir;
    double sinAngle2 = dsoMissd.norm() / 2.0;

    if (sinAngle2 <= sinAngle2Closest)
    {
        sinAngle2Closest = std::max(sinAngle2, ANGULAR_RES);
        pickedDSO        = dso;
    }
}


class CloseDSOPicker : public DSOHandler
{
public:
    CloseDSOPicker(const  Vector3d& pos,
                   const  Vector3d& dir,
                   uint64_t renderFlags,
                   double maxDistance,
                   float);
    ~CloseDSOPicker() = default;

    void process(DeepSkyObject* const & dso, double distance, float appMag);

public:
    Vector3d  pickOrigin;
    Vector3d  pickDir;
    uint64_t renderFlags;
    double    maxDistance;

    const DeepSkyObject* closestDSO;
    double largestCosAngle;
};


CloseDSOPicker::CloseDSOPicker(const Vector3d& pos,
                               const Vector3d& dir,
                               uint64_t renderFlags,
                               double maxDistance,
                               float /*unused*/) :
    pickOrigin     (pos),
    pickDir        (dir),
    renderFlags    (renderFlags),
    maxDistance    (maxDistance),
    closestDSO     (nullptr),
    largestCosAngle(-2.0)
{
}


void CloseDSOPicker::process(DeepSkyObject* const & dso,
                             double distance,
                             float /*unused*/)
{
    if (distance > maxDistance || !(dso->getRenderMask() & renderFlags) || !dso->isVisible() || !dso->isClickable())
        return;

    double  distanceToPicker       = 0.0;
    double  cosAngleToBoundCenter  = 0.0;
    if (dso->pick(Ray3d(pickOrigin, pickDir), distanceToPicker, cosAngleToBoundCenter))
    {
        // Don't select the object the observer is currently in:
        if ((pickOrigin - dso->getPosition()).norm() > dso->getRadius() &&
            cosAngleToBoundCenter > largestCosAngle)
        {
            closestDSO      = dso;
            largestCosAngle = cosAngleToBoundCenter;
        }
    }
}


Selection Universe::pickDeepSkyObject(const UniversalCoord& origin,
                                      const Vector3f& direction,
                                      uint64_t renderFlags,
                                      float faintestMag,
                                      float tolerance)
{
    Vector3d orig = origin.toLy();
    Vector3d dir = direction.cast<double>();

    CloseDSOPicker closePicker(orig, dir, renderFlags, 1e9, tolerance);

    dsoCatalog->findCloseDSOs(closePicker, orig, 1e9);
    if (closePicker.closestDSO != nullptr)
    {
        return Selection(const_cast<DeepSkyObject*>(closePicker.closestDSO));
    }

    Quaternionf rotation;
    rotation.setFromTwoVectors(-Vector3f::UnitZ(), direction);

    DSOPicker picker(orig, dir, renderFlags, tolerance);
    dsoCatalog->findVisibleDSOs(picker,
                                orig,
                                rotation.conjugate(),
                                tolerance,
                                1.0f,
                                faintestMag);
    if (picker.pickedDSO != nullptr)
        return Selection(const_cast<DeepSkyObject*>(picker.pickedDSO));
    else
        return Selection();
}


Selection Universe::pick(const UniversalCoord& origin,
                         const Vector3f& direction,
                         double when,
                         uint64_t renderFlags,
                         float  faintestMag,
                         float  tolerance)
{
    Selection sel;

    if (renderFlags & Renderer::ShowPlanets)
    {
        closeStars.clear();
        getNearStars(origin, 1.0f, closeStars);
        for (const auto star : closeStars)
        {
            SolarSystem* solarSystem = getSolarSystem(star);
            if (solarSystem != nullptr)
            {
                sel = pickPlanet(*solarSystem,
                                origin, direction,
                                when,
                                faintestMag,
                                tolerance);
                if (!sel.empty())
                    break;
            }
        }
    }

    if (sel.empty() && (renderFlags & Renderer::ShowStars))
    {
        sel = pickStar(origin, direction, when, faintestMag, tolerance);
    }

    if (sel.empty())
    {
        sel = pickDeepSkyObject(origin, direction, renderFlags, faintestMag, tolerance);
    }

    return sel;
}


// Search by name for an immediate child of the specified object.
Selection Universe::findChildObject(const Selection& sel,
                                    const string& name,
                                    bool i18n) const
{
    switch (sel.getType())
    {
    case Selection::Type_Star:
        {
            SolarSystem* sys = getSolarSystem(sel.star());
            if (sys != nullptr)
            {
                PlanetarySystem* planets = sys->getPlanets();
                if (planets != nullptr)
                    return Selection(planets->find(name, false, i18n));
            }
        }
        break;

    case Selection::Type_Body:
        {
            // First, search for a satellite
            PlanetarySystem* sats = sel.body()->getSatellites();
            if (sats != nullptr)
            {
                Body* body = sats->find(name, false, i18n);
                if (body != nullptr)
                    return Selection(body);
            }

            // If a satellite wasn't found, check this object's locations
            Location* loc = sel.body()->findLocation(name, i18n);
            if (loc != nullptr)
                return Selection(loc);
        }
        break;

    case Selection::Type_Location:
        // Locations have no children
        break;

    case Selection::Type_DeepSky:
        // Deep sky objects have no children
        break;

    default:
        break;
    }

    return Selection();
}


// Search for a name within an object's context.  For stars, planets (bodies),
// and locations, the context includes all bodies in the associated solar
// system.  For locations and planets, the context additionally includes
// sibling or child locations, respectively.
Selection Universe::findObjectInContext(const Selection& sel,
                                        const string& name,
                                        bool i18n) const
{
    Body* contextBody = nullptr;

    switch (sel.getType())
    {
    case Selection::Type_Body:
        contextBody = sel.body();
        break;

    case Selection::Type_Location:
        contextBody = sel.location()->getParentBody();
        break;

    default:
        break;
    }

    // First, search for bodies...
    SolarSystem* sys = getSolarSystem(sel);
    if (sys != nullptr)
    {
        PlanetarySystem* planets = sys->getPlanets();
        if (planets != nullptr)
        {
            Body* body = planets->find(name, true, i18n);
            if (body != nullptr)
                return Selection(body);
        }
    }

    // ...and then locations.
    if (contextBody != nullptr)
    {
        Location* loc = contextBody->findLocation(name, i18n);
        if (loc != nullptr)
            return Selection(loc);
    }

    return Selection();
}


// Select an object by name, with the following priority:
//   1. Try to look up the name in the star catalog
//   2. Search the deep sky catalog for a matching name.
//   3. Check the solar systems for planet names; we don't make any decisions
//      about which solar systems are relevant, and let the caller pass them
//      to us to search.
Selection Universe::find(const string& s,
                         Selection* contexts,
                         int nContexts,
                         bool i18n) const
{
    if (starCatalog != nullptr)
    {
        Star* star = starCatalog->find(s);
        if (star != nullptr)
            return Selection(star);
        star = starCatalog->find(ReplaceGreekLetterAbbr(s));
        if (star != nullptr)
            return Selection(star);
    }

    if (dsoCatalog != nullptr)
    {
        DeepSkyObject* dso = dsoCatalog->find(s);
        if (dso != nullptr)
            return Selection(dso);
        dso = dsoCatalog->find(ReplaceGreekLetterAbbr(s));
        if (dso != nullptr)
            return Selection(dso);
    }

    for (int i=0; i<nContexts; ++i)
    {
        Selection sel = findObjectInContext(contexts[i], s, i18n);
        if (!sel.empty())
            return sel;
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
                             Selection contexts[],
                             int nContexts,
                             bool i18n) const
{
    string::size_type pos = s.find('/', 0);

    // No delimiter found--just do a normal find.
    if (pos == string::npos)
        return find(s, contexts, nContexts, i18n);

    // Find the base object
    string base(s, 0, pos);

    Selection sel = find(base, contexts, nContexts, i18n);

    while (!sel.empty() && pos != string::npos)
    {
        string::size_type nextPos = s.find('/', pos + 1);
        string::size_type len;
        if (nextPos == string::npos)
            len = s.size() - pos - 1;
        else
            len = nextPos - pos - 1;
        string name = string(s, pos + 1, len);

        sel = findChildObject(sel, name, i18n);

        pos = nextPos;
    }

    return sel;
}


vector<string> Universe::getCompletion(const string& s,
                                                 Selection* contexts,
                                                 int nContexts,
                                                 bool withLocations)
{
    vector<string> completion;
    int s_length = UTF8Length(s);

    // Solar bodies first:
    for (int i = 0; i < nContexts; i++)
    {
        if (withLocations && contexts[i].getType() == Selection::Type_Body)
        {
            vector<Location*>* locations = contexts[i].body()->getLocations();
            if (locations != nullptr)
            {
                for (const auto location : *locations)
                {
                    if (!UTF8StringCompare(s, location->getName(true), s_length))
                        completion.push_back(location->getName(true));
                }
            }
        }

        SolarSystem* sys = getSolarSystem(contexts[i]);
        if (sys != nullptr)
        {
            PlanetarySystem* planets = sys->getPlanets();
            if (planets != nullptr)
            {
                vector<string> bodies = planets->getCompletion(s);
                completion.insert(completion.end(),
                                  bodies.begin(), bodies.end());
            }
        }
    }

    // Deep sky objects:
    if (dsoCatalog != nullptr)
    {
        vector<string> dsos  = dsoCatalog->getCompletion(s);
        completion.insert(completion.end(), dsos.begin(), dsos.end());
    }

    // and finally stars;
    if (starCatalog != nullptr)
    {
        vector<string> stars  = starCatalog->getCompletion(s);
        completion.insert(completion.end(), stars.begin(), stars.end());
    }

    return completion;
}


vector<string> Universe::getCompletionPath(const string& s,
                                           Selection* contexts,
                                           int nContexts,
                                           bool withLocations)
{
    vector<string> completion;
    vector<string> locationCompletion;
    string::size_type pos = s.rfind('/', s.length());

    if (pos == string::npos)
        return getCompletion(s, contexts, nContexts, withLocations);

    string base(s, 0, pos);
    Selection sel = findPath(base, contexts, nContexts, true);

    if (sel.empty())
    {
        return completion;
    }

    if (sel.getType() == Selection::Type_DeepSky)
    {
        completion.push_back(dsoCatalog->getDSOName(sel.deepsky()));
        return completion;
    }

    PlanetarySystem* worlds = nullptr;
    if (sel.getType() == Selection::Type_Body)
    {
        worlds = sel.body()->getSatellites();
        vector<Location*>* locations = sel.body()->getLocations();
        if (locations != nullptr && withLocations)
        {
            string search = s.substr(pos + 1);
            for (const auto location : *locations)
            {
                if (!UTF8StringCompare(search, location->getName(true),
                                       search.length()))
                {
                    locationCompletion.push_back(location->getName(true));
                }
            }
        }
    }
    else if (sel.getType() == Selection::Type_Star)
    {
        SolarSystem* ssys = getSolarSystem(sel.star());
        if (ssys != nullptr)
            worlds = ssys->getPlanets();
    }

    if (worlds != nullptr)
        completion = worlds->getCompletion(s.substr(pos + 1), false);

    completion.insert(completion.end(), locationCompletion.begin(), locationCompletion.end());

    return completion;
}


// Return the closest solar system to position, or nullptr if there are no planets
// with in one light year.
SolarSystem* Universe::getNearestSolarSystem(const UniversalCoord& position) const
{
    Vector3f pos = position.toLy().cast<float>();
    ClosestStarFinder closestFinder(1.0f, this);
    closestFinder.withPlanets = true;
    starCatalog->findCloseStars(closestFinder, pos, 1.0f);
    return getSolarSystem(closestFinder.closestStar);
}


void
Universe::getNearStars(const UniversalCoord& position,
                       float maxDistance,
                       vector<const Star*>& nearStars) const
{
    Vector3f pos = position.toLy().cast<float>();
    NearStarFinder finder(maxDistance, nearStars);
    starCatalog->findCloseStars(finder, pos, maxDistance);
}
