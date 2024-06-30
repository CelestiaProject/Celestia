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
#include <limits>
#include <utility>

#include <celcompat/numbers.h>
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
#include "visibleobjectvisitor.h"

namespace engine = celestia::engine;
namespace numbers = celestia::numbers;
namespace math = celestia::math;
namespace util = celestia::util;

namespace
{

constexpr double ANGULAR_RES = 3.5e-6;

template<typename T>
bool
checkNodeDistance(const Eigen::Matrix<T, 3, 1>& center,
                  const Eigen::Matrix<T, 3, 1>& position,
                  T size,
                  T maxDistance)
{
    T distance = (position - center).norm() - size * numbers::sqrt3_v<T>;
    return distance <= maxDistance;
}

class ClosestSystemFinder
{
public:
    ClosestSystemFinder(const Eigen::Vector3f&, float, const Universe* universe);

    bool checkNode(const Eigen::Vector3f&, float, float) const;
    void process(const Star&);

    const Star* best() const { return m_best; }

private:
    Eigen::Vector3f m_position;
    float m_maxDistance;
    const Universe* m_universe;
    const Star* m_best{ nullptr };
    float m_bestDistance2;
};

ClosestSystemFinder::ClosestSystemFinder(const Eigen::Vector3f& position,
                                         float maxDistance,
                                         const Universe* universe) :
    m_position(position),
    m_maxDistance(maxDistance),
    m_universe(universe),
    m_bestDistance2(math::square(maxDistance))
{
}

bool
ClosestSystemFinder::checkNode(const Eigen::Vector3f& center,
                               float size,
                               float /* magnitude */) const
{
    return checkNodeDistance(center, m_position, size, m_maxDistance);
}

void
ClosestSystemFinder::process(const Star& star)
{
    float distance2 = (star.getPosition() - m_position).squaredNorm();
    if (distance2 < m_bestDistance2 && m_universe->getSolarSystem(&star))
    {
        m_best = &star;
        m_maxDistance = std::sqrt(distance2);
        m_bestDistance2 = distance2;
    }
}

template<typename T>
class NearStarFinder
{
public:
    NearStarFinder(const Eigen::Vector3f&, float, T*);

    bool checkNode(const Eigen::Vector3f&, float, float);
    void process(const Star& star);

private:
    Eigen::Vector3f m_position;
    float m_maxDistance;
    float m_maxDistance2;
    T* m_stars;
};

template<typename T>
NearStarFinder<T>::NearStarFinder(const Eigen::Vector3f& position,
                                  float maxDistance,
                                  T* stars) :
    m_position(position),
    m_maxDistance(maxDistance),
    m_maxDistance2(math::square(maxDistance)),
    m_stars(stars)
{
    m_stars->clear();
}

template<typename T>
bool
NearStarFinder<T>::checkNode(const Eigen::Vector3f& center,
                             float size,
                             float /* magnitude */)
{
    return checkNodeDistance(center, m_position, size, m_maxDistance);
}

template<typename T>
void
NearStarFinder<T>::process(const Star& star)
{
    float distance2 = (star.getPosition() - m_position).squaredNorm();
    if (distance2 <= m_maxDistance2)
        m_stars->push_back(&star);
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

class StarPicker : private VisibleObjectVisitor<float>
{
public:
    StarPicker(const UniversalCoord&,
               const Eigen::Vector3f&,
               float fov,
               float faintestMag,
               double now);

    using VisibleObjectVisitor::checkNode;
    void process(const Star&);

    const Star* pickedStar() const;

private:
    Eigen::Vector3f m_pickOrigin;
    Eigen::Vector3f m_pickRay;
    double m_now;
    double m_sinAngle2Closest;
    const Star* m_pickedStar{ nullptr };
};

StarPicker::StarPicker(const UniversalCoord& pos,
                       const Eigen::Vector3f& direction,
                       float fov,
                       float faintestMag,
                       double now) :
    VisibleObjectVisitor(pos,
                         Eigen::Quaternionf::FromTwoVectors(-Eigen::Vector3f::UnitZ(), direction).conjugate(),
                         fov,
                         1.0f,
                         std::numeric_limits<float>::max(),
                         faintestMag),
    m_pickOrigin(pos.toLy().cast<float>()),
    m_pickRay(direction),
    m_now(now),
    m_sinAngle2Closest(std::max(std::sin(fov * 0.5), ANGULAR_RES))
{
}

void
StarPicker::process(const Star& star)
{
    Eigen::Vector3f relativeStarPos = star.getPosition() - m_pickOrigin;
    Eigen::Vector3f starDir = relativeStarPos.normalized();

    // Stars with orbits need special handling
    float orbitalRadius = star.getOrbitalRadius();
    if (orbitalRadius != 0.0f)
    {
        float distance = 0.0f;

        // Check for an intersection with orbital bounding sphere; if there's
        // no intersection, then just use normal calculation.  We actually test
        // intersection with a larger sphere to make sure we don't miss a star
        // right on the edge of the sphere.
        if (math::testIntersection(Eigen::ParametrizedLine<float, 3>(Eigen::Vector3f::Zero(), m_pickRay),
                                   math::Spheref(relativeStarPos, orbitalRadius * 2.0f),
                                   distance))
        {
            Eigen::Vector3d starPos = star.getPosition(m_now).toLy();
            starDir = (starPos - m_observerPos).normalized().cast<float>();
        }
    }

    double sinAngle2 = (starDir - m_pickRay).cast<double>().norm() * 0.5;
    if (sinAngle2 <= m_sinAngle2Closest)
    {
        m_sinAngle2Closest = std::max(sinAngle2, ANGULAR_RES);
        m_pickedStar = &star;
        if (auto barycenter = m_pickedStar->getOrbitBarycenter(); barycenter != nullptr)
            m_pickedStar = barycenter;
    }
}

inline const Star*
StarPicker::pickedStar() const
{
    return m_pickedStar;
}

class CloseStarPicker
{
public:
    CloseStarPicker(const UniversalCoord&,
                    const Eigen::Vector3f&,
                    double t,
                    float,
                    float);


    bool checkNode(const Eigen::Vector3f&, float, float) const;
    void process(const Star&);

    const Star* closestStar() const;

private:
    UniversalCoord m_origin;
    Eigen::Vector3f m_position;
    Eigen::Vector3f m_direction;
    float m_maxDistance;
    float m_maxDistance2;
    float m_sinAngle2Closest;
    float m_closestDistance{ std::numeric_limits<float>::max() };
    double m_now;
    const Star* m_closestStar{ nullptr };
};

CloseStarPicker::CloseStarPicker(const UniversalCoord& pos,
                                 const Eigen::Vector3f& dir,
                                 double t,
                                 float maxDistance,
                                 float angle) :
    m_origin(pos),
    m_position(pos.toLy().cast<float>()),
    m_direction(dir),
    m_maxDistance(maxDistance),
    m_maxDistance2(math::square(maxDistance)),
    m_sinAngle2Closest(std::max(std::sin(angle * 0.5), ANGULAR_RES)),
    m_now(t)
{
}

bool
CloseStarPicker::checkNode(const Eigen::Vector3f& center,
                           float size,
                           float /* magnitude */) const
{
    return checkNodeDistance(center, m_position, size, m_maxDistance);
}

void
CloseStarPicker::process(const Star& star)
{
    if (auto lowPrecDistance2 = (star.getPosition() - m_position).squaredNorm();
        lowPrecDistance2 > m_maxDistance2)
    {
        return;
    }

    Eigen::Vector3d hPos = star.getPosition(m_now).offsetFromKm(m_origin);
    Eigen::Vector3f starDir = hPos.cast<float>();

    float distance = 0.0f;
    if (math::testIntersection(Eigen::ParametrizedLine<float, 3>(Eigen::Vector3f::Zero(), m_direction),
                               math::Spheref(starDir, star.getRadius()),
                               distance))
    {
        if (distance > 0.0f && distance < m_closestDistance)
        {
            m_closestStar = &star;
            m_closestDistance = starDir.norm();
            // An exact hit -- se the angle to "zero"
            m_sinAngle2Closest = ANGULAR_RES;
        }
    }
    else
    {
        // We don't have an exact hit, check to see if we're close enough
        distance = starDir.norm();
        starDir /= distance;
        double sinAngle2 = (starDir - m_direction).cast<double>().norm() * 0.5;
        if (sinAngle2 <= m_sinAngle2Closest && distance < m_closestDistance)
        {
            m_closestStar = &star;
            m_closestDistance = distance;
            m_sinAngle2Closest = std::max(sinAngle2, ANGULAR_RES);
        }
    }
}

inline const Star*
CloseStarPicker::closestStar() const
{
    return m_closestStar;
}

class DSOPicker : private VisibleObjectVisitor<double>
{
public:
    DSOPicker(const UniversalCoord&,
              const Eigen::Vector3d&,
              float fov,
              float faintestMag,
              std::uint64_t renderFlags);

    using VisibleObjectVisitor::checkNode;
    void process(const std::unique_ptr<DeepSkyObject>&);

    DeepSkyObject* pickedDSO() const;

private:
    Eigen::Vector3d m_pickDir;
    std::uint64_t m_renderFlags;
    double m_sinAngle2Closest;
    DeepSkyObject* m_pickedDSO{ nullptr };
};

DSOPicker::DSOPicker(const UniversalCoord& pos,
                     const Eigen::Vector3d& direction,
                     float fov,
                     float faintestMag,
                     std::uint64_t renderFlags) :
    VisibleObjectVisitor(pos,
                         Eigen::Quaternionf::FromTwoVectors(-Eigen::Vector3f::UnitZ(), direction.cast<float>()).conjugate(),
                         fov,
                         1.0f,
                         std::numeric_limits<double>::max(),
                         faintestMag),
    m_pickDir(direction),
    m_renderFlags(renderFlags),
    m_sinAngle2Closest(std::max(std::sin(fov * 0.5), ANGULAR_RES))
{
}

void
DSOPicker::process(const std::unique_ptr<DeepSkyObject>& dso)
{
    if ((dso->getRenderMask() & m_renderFlags) == 0 || !dso->isVisible() || !dso->isClickable())
        return;

    Eigen::Vector3d relativeDSOPos = dso->getPosition() - m_observerPos;
    Eigen::Vector3d dsoDir = relativeDSOPos;

    if (double distance2 = 0.0;
        math::testIntersection(Eigen::ParametrizedLine<double, 3>(Eigen::Vector3d::Zero(), m_pickDir),
                               math::Sphered(relativeDSOPos, (double) dso->getRadius()), distance2))
    {
        Eigen::Vector3d dsoPos = dso->getPosition();
        dsoDir = dsoPos * 1.0e-6 - m_observerPos; // wtf???
    }
    dsoDir.normalize();

    double sinAngle2 = (dsoDir - m_pickDir).norm() * 0.5;
    if (sinAngle2 <= m_sinAngle2Closest)
    {
        m_sinAngle2Closest = std::max(sinAngle2, ANGULAR_RES);
        m_pickedDSO        = dso.get();
    }
}

inline DeepSkyObject*
DSOPicker::pickedDSO() const
{
    return m_pickedDSO;
}

class CloseDSOPicker
{
public:
    CloseDSOPicker(const Eigen::Vector3d&,
                   const Eigen::Vector3d&,
                   double,
                   std::uint64_t);

    bool checkNode(const Eigen::Vector3d&, double, float) const;
    void process(const std::unique_ptr<DeepSkyObject>&);

    DeepSkyObject* closestDSO() const;

private:
    Eigen::Vector3d m_position;
    Eigen::Vector3d m_pickDir;
    double m_maxDistance;
    double m_maxDistance2;
    std::uint64_t m_renderFlags;
    DeepSkyObject* m_closestDSO{ nullptr };
    double m_largestCosAngle{ -2.0 };
};

CloseDSOPicker::CloseDSOPicker(const Eigen::Vector3d& position,
                               const Eigen::Vector3d& direction,
                               double maxDistance,
                               std::uint64_t renderFlags) :
    m_position(position),
    m_pickDir(direction),
    m_maxDistance(maxDistance),
    m_maxDistance2(math::square(maxDistance)),
    m_renderFlags(renderFlags)
{
}

bool
CloseDSOPicker::checkNode(const Eigen::Vector3d& center,
                          double size,
                          float /* magnitude */) const
{
    return checkNodeDistance(center, m_position, size, m_maxDistance);
}

void
CloseDSOPicker::process(const std::unique_ptr<DeepSkyObject>& dso)
{
    if (!(dso->getRenderMask() & m_renderFlags) || !dso->isVisible() || !dso->isClickable())
        return;

    auto distance2 = (dso->getPosition() - m_position).squaredNorm();
    if (distance2 > m_maxDistance2)
        return;

    double distanceToPicker       = 0.0;
    double cosAngleToBoundCenter  = 0.0;
    if (!dso->pick(Eigen::ParametrizedLine<double, 3>(m_position, m_pickDir), distanceToPicker, cosAngleToBoundCenter))
        return;

    // Don't select the object the observer is currently in:
    if (distance2 > math::square(dso->getRadius()) && cosAngleToBoundCenter > m_largestCosAngle)
    {
        m_closestDSO      = dso.get();
        m_largestCosAngle = cosAngleToBoundCenter;
    }
}

inline DeepSkyObject*
CloseDSOPicker::closestDSO() const
{
    return m_closestDSO;
}

#if 0
class DSOPicker : public DSOHandler
{
public:
    DSOPicker(const Eigen::Vector3d& pickOrigin,
              const Eigen::Vector3d& pickDir,
              std::uint64_t renderFlags,
              float angle);
    ~DSOPicker() = default;

    void process(DeepSkyObject* const &, double, float) override;

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
DSOPicker::process(DeepSkyObject* const & dso, double /*unused*/, float /*unused*/)
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
        pickedDSO        = dso;
    }
}

#endif

void
getLocationsCompletion(std::vector<std::string>& completion,
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
            completion.push_back(name);
        }
        else
        {
            const std::string& lname = location->getName(true);
            if (lname != name && UTF8StartsWith(lname, s))
                completion.push_back(lname);
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
    starCatalog->getOctree().processDepthFirst(closePicker);
    if (auto star = closePicker.closestStar(); star != nullptr)
        return Selection(const_cast<Star*>(star));

    // Find visible stars expects an orientation, but we just have a direction
    // vector.  Convert the direction vector into an orientation by computing
    // the rotation required to map -Z to the direction.
    Eigen::Quaternionf rotation;
    rotation.setFromTwoVectors(-Eigen::Vector3f::UnitZ(), direction);

    StarPicker picker(origin, direction, tolerance, faintestMag, when);
    starCatalog->getOctree().processDepthFirst(picker);
    /*starCatalog->findVisibleStars(picker,
                                  o,
                                  rotation.conjugate(),
                                  tolerance, 1.0f,
                                  faintestMag);*/
    if (auto star = picker.pickedStar(); star != nullptr)
        return Selection(const_cast<Star*>(star));

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

    CloseDSOPicker closePicker(orig, dir, 1e9, renderFlags);
    dsoCatalog->getOctree().processDepthFirst(closePicker);
    if (auto dso = closePicker.closestDSO(); dso != nullptr)
        return Selection(dso);

    DSOPicker picker(origin, dir, tolerance, faintestMag, renderFlags);
    dsoCatalog->getOctree().processDepthFirst(picker);
    if (auto dso = picker.pickedDSO(); dso != nullptr)
        return Selection(dso);

    return Selection();
}

Selection
Universe::pick(const UniversalCoord& origin,
               const Eigen::Vector3f& direction,
               double when,
               std::uint64_t renderFlags,
               float faintestMag,
               float tolerance)
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
        sel = pickStar(origin, direction, when, faintestMag, tolerance);

    if (sel.empty())
        sel = pickDeepSkyObject(origin, direction, renderFlags, faintestMag, tolerance);

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
Universe::getCompletion(std::vector<std::string>& completion,
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
Universe::getCompletionPath(std::vector<std::string>& completion,
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
        completion.push_back(dsoCatalog->getDSOName(sel.deepsky()));
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
    ClosestSystemFinder finder(pos, 1.0f, this);
    starCatalog->getOctree().processDepthFirst(finder);
    return getSolarSystem(finder.best());
}

void
Universe::getNearStars(const UniversalCoord& position,
                       float maxDistance,
                       std::vector<const Star*>& nearStars) const
{
    Eigen::Vector3f pos = position.toLy().cast<float>();
    NearStarFinder finder(pos, maxDistance, &nearStars);
    starCatalog->getOctree().processDepthFirst(finder);
}
