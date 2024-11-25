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

#include "universe.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <utility>

#include <celcompat/numbers.h>
#include <celephem/orbit.h>
#include <celmath/mathlib.h>
#include <celmath/intersect.h>
#include <celmath/ray.h>
#include <celutil/greek.h>
#include <celutil/utf8.h>
#include "asterism.h"
#include "body.h"
#include "boundaries.h"
#include "frametree.h"
#include "location.h"
#include "meshmanager.h"
#include "render.h"
#include "timelinephase.h"

namespace engine = celestia::engine;
namespace math = celestia::math;
namespace util = celestia::util;

namespace
{

constexpr double ANGULAR_RES = 3.5e-6;

class ClosestStarFinder : public engine::StarHandler
{
public:
    ClosestStarFinder(float _maxDistance, const Universe* _universe);
    ~ClosestStarFinder() = default;
    void process(const Star& star, float distance, float appMag) override;

public:
    float maxDistance;
    float closestDistance;
    const Star* closestStar;
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

void
ClosestStarFinder::process(const Star& star, float distance, float /*unused*/)
{
    if (distance < closestDistance)
    {
        if (!withPlanets || universe->getSolarSystem(&star))
        {
            closestStar = &star;
            closestDistance = distance;
        }
    }
}

class NearStarFinder : public engine::StarHandler
{
public:
    NearStarFinder(float _maxDistance, std::vector<const Star*>& nearStars);
    ~NearStarFinder() = default;
    void process(const Star& star, float distance, float appMag);

private:
    float maxDistance;
    std::vector<const Star*>& nearStars;
};

NearStarFinder::NearStarFinder(float _maxDistance,
                               std::vector<const Star*>& _nearStars) :
    maxDistance(_maxDistance),
    nearStars(_nearStars)
{
}

void
NearStarFinder::process(const Star& star, float distance, float /*unused*/)
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
    Eigen::ParametrizedLine<double, 3> pickRay;
    double jd;
    float atanTolerance;
};

bool
ApproxPlanetPickTraversal(Body* body, PlanetPickInfo& pickInfo)
{
    // Reject invisible bodies and bodies that don't exist at the current time
    if (!body->isVisible() || !body->extant(pickInfo.jd) || !body->isClickable())
        return true;

    Eigen::Vector3d bpos = body->getAstrocentricPosition(pickInfo.jd);
    Eigen::Vector3d bodyDir = bpos - pickInfo.pickRay.origin();
    double distance = bodyDir.norm();

    // Check the apparent radius of the orbit against our tolerance factor.
    // This check exists to make sure than when picking a distant, we select
    // the planet rather than one of its satellites.
    if (auto appOrbitRadius = static_cast<float>(body->getOrbit(pickInfo.jd)->getBoundingRadius() / distance);
        std::max(static_cast<double>(pickInfo.atanTolerance), ANGULAR_RES) > appOrbitRadius)
    {
        return true;
    }

    bodyDir.normalize();
    Eigen::Vector3d bodyMiss = bodyDir - pickInfo.pickRay.direction();
    if (double sinAngle2 = bodyMiss.norm() / 2.0; sinAngle2 <= pickInfo.sinAngle2Closest)
    {
        pickInfo.sinAngle2Closest = std::max(sinAngle2, ANGULAR_RES);
        pickInfo.closestBody = body;
        pickInfo.closestApproxDistance = distance;
    }

    return true;
}

// Perform an intersection test between the pick ray and a body
bool
ExactPlanetPickTraversal(Body* body, PlanetPickInfo& pickInfo)
{
    Eigen::Vector3d bpos = body->getAstrocentricPosition(pickInfo.jd);
    float radius = body->getRadius();
    double distance = -1.0;

    // Test for intersection with the bounding sphere
    if (!body->isVisible() || !body->extant(pickInfo.jd) || !body->isClickable() ||
        !math::testIntersection(pickInfo.pickRay, math::Sphered(bpos, radius), distance))
        return true;

    if (body->getGeometry() == InvalidResource)
    {
        // There's no mesh, so the object is an ellipsoid.  If it's
        // spherical, we've already done all the work we need to. Otherwise,
        // we need to perform a ray-ellipsoid intersection test.
        if (!body->isSphere())
        {
            Eigen::Vector3d ellipsoidAxes = body->getSemiAxes().cast<double>();

            // Transform rotate the pick ray into object coordinates
            Eigen::Matrix3d m = body->getEclipticToEquatorial(pickInfo.jd).toRotationMatrix();
            Eigen::ParametrizedLine<double, 3> r(pickInfo.pickRay.origin() - bpos, pickInfo.pickRay.direction());
            r = math::transformRay(r, m);
            if (!math::testIntersection(r, math::Ellipsoidd(ellipsoidAxes), distance))
                distance = -1.0;
        }
    }
    else
    {
        // Transform rotate the pick ray into object coordinates
        Eigen::Quaterniond qd = body->getGeometryOrientation().cast<double>();
        Eigen::Matrix3d m = (qd * body->getEclipticToBodyFixed(pickInfo.jd)).toRotationMatrix();
        Eigen::ParametrizedLine<double, 3> r(pickInfo.pickRay.origin() - bpos, pickInfo.pickRay.direction());
        r = math::transformRay(r, m);

        const Geometry* geometry = engine::GetGeometryManager()->find(body->getGeometry());
        float scaleFactor = body->getGeometryScale();
        if (geometry != nullptr && geometry->isNormalized())
            scaleFactor = radius;

        // The mesh vertices are normalized, then multiplied by a scale
        // factor.  Thus, the ray needs to be multiplied by the inverse of
        // the mesh scale factor.
        double is = 1.0 / scaleFactor;
        r.origin() *= is;
        r.direction() *= is;

        if (geometry != nullptr && !geometry->pick(r, distance))
            distance = -1.0;
    }
    // Make also sure that the pickRay does not intersect the body in the
    // opposite hemisphere! Hence, need again the "bodyMiss" angle

    Eigen::Vector3d bodyDir = bpos - pickInfo.pickRay.origin();
    bodyDir.normalize();
    Eigen::Vector3d bodyMiss = bodyDir - pickInfo.pickRay.direction();

    if (double sinAngle2 = bodyMiss.norm() / 2.0;
        sinAngle2 < (celestia::numbers::sqrt2 * 0.5) && // sin(45 degrees) = sqrt(2)/2
        distance > 0.0 && distance <= pickInfo.closestDistance)
    {
        pickInfo.closestDistance = distance;
        pickInfo.closestBody = body;
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
template<typename F>
bool
traverseFrameTree(const FrameTree* frameTree,
                  double tdb,
                  F func,
                  PlanetPickInfo& info)
{
    for (unsigned int i = 0; i < frameTree->childCount(); i++)
    {
        const TimelinePhase* phase = frameTree->getChild(i);
        if (!phase->includes(tdb))
            continue;

        Body* body = phase->body();
        if (!func(body, info))
            return false;

        if (const FrameTree* bodyFrameTree = body->getFrameTree();
            bodyFrameTree != nullptr && !traverseFrameTree(bodyFrameTree, tdb, func, info))
        {
            return false;
        }
    }

    return true;
}

// StarPicker is a callback class for StarDatabase::findVisibleStars
class StarPicker : public engine::StarHandler
{
public:
    StarPicker(const Eigen::Vector3f&, const Eigen::Vector3f&, double, float);
    ~StarPicker() = default;

    void process(const Star& /*star*/, float /*unused*/, float /*unused*/) override;

public:
    const Star* pickedStar;
    Eigen::Vector3f pickOrigin;
    Eigen::Vector3f pickRay;
    double sinAngle2Closest;
    double when;
};

StarPicker::StarPicker(const Eigen::Vector3f& _pickOrigin,
                       const Eigen::Vector3f& _pickRay,
                       double _when,
                       float angle) :
    pickedStar(nullptr),
    pickOrigin(_pickOrigin),
    pickRay(_pickRay),
    sinAngle2Closest(std::max(std::sin(angle / 2.0), ANGULAR_RES)),
    when(_when)
{
}

void
StarPicker::process(const Star& star, float /*unused*/, float /*unused*/)
{
    Eigen::Vector3f relativeStarPos = star.getPosition() - pickOrigin;
    Eigen::Vector3f starDir = relativeStarPos.normalized();

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
        if (math::testIntersection(Eigen::ParametrizedLine<float, 3>(Eigen::Vector3f::Zero(), pickRay),
                                   math::Spheref(relativeStarPos, orbitalRadius * 2.0f),
                                   distance))
        {
            Eigen::Vector3d starPos = star.getPosition(when).toLy();
            starDir = (starPos - pickOrigin.cast<double>()).cast<float>().normalized();
        }
    }

    Eigen::Vector3f starMiss = starDir - pickRay;
    Eigen::Vector3d sMd = starMiss.cast<double>();
    sinAngle2 = sMd.norm() / 2.0;

    if (sinAngle2 <= sinAngle2Closest)
    {
        sinAngle2Closest = std::max(sinAngle2, ANGULAR_RES);
        pickedStar = &star;
        if (pickedStar->getOrbitBarycenter() != nullptr)
            pickedStar = pickedStar->getOrbitBarycenter();
    }
}

class CloseStarPicker : public engine::StarHandler
{
public:
    CloseStarPicker(const UniversalCoord& pos,
                    const Eigen::Vector3f& dir,
                    double t,
                    float _maxDistance,
                    float angle);
    ~CloseStarPicker() = default;
    void process(const Star& star, float lowPrecDistance, float appMag) override;

public:
    UniversalCoord pickOrigin;
    Eigen::Vector3f pickDir;
    double now;
    float maxDistance;
    const Star* closestStar;
    float closestDistance;
    double sinAngle2Closest;
};

CloseStarPicker::CloseStarPicker(const UniversalCoord& pos,
                                 const Eigen::Vector3f& dir,
                                 double t,
                                 float _maxDistance,
                                 float angle) :
    pickOrigin(pos),
    pickDir(dir),
    now(t),
    maxDistance(_maxDistance),
    closestStar(nullptr),
    closestDistance(0.0f),
    sinAngle2Closest(std::max(std::sin(angle/2.0), ANGULAR_RES))
{
}

void
CloseStarPicker::process(const Star& star,
                         float lowPrecDistance,
                         float /*unused*/)
{
    if (lowPrecDistance > maxDistance)
        return;

    Eigen::Vector3d hPos = star.getPosition(now).offsetFromKm(pickOrigin);
    Eigen::Vector3f starDir = hPos.cast<float>();

    float distance = 0.0f;

     if (math::testIntersection(Eigen::ParametrizedLine<float, 3>(Eigen::Vector3f::Zero(), pickDir),
                                math::Spheref(starDir, star.getRadius()), distance))
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
        Eigen::Vector3f starMiss = starDir - pickDir;
        Eigen::Vector3d sMd = starMiss.cast<double>();

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

class DSOPicker : public engine::DSOHandler
{
public:
    DSOPicker(const Eigen::Vector3d& pickOrigin,
              const Eigen::Vector3d& pickDir,
              std::uint64_t renderFlags,
              float angle);
    ~DSOPicker() = default;

    void process(const std::unique_ptr<DeepSkyObject>&, double, float) override; //NOSONAR

public:
    Eigen::Vector3d pickOrigin;
    Eigen::Vector3d pickDir;
    std::uint64_t renderFlags;

    const DeepSkyObject* pickedDSO;
    double  sinAngle2Closest;
};

DSOPicker::DSOPicker(const Eigen::Vector3d& pickOrigin,
                     const Eigen::Vector3d& pickDir,
                     std::uint64_t renderFlags,
                     float angle) :
    pickOrigin      (pickOrigin),
    pickDir         (pickDir),
    renderFlags     (renderFlags),
    pickedDSO       (nullptr),
    sinAngle2Closest(std::max(std::sin(angle / 2.0), ANGULAR_RES))
{
}

void
DSOPicker::process(const std::unique_ptr<DeepSkyObject>& dso, double, float) //NOSONAR
{
    if (!(dso->getRenderMask() & renderFlags) || !dso->isVisible() || !dso->isClickable())
        return;

    Eigen::Vector3d relativeDSOPos = dso->getPosition() - pickOrigin;
    Eigen::Vector3d dsoDir = relativeDSOPos;

    if (double distance2 = 0.0;
        math::testIntersection(Eigen::ParametrizedLine<double, 3>(Eigen::Vector3d::Zero(), pickDir),
                               math::Sphered(relativeDSOPos, (double) dso->getRadius()), distance2))
    {
        Eigen::Vector3d dsoPos = dso->getPosition();
        dsoDir = dsoPos * 1.0e-6 - pickOrigin;
    }
    dsoDir.normalize();

    Eigen::Vector3d dsoMissd   = dsoDir - pickDir;
    double sinAngle2 = dsoMissd.norm() / 2.0;

    if (sinAngle2 <= sinAngle2Closest)
    {
        sinAngle2Closest = std::max(sinAngle2, ANGULAR_RES);
        pickedDSO        = dso.get();
    }
}

class CloseDSOPicker : public engine::DSOHandler
{
public:
    CloseDSOPicker(const Eigen::Vector3d& pos,
                   const Eigen::Vector3d& dir,
                   std::uint64_t renderFlags,
                   double maxDistance,
                   float);
    ~CloseDSOPicker() = default;

    void process(const std::unique_ptr<DeepSkyObject>& dso, double distance, float appMag); //NOSONAR

public:
    Eigen::Vector3d  pickOrigin;
    Eigen::Vector3d  pickDir;
    std::uint64_t renderFlags;
    double    maxDistance;

    const DeepSkyObject* closestDSO;
    double largestCosAngle;
};

CloseDSOPicker::CloseDSOPicker(const Eigen::Vector3d& pos,
                               const Eigen::Vector3d& dir,
                               std::uint64_t renderFlags,
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

void
CloseDSOPicker::process(const std::unique_ptr<DeepSkyObject>& dso, //NOSONAR
                        double distance,
                        float /*unused*/)
{
    if (distance > maxDistance || !(dso->getRenderMask() & renderFlags) || !dso->isVisible() || !dso->isClickable())
        return;

    double  distanceToPicker       = 0.0;
    double  cosAngleToBoundCenter  = 0.0;
    if (dso->pick(Eigen::ParametrizedLine<double, 3>(pickOrigin, pickDir), distanceToPicker, cosAngleToBoundCenter))
    {
        // Don't select the object the observer is currently in:
        if ((pickOrigin - dso->getPosition()).norm() > dso->getRadius() &&
            cosAngleToBoundCenter > largestCosAngle)
        {
            closestDSO      = dso.get();
            largestCosAngle = cosAngleToBoundCenter;
        }
    }
}

void
getLocationsCompletion(std::vector<celestia::engine::Completion>& completion,
                       std::string_view s,
                       const Body& body)
{
    auto locations = GetBodyFeaturesManager()->getLocations(&body);
    if (!locations.has_value())
        return;

    for (const auto location : *locations)
    {
        const std::string& name = location->getName(false);
        if (UTF8StartsWith(name, s))
        {
            completion.emplace_back(name, Selection(location));
        }
        else
        {
            const std::string& lname = location->getName(true);
            if (lname != name && UTF8StartsWith(lname, s))
                completion.emplace_back(lname, Selection(location));
        }
    }
}

} // end unnamed namespace

// Needs definition of ConstellationBoundaries
Universe::~Universe() = default;

StarDatabase*
Universe::getStarCatalog() const
{
    return starCatalog.get();
}

void
Universe::setStarCatalog(std::unique_ptr<StarDatabase>&& catalog)
{
    starCatalog = std::move(catalog);
}

SolarSystemCatalog*
Universe::getSolarSystemCatalog() const
{
    return solarSystemCatalog.get();
}

void
Universe::setSolarSystemCatalog(std::unique_ptr<SolarSystemCatalog>&& catalog)
{
    solarSystemCatalog = std::move(catalog);
}

DSODatabase*
Universe::getDSOCatalog() const
{
    return dsoCatalog.get();
}

void
Universe::setDSOCatalog(std::unique_ptr<DSODatabase>&& catalog)
{
    dsoCatalog = std::move(catalog);
}

AsterismList*
Universe::getAsterisms() const
{
    return asterisms.get();
}

void
Universe::setAsterisms(std::unique_ptr<AsterismList>&& _asterisms)
{
    asterisms = std::move(_asterisms);
}

ConstellationBoundaries*
Universe::getBoundaries() const
{
    return boundaries.get();
}

void
Universe::setBoundaries(std::unique_ptr<ConstellationBoundaries>&& _boundaries)
{
    boundaries = std::move(_boundaries);
}

// Return the planetary system of a star, or nullptr if it has no planets.
SolarSystem*
Universe::getSolarSystem(const Star* star) const
{
    if (star == nullptr)
        return nullptr;

    auto starNum = star->getIndex();
    auto iter = solarSystemCatalog->find(starNum);
    return iter == solarSystemCatalog->end()
        ? nullptr
        : iter->second.get();
}

// A more general version of the method above--return the solar system
// that contains an object, or nullptr if there is no solar sytstem.
SolarSystem*
Universe::getSolarSystem(const Selection& sel) const
{
    switch (sel.getType())
    {
    case SelectionType::Star:
        return getSolarSystem(sel.star());

    case SelectionType::Body:
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

    case SelectionType::Location:
        return getSolarSystem(Selection(sel.location()->getParentBody()));

    default:
        return nullptr;
    }
}

// Create a new solar system for a star and return a pointer to it; if it
// already has a solar system, just return a pointer to the existing one.
SolarSystem*
Universe::getOrCreateSolarSystem(Star* star) const
{
    auto starNum = star->getIndex();
    auto iter = solarSystemCatalog->lower_bound(starNum);
    if (iter != solarSystemCatalog->end() && iter->first == starNum)
        return iter->second.get();

    iter = solarSystemCatalog->emplace_hint(iter, starNum, std::make_unique<SolarSystem>(star));
    return iter->second.get();
}

const celestia::MarkerList&
Universe::getMarkers() const
{
    return markers;
}

void
Universe::markObject(const Selection& sel,
                     const celestia::MarkerRepresentation& rep,
                     int priority,
                     bool occludable,
                     celestia::MarkerSizing sizing)
{
    if (auto iter = std::find_if(markers.begin(), markers.end(),
                                 [&sel](const auto& m) { return m.object() == sel; });
        iter != markers.end())
    {
        // Handle the case when the object is already marked.  If the
        // priority is higher or equal to the existing marker, replace it.
        // Otherwise, do nothing.
        if (priority < iter->priority())
            return;
        markers.erase(iter);
    }

    celestia::Marker& marker = markers.emplace_back(sel);
    marker.setRepresentation(rep);
    marker.setPriority(priority);
    marker.setOccludable(occludable);
    marker.setSizing(sizing);
}

void
Universe::unmarkObject(const Selection& sel, int priority)
{
    auto iter = std::find_if(markers.begin(), markers.end(),
                             [&sel](const auto& m) { return m.object() == sel; });
    if (iter != markers.end() && priority >= iter->priority())
        markers.erase(iter);
}

void
Universe::unmarkAll()
{
    markers.clear();
}

bool
Universe::isMarked(const Selection& sel, int priority) const
{
    auto iter = std::find_if(markers.begin(), markers.end(),
                             [&sel](const auto& m) { return m.object() == sel; });
    return iter != markers.end() && iter->priority() >= priority;
}

Selection
Universe::pickPlanet(const SolarSystem& solarSystem,
                     const UniversalCoord& origin,
                     const Eigen::Vector3f& direction,
                     double when,
                     float /*faintestMag*/,
                     float tolerance) const
{
    double sinTol2 = std::max(std::sin(tolerance / 2.0), ANGULAR_RES);
    PlanetPickInfo pickInfo;

    Star* star = solarSystem.getStar();
    assert(star != nullptr);

    // Transform the pick ray origin into astrocentric coordinates
    Eigen::Vector3d astrocentricOrigin = origin.offsetFromKm(star->getPosition(when));

    pickInfo.pickRay = Eigen::ParametrizedLine<double, 3>(astrocentricOrigin, direction.cast<double>());
    pickInfo.sinAngle2Closest = 1.0;
    pickInfo.closestDistance = 1.0e50;
    pickInfo.closestApproxDistance = 1.0e50;
    pickInfo.closestBody = nullptr;
    pickInfo.jd = when;
    pickInfo.atanTolerance = (float) atan(tolerance);

    // First see if there's a planet|moon that the pick ray intersects.
    // Select the closest planet|moon intersected.
    traverseFrameTree(solarSystem.getFrameTree(), when, &ExactPlanetPickTraversal, pickInfo);

    if (pickInfo.closestBody != nullptr)
    {
        // Retain that body
        Body* closestBody = pickInfo.closestBody;

        // Check if there is a satellite in front of the primary body that is
        // sufficiently close to the pickRay
        traverseFrameTree(solarSystem.getFrameTree(), when, &ApproxPlanetPickTraversal, pickInfo);

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
    traverseFrameTree(solarSystem.getFrameTree(), when, &ApproxPlanetPickTraversal, pickInfo);

    if (pickInfo.sinAngle2Closest <= sinTol2)
        return Selection(pickInfo.closestBody);
    else
        return Selection();
}

Selection
Universe::pickStar(const UniversalCoord& origin,
                   const Eigen::Vector3f& direction,
                   double when,
                   float faintestMag,
                   float tolerance) const
{
    Eigen::Vector3f o = origin.toLy().cast<float>();

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
    Eigen::Quaternionf rotation;
    rotation.setFromTwoVectors(-Eigen::Vector3f::UnitZ(), direction);

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

Selection
Universe::pickDeepSkyObject(const UniversalCoord& origin,
                            const Eigen::Vector3f& direction,
                            std::uint64_t renderFlags,
                            float faintestMag,
                            float tolerance) const
{
    Eigen::Vector3d orig = origin.toLy();
    Eigen::Vector3d dir = direction.cast<double>();

    CloseDSOPicker closePicker(orig, dir, renderFlags, 1e9, tolerance);

    dsoCatalog->findCloseDSOs(closePicker, orig, 1e9);
    if (closePicker.closestDSO != nullptr)
    {
        return Selection(const_cast<DeepSkyObject*>(closePicker.closestDSO));
    }

    Eigen::Quaternionf rotation;
    rotation.setFromTwoVectors(-Eigen::Vector3f::UnitZ(), direction);

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

Selection
Universe::pick(const UniversalCoord& origin,
               const Eigen::Vector3f& direction,
               double when,
               std::uint64_t renderFlags,
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
            const SolarSystem* solarSystem = getSolarSystem(star);
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
Selection
Universe::findChildObject(const Selection& sel,
                          std::string_view name,
                          bool i18n) const
{
    switch (sel.getType())
    {
    case SelectionType::Star:
        if (const SolarSystem* sys = getSolarSystem(sel.star()); sys != nullptr)
        {
            PlanetarySystem* planets = sys->getPlanets();
            if (planets != nullptr)
                return Selection(planets->find(name, false, i18n));
        }
        break;

    case SelectionType::Body:
        // First, search for a satellite
        if (const PlanetarySystem* sats = sel.body()->getSatellites();sats != nullptr)
        {
            Body* body = sats->find(name, false, i18n);
            if (body != nullptr)
                return Selection(body);
        }

        // If a satellite wasn't found, check this object's locations
        if (Location* loc = GetBodyFeaturesManager()->findLocation(sel.body(), name, i18n);
            loc != nullptr)
        {
            return Selection(loc);
        }
        break;

    default:
        // Locations and deep sky objects have no children
        break;
    }

    return Selection();
}

// Search for a name within an object's context.  For stars, planets (bodies),
// and locations, the context includes all bodies in the associated solar
// system.  For locations and planets, the context additionally includes
// sibling or child locations, respectively.
Selection
Universe::findObjectInContext(const Selection& sel,
                              std::string_view name,
                              bool i18n) const
{
    const Body* contextBody = nullptr;

    switch (sel.getType())
    {
    case SelectionType::Body:
        contextBody = sel.body();
        break;

    case SelectionType::Location:
        contextBody = sel.location()->getParentBody();
        break;

    default:
        break;
    }

    // First, search for bodies...
    if (const SolarSystem* sys = getSolarSystem(sel); sys != nullptr)
    {
        if (const PlanetarySystem* planets = sys->getPlanets(); planets != nullptr)
        {
            if (Body* body = planets->find(name, true, i18n); body != nullptr)
                return Selection(body);
        }
    }

    // ...and then locations.
    if (contextBody != nullptr)
    {
        Location* loc = GetBodyFeaturesManager()->findLocation(contextBody, name, i18n);
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
Selection
Universe::find(std::string_view s,
               util::array_view<const Selection> contexts,
               bool i18n) const
{
    if (starCatalog != nullptr)
    {
        Star* star = starCatalog->find(s, i18n);
        if (star != nullptr)
            return Selection(star);
        star = starCatalog->find(ReplaceGreekLetterAbbr(s), i18n);
        if (star != nullptr)
            return Selection(star);
    }

    if (dsoCatalog != nullptr)
    {
        DeepSkyObject* dso = dsoCatalog->find(s, i18n);
        if (dso != nullptr)
            return Selection(dso);
        dso = dsoCatalog->find(ReplaceGreekLetterAbbr(s), i18n);
        if (dso != nullptr)
            return Selection(dso);
    }

    for (const auto& context : contexts)
    {
        Selection sel = findObjectInContext(context, s, i18n);
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
Selection
Universe::findPath(std::string_view s,
                   util::array_view<const Selection> contexts,
                   bool i18n) const
{
    std::string_view::size_type pos = s.find('/', 0);

    // No delimiter found--just do a normal find.
    if (pos == std::string_view::npos)
        return find(s, contexts, i18n);

    // Find the base object
    auto base = s.substr(0, pos);
    Selection sel = find(base, contexts, i18n);

    while (!sel.empty() && pos != std::string_view::npos)
    {
        auto nextPos = s.find('/', pos + 1);
        auto len = nextPos == std::string_view::npos
                 ? s.size() - pos - 1
                 : nextPos - pos - 1;

        auto name = s.substr(pos + 1, len);

        sel = findChildObject(sel, name, i18n);

        pos = nextPos;
    }

    return sel;
}

void
Universe::getCompletion(std::vector<celestia::engine::Completion>& completion,
                        std::string_view s,
                        util::array_view<const Selection> contexts,
                        bool withLocations) const
{
    // Solar bodies first:
    for (const Selection& context : contexts)
    {
        if (withLocations && context.getType() == SelectionType::Body)
        {
            getLocationsCompletion(completion, s, *context.body());
        }

        const SolarSystem* sys = getSolarSystem(context);
        if (sys != nullptr)
        {
            const PlanetarySystem* planets = sys->getPlanets();
            if (planets != nullptr)
                planets->getCompletion(completion, s);
        }
    }

    // Deep sky objects:
    if (dsoCatalog != nullptr)
        dsoCatalog->getCompletion(completion, s);

    // and finally stars;
    if (starCatalog != nullptr)
        starCatalog->getCompletion(completion, s);
}

void
Universe::getCompletionPath(std::vector<celestia::engine::Completion>& completion,
                            std::string_view s,
                            util::array_view<const Selection> contexts,
                            bool withLocations) const
{
    std::string_view::size_type pos = s.rfind('/', s.length());

    if (pos == std::string_view::npos)
    {
        getCompletion(completion, s, contexts, withLocations);
        return;
    }

    auto base = s.substr(0, pos);
    Selection sel = findPath(base, contexts, true);

    if (sel.empty())
    {
        return;
    }

    if (sel.getType() == SelectionType::DeepSky)
    {
        completion.emplace_back(dsoCatalog->getDSOName(sel.deepsky()), sel);
        return;
    }

    const PlanetarySystem* worlds = nullptr;
    if (sel.getType() == SelectionType::Body)
    {
        worlds = sel.body()->getSatellites();
    }
    else if (sel.getType() == SelectionType::Star)
    {
        const SolarSystem* ssys = getSolarSystem(sel.star());
        if (ssys != nullptr)
            worlds = ssys->getPlanets();
    }

    if (worlds != nullptr)
        worlds->getCompletion(completion, s.substr(pos + 1), false);

    if (sel.getType() == SelectionType::Body && withLocations)
    {
        getLocationsCompletion(completion,
                               s.substr(pos + 1),
                               *sel.body());
    }
}

// Return the closest solar system to position, or nullptr if there are no planets
// with in one light year.
SolarSystem*
Universe::getNearestSolarSystem(const UniversalCoord& position) const
{
    Eigen::Vector3f pos = position.toLy().cast<float>();
    ClosestStarFinder closestFinder(1.0f, this);
    closestFinder.withPlanets = true;
    starCatalog->findCloseStars(closestFinder, pos, 1.0f);
    return getSolarSystem(closestFinder.closestStar);
}

void
Universe::getNearStars(const UniversalCoord& position,
                       float maxDistance,
                       std::vector<const Star*>& nearStars) const
{
    Eigen::Vector3f pos = position.toLy().cast<float>();
    NearStarFinder finder(maxDistance, nearStars);
    starCatalog->findCloseStars(finder, pos, maxDistance);
}
