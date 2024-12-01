// body.cpp
//
// Copyright (C) 2001-2006 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "body.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <utility>

#include <celastro/astro.h>
#include <celcompat/numbers.h>
#include <celephem/orbit.h>
#include <celephem/rotation.h>
#include <celmath/mathlib.h>
#include <celutil/gettext.h>
#include "atmosphere.h"
#include "frame.h"
#include "frametree.h"
#include "geometry.h"
#include "location.h"
#include "meshmanager.h"
#include "referencemark.h"
#include "selection.h"
#include "star.h"
#include "stardb.h"
#include "timeline.h"
#include "timelinephase.h"
#include "univcoord.h"

namespace astro = celestia::astro;
namespace engine = celestia::engine;
namespace numbers = celestia::numbers;
namespace math = celestia::math;
namespace util = celestia::util;

namespace
{

const Color defaultCometTailColor(0.5f, 0.5f, 0.75f);

constexpr auto CLASSES_VISIBLE_AS_POINT = ~(BodyClassification::Invisible      |
                                            BodyClassification::SurfaceFeature |
                                            BodyClassification::Component      |
                                            BodyClassification::Diffuse);

constexpr auto CLASSES_SECONDARY_ILLUMINATOR = BodyClassification::Planet      |
                                               BodyClassification::Moon        |
                                               BodyClassification::MinorMoon   |
                                               BodyClassification::DwarfPlanet |
                                               BodyClassification::Asteroid    |
                                               BodyClassification::Comet;

}

Body::Body(PlanetarySystem* _system, const std::string& _name) :
    system(_system),
    orbitVisibility(UseClassVisibility)
{
    setName(_name);
    recomputeCullingRadius();
}

Body::~Body()
{
    auto bodyFeaturesManager = GetBodyFeaturesManager();
    bodyFeaturesManager->removeFeatures(this);
}

/*! Reset body attributes to their default values. The object hierarchy is left untouched,
 *  i.e. child objects are not removed. Alternate surfaces and locations are not removed
 *  either.
 */
void
Body::setDefaultProperties()
{
    radius = 1.0f;
    semiAxes = Eigen::Vector3f::Ones();
    mass = 0.0f;
    density = 0.0f;
    bondAlbedo = 0.5f;
    geomAlbedo = 0.5f;
    reflectivity = 0.5f;
    temperature = 0.0f;
    tempDiscrepancy = 0.0f;
    geometryOrientation = Eigen::Quaternionf::Identity();
    geometry = InvalidResource;
    surface = Surface(Color::White);
    auto manager = GetBodyFeaturesManager();
    manager->setAtmosphere(this, nullptr);
    manager->setRings(this, nullptr);
    classification = BodyClassification::Unknown;
    visible = true;
    clickable = true;
    manager->unsetOrbitColor(this);
    manager->unsetCometTailColor(this);
    orbitVisibility = UseClassVisibility;
    recomputeCullingRadius();
}

/*! Return the list of all names (non-localized) by which this
 *  body is known.
 */
const std::vector<std::string>&
Body::getNames() const
{
    return names;
}

/*! Return the primary name for the body; if i18n, return the
 *  localized name of the body.
 */
const std::string&
Body::getName(bool i18n) const
{
    if (i18n && hasLocalizedName())
        return localizedName;
    return names[0];
}

std::string
Body::getPath(const StarDatabase* starDB, char delimiter) const
{
    std::string name = names[0];
    const PlanetarySystem* planetarySystem = system;
    while (planetarySystem != nullptr)
    {
        if (const Body* parent = planetarySystem->getPrimaryBody(); parent != nullptr)
        {
            name = parent->getName() + delimiter + name;
            planetarySystem = parent->getSystem();
        }
        else
        {
            if (const Star* parentStar = system->getStar(); parentStar != nullptr)
                name = starDB->getStarName(*parentStar) + delimiter + name;
            break;
        }
    }

    return name;
}

/*! Get the localized name for the body. If no localized name
 *  has been set, the primary name is returned.
 */
const std::string&
Body::getLocalizedName() const
{
    return hasLocalizedName() ? localizedName : names[0];
}

bool
Body::hasLocalizedName() const
{
    return !localizedName.empty();
}

/*! Set the primary name of the body. The localized name is updated
 *  automatically as well.
 *  Note: setName() is private, and only called from the Body constructor.
 *  It shouldn't be called elsewhere.
 */
void
Body::setName(const std::string& name)
{
    names[0] = name;

    // Gettext uses the empty string to store various metadata, so don't try
    // to translate it.
    if (name.empty())
    {
        localizedName = {};
        return;
    }

    if (auto locName = D_(name.c_str()); locName == name)
    {
        // No localized name
        localizedName = {};
    }
    else
    {
        localizedName = locName;
    }
}

/*! Add a new name for this body. Aliases are non localized.
 */
void
Body::addAlias(const std::string& alias)
{
    // Don't add an alias if it matches the primary name
    if (alias != names[0])
    {
        names.push_back(alias);
        system->addAlias(this, alias);
    }
}

PlanetarySystem*
Body::getSystem() const
{
    return system;
}

FrameTree*
Body::getFrameTree() const
{
    return frameTree.get();
}

FrameTree*
Body::getOrCreateFrameTree()
{
    if (!frameTree)
        frameTree = std::make_unique<FrameTree>(this);
    return frameTree.get();
}

const Timeline*
Body::getTimeline() const
{
    return timeline.get();
}

void
Body::setTimeline(std::unique_ptr<Timeline>&& newTimeline)
{
    timeline = std::move(newTimeline);
    markChanged();
}

void
Body::markChanged()
{
    if (timeline)
        timeline->markChanged();
}

void
Body::markUpdated()
{
    if (frameTree)
        frameTree->markUpdated();
}

const std::shared_ptr<const ReferenceFrame>&
Body::getOrbitFrame(double tdb) const
{
    return timeline->findPhase(tdb)->orbitFrame();
}

const celestia::ephem::Orbit*
Body::getOrbit(double tdb) const
{
    return timeline->findPhase(tdb)->orbit().get();
}

const std::shared_ptr<const ReferenceFrame>&
Body::getBodyFrame(double tdb) const
{
    return timeline->findPhase(tdb)->bodyFrame();
}

const celestia::ephem::RotationModel*
Body::getRotationModel(double tdb) const
{
    return timeline->findPhase(tdb)->rotationModel().get();
}

/*! Get the radius of a sphere large enough to contain the primary
 *  geometry of the object: either a mesh or an ellipsoid.
 *  For an irregular (mesh) object, the radius is defined to be
 *  the largest semi-axis of the axis-aligned bounding box.  The
 *  radius of the smallest sphere containing the object is potentially
 *  larger by a factor of sqrt(3).
 *
 *  This method does not consider additional object features
 *  such as rings, atmospheres, or reference marks; use
 *  getCullingRadius() for that.
 */
float
Body::getBoundingRadius() const
{
    if (geometry == InvalidResource)
        return radius;

    return radius * numbers::sqrt3_v<float>;
}

/*! Return the radius of sphere large enough to contain any geometry
 *  associated with this object: the primary geometry, comet tail,
 *  rings, atmosphere shell, cloud layers, or reference marks.
 */
float
Body::getCullingRadius() const
{
    return cullingRadius;
}

float
Body::getMass() const
{
    return mass;
}

void
Body::setMass(float _mass)
{
    mass = _mass;
}

float
Body::getDensity() const
{
    if (density > 0.0f)
        return density;

    if (radius == 0.0f || !isEllipsoid())
        return 0.0f;

    // @mass unit is mass of Earth
    // @astro::EarthMass unit is kg
    // @radius unit km
    // so we divide density by 1e9 to have kg/m^3
    float volume = 4.0f / 3.0f * numbers::pi_v<float> * semiAxes.prod();
    return volume == 0.0f ? 0.0f : mass * static_cast<float>(astro::EarthMass / 1e9) / volume;
}

void
Body::setDensity(float _density)
{
    density = _density;
}

float
Body::getGeomAlbedo() const
{
    return geomAlbedo;
}

void
Body::setGeomAlbedo(float _geomAlbedo)
{
    geomAlbedo = _geomAlbedo;
}

float
Body::getBondAlbedo() const
{
    return bondAlbedo;
}

void
Body::setBondAlbedo(float _bondAlbedo)
{
    bondAlbedo = _bondAlbedo;
}

float
Body::getReflectivity() const
{
    return reflectivity;
}

void
Body::setReflectivity(float _reflectivity)
{
    reflectivity = _reflectivity;
}

float
Body::getTemperature(double time) const
{
    if (temperature > 0)
        return temperature;

    const PlanetarySystem* system = getSystem();
    if (system == nullptr)
        return 0;

    const Star* sun = system->getStar();
    if (sun == nullptr)
        return 0;

    float temp = 0.0f;
    if (sun->getVisibility()) // the sun is a star
    {
        auto distFromSun = static_cast<float>(getAstrocentricPosition(time).norm());
        temp = sun->getTemperature() *
               std::pow(1.0f - getBondAlbedo(), 0.25f) *
               std::sqrt(sun->getRadius() / (2.0f * distFromSun));
    }
    else // the sun is a barycenter
    {
        auto orbitingStars = sun->getOrbitingStars();
        if (orbitingStars.empty())
            return 0.0f;

        const UniversalCoord bodyPos = getPosition(time);
        float flux = 0.0f;
        for (const auto *s : orbitingStars)
        {
            auto distFromSun = static_cast<float>(s->getPosition(time).distanceFromKm(bodyPos));
            float lum = math::square(s->getRadius() * math::square(s->getTemperature()));
            flux += lum / math::square(distFromSun);
        }
        temp = std::pow((1.0f - getBondAlbedo()) * flux, 0.25f) * (numbers::sqrt2_v<float> * 0.5f);
    }
    return getTempDiscrepancy() + temp;
}

void
Body::setTemperature(float _temperature)
{
    temperature = _temperature;
}

float
Body::getTempDiscrepancy() const
{
    return tempDiscrepancy;
}

void
Body::setTempDiscrepancy(float _tempDiscrepancy)
{
    tempDiscrepancy = _tempDiscrepancy;
}

Eigen::Quaternionf
Body::getGeometryOrientation() const
{
    return geometryOrientation;
}

void
Body::setGeometryOrientation(const Eigen::Quaternionf& orientation)
{
    geometryOrientation = orientation;
}

/*! Set the semiaxes of a body.
 */
void
Body::setSemiAxes(const Eigen::Vector3f& _semiAxes)
{
    semiAxes = _semiAxes;

    // Radius will always be the largest of the three semi axes
    radius = semiAxes.maxCoeff();
    recomputeCullingRadius();
}

/*! Retrieve the body's semiaxes
 */
const Eigen::Vector3f&
Body::getSemiAxes() const
{
    return semiAxes;
}

/*! Get the radius of the body. For a spherical body, this is simply
 *  the sphere's radius. For an ellipsoidal body, the radius is the
 *  largest of the three semiaxes. For irregular bodies (with a shape
 *  represented by a mesh), the radius is the largest semiaxis of the
 *  mesh's axis aligned bounding axis. Note that this means some portions
 *  of the mesh may extend outside the sphere of the retrieved radius.
 *  To obtain the radius of a sphere that will definitely enclose the
 *  body, call getBoundingRadius() instead.
 */
float
Body::getRadius() const
{
    return radius;
}

/*! Return true if the body is a perfect sphere.
*/
bool
Body::isSphere() const
{
    return (geometry == InvalidResource) &&
           (semiAxes.x() == semiAxes.y()) &&
           (semiAxes.x() == semiAxes.z());
}

/*! Return true if the body is ellipsoidal, with geometry determined
 *  completely by its semiaxes rather than a triangle based model.
 */
bool
Body::isEllipsoid() const
{
    return geometry == InvalidResource;
}

const
Surface& Body::getSurface() const
{
    return surface;
}

Surface&
Body::getSurface()
{
    return surface;
}

void
Body::setSurface(const Surface& surf)
{
    surface = surf;
}

void
Body::setGeometry(ResourceHandle _geometry)
{
    geometry = _geometry;
}

/*! Set the scale factor for geometry; this is only used with unnormalized meshes.
 *  When a mesh is normalized, the effective scale factor is the radius.
 */
void
Body::setGeometryScale(float scale)
{
    geometryScale = scale;
}

PlanetarySystem*
Body::getSatellites() const
{
    return satellites.get();
}

PlanetarySystem*
Body::getOrCreateSatellites()
{
    if (satellites == nullptr)
        satellites = std::make_unique<PlanetarySystem>(this);
    return satellites.get();
}

// The following four functions are used to get the state of the body
// in universal coordinates:
//    * getPosition
//    * getOrientation
//    * getVelocity
//    * getAngularVelocity

/*! Get the position of the body in the universal coordinate system.
 *  This method uses high-precision coordinates and is thus
 *  slower relative to getAstrocentricPosition(), which works strictly
 *  with standard double precision. For most purposes,
 *  getAstrocentricPosition() should be used instead of the more
 *  general getPosition().
 */
UniversalCoord
Body::getPosition(double tdb) const
{
    Eigen::Vector3d position = Eigen::Vector3d::Zero();

    const TimelinePhase* phase = timeline->findPhase(tdb).get();
    Eigen::Vector3d p = phase->orbit()->positionAtTime(tdb);
    const ReferenceFrame* frame = phase->orbitFrame().get();

    while (frame->getCenter().getType() == SelectionType::Body)
    {
        phase = frame->getCenter().body()->timeline->findPhase(tdb).get();
        position += frame->getOrientation(tdb).conjugate() * p;
        p = phase->orbit()->positionAtTime(tdb);
        frame = phase->orbitFrame().get();
    }

    position += frame->getOrientation(tdb).conjugate() * p;

    if (frame->getCenter().star())
        return frame->getCenter().star()->getPosition(tdb).offsetKm(position);
    else
        return frame->getCenter().getPosition(tdb).offsetKm(position);
}

/*! Get the orientation of the body in the universal coordinate system.
 */
Eigen::Quaterniond
Body::getOrientation(double tdb) const
{
    const TimelinePhase* phase = timeline->findPhase(tdb).get();
    return phase->rotationModel()->orientationAtTime(tdb) * phase->bodyFrame()->getOrientation(tdb);
}

/*! Get the velocity of the body in the universal frame.
 */
Eigen::Vector3d
Body::getVelocity(double tdb) const
{
    const TimelinePhase* phase = timeline->findPhase(tdb).get();

    const ReferenceFrame* orbitFrame = phase->orbitFrame().get();

    Eigen::Vector3d v = phase->orbit()->velocityAtTime(tdb);
    v = orbitFrame->getOrientation(tdb).conjugate() * v + orbitFrame->getCenter().getVelocity(tdb);

    if (!orbitFrame->isInertial())
    {
        Eigen::Vector3d r = getPosition(tdb).offsetFromKm(orbitFrame->getCenter().getPosition(tdb));
        v += orbitFrame->getAngularVelocity(tdb).cross(r);
    }

    return v;
}

/*! Get the angular velocity of the body in the universal frame.
 */
Eigen::Vector3d
Body::getAngularVelocity(double tdb) const
{
    const TimelinePhase* phase = timeline->findPhase(tdb).get();

    Eigen::Vector3d v = phase->rotationModel()->angularVelocityAtTime(tdb);

    const ReferenceFrame* bodyFrame = phase->bodyFrame().get();
    v = bodyFrame->getOrientation(tdb).conjugate() * v;
    if (!bodyFrame->isInertial())
    {
        v += bodyFrame->getAngularVelocity(tdb);
    }

    return v;
}

/*! Get the transformation which converts body coordinates into
 *  astrocentric coordinates. Some clarification on the meaning
 *  of 'astrocentric': the position of every solar system body
 *  is ultimately defined with respect to some star or star
 *  system barycenter.
 */
Eigen::Matrix4d
Body::getLocalToAstrocentric(double tdb) const
{
    const TimelinePhase* phase = timeline->findPhase(tdb).get();
    Eigen::Vector3d p = phase->orbitFrame()->convertToAstrocentric(phase->orbit()->positionAtTime(tdb), tdb);
    return Eigen::Transform<double, 3, Eigen::Affine>(Eigen::Translation3d(p)).matrix();
}

/*! Get the position of the center of the body in astrocentric ecliptic coordinates.
 */
Eigen::Vector3d
Body::getAstrocentricPosition(double tdb) const
{
    // TODO: Switch the iterative method used in getPosition
    const TimelinePhase* phase = timeline->findPhase(tdb).get();
    return phase->orbitFrame()->convertToAstrocentric(phase->orbit()->positionAtTime(tdb), tdb);
}

/*! Get a rotation that converts from the ecliptic frame to the body frame.
 */
Eigen::Quaterniond
Body::getEclipticToFrame(double tdb) const
{
    const TimelinePhase* phase = timeline->findPhase(tdb).get();
    return phase->bodyFrame()->getOrientation(tdb);
}

/*! Get a rotation that converts from the ecliptic frame to the body's
 *  mean equatorial frame.
 */
Eigen::Quaterniond
Body::getEclipticToEquatorial(double tdb) const
{
    const TimelinePhase* phase = timeline->findPhase(tdb).get();
    return phase->rotationModel()->equatorOrientationAtTime(tdb) * phase->bodyFrame()->getOrientation(tdb);
}

// The body-fixed coordinate system has an origin at the center of the
// body, y-axis parallel to the rotation axis, x-axis through the prime
// meridian, and z-axis at a right angle the xy plane.
Eigen::Quaterniond
Body::getEquatorialToBodyFixed(double tdb) const
{
    const TimelinePhase* phase = timeline->findPhase(tdb).get();
    return phase->rotationModel()->spin(tdb);
}

/*! Get a transformation to convert from the object's body fixed frame
 *  to the astrocentric ecliptic frame.
 */
Eigen::Matrix4d
Body::getBodyFixedToAstrocentric(double tdb) const
{
    Eigen::Matrix4d m = Eigen::Affine3d(getEquatorialToBodyFixed(tdb)).matrix();
    return m * getLocalToAstrocentric(tdb);
}

Eigen::Vector3d
Body::planetocentricToCartesian(double lon, double lat, double alt) const
{
    using celestia::numbers::pi;
    double sphi;
    double cphi;
    math::sincos(-math::degToRad(lat) + pi * 0.5, sphi, cphi);
    double stheta;
    double ctheta;
    math::sincos(math::degToRad(lon) - pi, stheta, ctheta);

    Eigen::Vector3d pos(ctheta * sphi,
                        cphi,
                        -stheta * sphi);

    return pos * (getRadius() + alt);
}

Eigen::Vector3d
Body::planetocentricToCartesian(const Eigen::Vector3d& lonLatAlt) const
{
    return planetocentricToCartesian(lonLatAlt.x(), lonLatAlt.y(), lonLatAlt.z());
}

/*! Convert planetocentric coordinates to geodetic ones.
 *
 * Formulae are taken from DOI 10.1007/s00190-011-0514-7.
 *
 * @param lon longitude
 * @param lat latitude
 * @param alt altitude (height above the surface)
 *
 * @return geodetic coordinates
 */
Eigen::Vector3d
Body::geodeticToCartesian(double lon, double lat, double alt) const
{
    using celestia::numbers::pi;
    double phi = math::degToRad(lat);
    double theta = math::degToRad(lon) + pi;
    double a2x = math::square(semiAxes.x());
    double a2y = math::square(semiAxes.z()); // swap y & z to convert from Celestia axes
    double b2  = math::square(semiAxes.y());
    double e2x = (a2x - b2) / a2x;
    double e2e = (a2x - a2y) / a2x;
    double sinphi, cosphi;
    math::sincos(phi, sinphi, cosphi);
    double sintheta, costheta;
    math::sincos(theta, sintheta, costheta);
    double v = semiAxes.x() / std::sqrt(1.0 - e2x * math::square(sinphi) - e2e  * math::square(cosphi) * math::square(sintheta));
    double xg = (v + alt) * cosphi * costheta;
    double yg = (v * (1.0 - e2e) + alt) * cosphi * sintheta;
    double zg = (v * (1.0 - e2x) + alt) * sinphi;
    return { xg, zg, -yg }; // convert to Celestia coordinates
}

Eigen::Vector3d
Body::geodeticToCartesian(const Eigen::Vector3d& lonLatAlt) const
{
    return geodeticToCartesian(lonLatAlt.x(), lonLatAlt.y(), lonLatAlt.z());
}

/*! Convert cartesian body-fixed coordinates to spherical planetocentric
 *  coordinates.
 */
Eigen::Vector3d
Body::cartesianToPlanetocentric(const Eigen::Vector3d& v) const
{
    Eigen::Vector3d w = v.normalized();

    double lat = numbers::pi / 2.0 - std::acos(w.y());
    double lon = std::atan2(w.z(), -w.x());

    return Eigen::Vector3d(lon, lat, v.norm() - getRadius());
}

/*! Convert body-centered ecliptic coordinates to spherical planetocentric
 *  coordinates.
 */
Eigen::Vector3d
Body::eclipticToPlanetocentric(const Eigen::Vector3d& ecl, double tdb) const
{
    Eigen::Vector3d bf = getEclipticToBodyFixed(tdb) * ecl;
    return cartesianToPlanetocentric(bf);
}

bool
Body::extant(double t) const
{
    return timeline->includes(t);
}

void
Body::getLifespan(double& begin, double& end) const
{
    begin = timeline->startTime();
    end = timeline->endTime();
}

bool
Body::isVisibleAsPoint() const
{
    return util::is_set(classification, CLASSES_VISIBLE_AS_POINT);
}

bool
Body::isSecondaryIlluminator() const
{
    return util::is_set(classification, CLASSES_SECONDARY_ILLUMINATOR);
}

float
Body::getLuminosity(const Star& sun,
                    float distanceFromSun) const
{
    return getLuminosity(sun.getLuminosity(), distanceFromSun);
}

float
Body::getLuminosity(float sunLuminosity,
                    float distanceFromSun) const
{
    // Compute the total power of the star in Watts
    double power = astro::SOLAR_POWER * sunLuminosity;

    // Compute the irradiance at a distance of 1au from the star in W/m^2
    // double irradiance = power / sphereArea(astro::AUtoKilometers(1.0) * 1000);

    // Compute the irradiance at the body's distance from the star
    double satIrradiance = power / math::sphereArea(distanceFromSun * 1000);

    // Compute the total energy hitting the planet
    double incidentEnergy = satIrradiance * math::circleArea(radius * 1000);

    double reflectedEnergy = incidentEnergy * getReflectivity();

    // Compute the luminosity (i.e. power relative to solar power)
    return static_cast<float>(reflectedEnergy / astro::SOLAR_POWER);
}

/*! Get the apparent magnitude of the body, neglecting the phase (as if
 *  the body was at opposition.
 */
float
Body::getApparentMagnitude(const Star& sun,
                           float distanceFromSun,
                           float distanceFromViewer) const
{
    return astro::lumToAppMag(getLuminosity(sun, distanceFromSun),
                              astro::kilometersToLightYears(distanceFromViewer));
}

/*! Get the apparent magnitude of the body, neglecting the phase (as if
 *  the body was at opposition.
 */
float
Body::getApparentMagnitude(float sunLuminosity,
                           float distanceFromSun,
                           float distanceFromViewer) const
{
    return astro::lumToAppMag(getLuminosity(sunLuminosity, distanceFromSun),
                              astro::kilometersToLightYears(distanceFromViewer));
}

/*! Get the apparent magnitude of the body, corrected for its phase.
 */
float
Body::getApparentMagnitude(const Star& sun,
                           const Eigen::Vector3d& sunPosition,
                           const Eigen::Vector3d& viewerPosition) const
{
    return getApparentMagnitude(sun.getLuminosity(),
                                sunPosition,
                                viewerPosition);
}

/*! Get the apparent magnitude of the body, corrected for its phase.
 */
float
Body::getApparentMagnitude(float sunLuminosity,
                           const Eigen::Vector3d& sunPosition,
                           const Eigen::Vector3d& viewerPosition) const
{
    double distanceToViewer = viewerPosition.norm();
    double distanceToSun = sunPosition.norm();
    auto illuminatedFraction = static_cast<float>(1.0 + (viewerPosition / distanceToViewer).dot(sunPosition / distanceToSun))
                             * 0.5f;

    return astro::lumToAppMag(getLuminosity(sunLuminosity, (float) distanceToSun) * illuminatedFraction,
                              static_cast<float>(astro::kilometersToLightYears(distanceToViewer)));
}

BodyClassification
Body::getClassification() const
{
    return classification;
}

void
Body::setClassification(BodyClassification _classification)
{
    classification = _classification;
    recomputeCullingRadius();
    markChanged();
}

/*! Return the effective classification of this body used when rendering
 *  orbits. Normally, this is just the classification of the object, but
 *  invisible objects are treated specially: they behave as if they have
 *  the classification of their child objects. This fixes annoyances when
 *  planets are defined with orbits relative to their system barycenters.
 *  For example, Pluto's orbit can seen in a solar system scale view, even
 *  though its orbit is defined relative to the Pluto-Charon barycenter
 *  and is this just a few hundred kilometers in size.
 */
BodyClassification
Body::getOrbitClassification() const
{
    if (classification != BodyClassification::Invisible || !frameTree)
        return classification;

    BodyClassification orbitClass = frameTree->childClassMask();
    if (util::is_set(orbitClass, BodyClassification::Planet))
        return BodyClassification::Planet;
    if (util::is_set(orbitClass, BodyClassification::DwarfPlanet))
        return BodyClassification::DwarfPlanet;
    if (util::is_set(orbitClass, BodyClassification::Asteroid))
        return BodyClassification::Asteroid;
    if (util::is_set(orbitClass, BodyClassification::Moon))
        return BodyClassification::Moon;
    if (util::is_set(orbitClass, BodyClassification::MinorMoon))
        return BodyClassification::MinorMoon;
    if (util::is_set(orbitClass, BodyClassification::Spacecraft))
        return BodyClassification::Spacecraft;
    return BodyClassification::Invisible;
}

const std::string&
Body::getInfoURL() const
{
    return infoURL;
}

void
Body::setInfoURL(std::string&& _infoURL)
{
    infoURL = std::move(_infoURL);
}

/*! Sets whether or not the object is visible.
 */
void
Body::setVisible(bool _visible)
{
    visible = _visible;
}

/*! Sets whether or not the object can be selected by clicking on
 *  it. If set to false, the object is completely ignored when the
 *  user clicks it, making it possible to select background objects.
 */
void
Body::setClickable(bool _clickable)
{
    clickable = _clickable;
}

/*! Set the visibility policy for the orbit of this object:
 *  - NeverVisible: Never show the orbit of this object.
 *  - UseClassVisibility: (Default) Show the orbit of this object
 *    its class is enabled in the orbit mask.
 *  - AlwaysVisible: Always show the orbit of this object whenever
 *    orbit paths are enabled.
 */
void
Body::setOrbitVisibility(VisibilityPolicy _orbitVisibility)
{
    orbitVisibility = _orbitVisibility;
}

void
Body::recomputeCullingRadius()
{
    float r = getBoundingRadius();

    const BodyFeaturesManager* manager = GetBodyFeaturesManager();
    if (auto atmosphere = manager->getAtmosphere(this); atmosphere != nullptr)
        r += std::max(atmosphere->height, atmosphere->cloudHeight);

    if (auto rings = manager->getRings(this); rings != nullptr)
        r = std::max(r, rings->outerRadius);

    manager->processReferenceMarks(this,
                                   [&r](const ReferenceMark* rm)
                                   {
                                       r = std::max(r, rm->boundingSphereRadius());
                                   });

    if (classification == BodyClassification::Comet)
        r = std::max(r, astro::AUtoKilometers(1.0f));

    if (r != cullingRadius)
    {
        cullingRadius = r;
        markChanged();
    }
}

/**** Implementation of PlanetarySystem ****/

/*! Return the equatorial frame for this object. This frame is used as
 *  the default frame for objects in SSC files that orbit non-stellar bodies.
 *  In order to avoid wasting memory, it is created until the first time it
 *  is requested.
 */

PlanetarySystem::PlanetarySystem(Body* _primary) :
    star(nullptr),
    primary(_primary)
{
    if (primary && primary->getSystem())
        star = primary->getSystem()->getStar();
}

PlanetarySystem::PlanetarySystem(Star* _star) :
    star(_star)
{
}

PlanetarySystem::~PlanetarySystem() = default;

/*! Add a new alias for an object. If an object with the specified
 *  alias already exists in the planetary system, the old entry will
 *  be replaced.
 */
void
PlanetarySystem::addAlias(Body* body, const std::string& alias)
{
    assert(body->getSystem() == this);

    objectIndex.try_emplace(alias, body);
}

Body*
PlanetarySystem::addBody(const std::string& name)
{
    auto body = std::make_unique<Body>(this, name);
    addBodyToNameIndex(body.get());
    return satellites.emplace_back(std::move(body)).get();
}

void
PlanetarySystem::removeBody(const Body* body)
{
    if (body->getSystem() != this)
        return;
    auto iter = std::find_if(satellites.begin(), satellites.end(),
                             [body](const auto& sat) { return sat.get() == body; });
    if (iter == satellites.end())
        return;

    removeBodyFromNameIndex(body);
    satellites.erase(iter);
}

// Add all aliases for the body to the name index
void
PlanetarySystem::addBodyToNameIndex(Body* body)
{
    const std::vector<std::string>& names = body->getNames();
    for (const auto& name : names)
    {
        objectIndex.try_emplace(name, body);
    }
}

void
PlanetarySystem::removeBodyFromNameIndex(const Body* body)
{
    const std::vector<std::string>& names = body->getNames();
    for (const auto& name : names)
    {
        auto iter = objectIndex.find(name);
        if (iter == objectIndex.end() || iter->second != body)
            continue;
        objectIndex.erase(iter);
    }
}

/*! Find a body with the specified name within a planetary system.
 *
 *  deepSearch: if true, recursively search the systems of child objects
 *  i18n: if true, allow matching of localized body names. When responding
 *    to a user query, this flag should be true. In other cases--such
 *    as resolving an object name in an ssc file--it should be false. Otherwise,
 *    object lookup will behave differently based on the locale.
 */
Body*
PlanetarySystem::find(std::string_view _name, bool deepSearch, bool i18n) const
{
    if (auto firstMatch = objectIndex.find(_name); firstMatch != objectIndex.end())
    {
        Body* matchedBody = firstMatch->second;

        if (i18n)
            return matchedBody;
        // Ignore localized names
        if (!matchedBody->hasLocalizedName() || _name != matchedBody->getLocalizedName())
            return matchedBody;
    }

    if (deepSearch)
    {
        for (const auto& satellite : satellites)
        {
            Body* sat = satellite.get();
            if (!UTF8StringCompare(sat->getName(false), _name))
                return sat;
            if (i18n && !UTF8StringCompare(sat->getName(true), _name))
                return sat;
            if (sat->getSatellites())
            {
                Body* body = sat->getSatellites()->find(_name, deepSearch, i18n);
                if (body)
                    return body;
            }
        }
    }

    return nullptr;
}

void
PlanetarySystem::getCompletion(std::vector<celestia::engine::Completion>& completion,
                               std::string_view _name,
                               bool deepSearch) const
{
    // Search through all names in this planetary system.
    for (const auto& index : objectIndex)
    {
        const std::string& alias = index.first;

        if (UTF8StartsWith(alias, _name))
        {
            completion.emplace_back(alias, Selection(index.second));
        }
        else
        {
            std::string lname = D_(alias.c_str());
            if (lname != alias && UTF8StartsWith(lname, _name))
                completion.emplace_back(lname, Selection(index.second));
        }
    }

    if (!deepSearch)
        return;

    // Scan child objects
    for (const auto& sat : satellites)
    {
        const PlanetarySystem* satelliteSystem = sat->getSatellites();
        if (satelliteSystem != nullptr)
            satelliteSystem->getCompletion(completion, _name);
    }
}

BodyLocations::~BodyLocations() = default;

RingSystem*
BodyFeaturesManager::getRings(const Body* body) const
{
    if (!util::is_set(body->features, BodyFeatures::Rings))
        return nullptr;

    auto it = rings.find(body);
    assert(it != rings.end());
    return it->second.get();
}

void
BodyFeaturesManager::setRings(Body* body, std::unique_ptr<RingSystem>&& ringSystem)
{
    if (ringSystem == nullptr)
    {
        body->features &= ~BodyFeatures::Rings;
        rings.erase(body);
    }
    else
    {
        body->features |= BodyFeatures::Rings;
        rings[body] = std::move(ringSystem);
    }

    body->recomputeCullingRadius();
}

void
BodyFeaturesManager::scaleRings(Body* body, float scaleFactor)
{
    if (!util::is_set(body->features, BodyFeatures::Rings))
        return;

    auto it = rings.find(body);
    assert(it != rings.end());

    it->second->innerRadius *= scaleFactor;
    it->second->outerRadius *= scaleFactor;
    body->recomputeCullingRadius();
}

Atmosphere*
BodyFeaturesManager::getAtmosphere(const Body* body) const
{
    if (!util::is_set(body->features, BodyFeatures::Atmosphere))
        return nullptr;

    auto it = atmospheres.find(body);
    return it == atmospheres.end() ? nullptr : it->second.get();
}

void
BodyFeaturesManager::setAtmosphere(Body* body, std::unique_ptr<Atmosphere>&& atmosphere)
{
    if (atmosphere == nullptr)
    {
        body->features &= ~BodyFeatures::Atmosphere;
        atmospheres.erase(body);
    }
    else
    {
        body->features |= BodyFeatures::Atmosphere;
        atmospheres[body] = std::move(atmosphere);
    }

    body->recomputeCullingRadius();
}

Surface*
BodyFeaturesManager::getAlternateSurface(const Body* body, std::string_view name) const
{
    if (!util::is_set(body->features, BodyFeatures::AlternateSurfaces))
        return nullptr;

    auto alternateSurfacesIt = alternateSurfaces.find(body);
    assert(alternateSurfacesIt != alternateSurfaces.end());

    auto altSurfaces = alternateSurfacesIt->second.get();
    auto it = altSurfaces->find(name);
    return it == altSurfaces->end() ? nullptr : it->second.get();
}

void
BodyFeaturesManager::addAlternateSurface(Body* body, std::string_view name, std::unique_ptr<Surface>&& altSurface)
{
    if (altSurface == nullptr)
    {
        auto alternateSurfacesIt = alternateSurfaces.find(body);
        if (alternateSurfacesIt == alternateSurfaces.end())
            return;

        auto& altSurfaces = *alternateSurfacesIt->second;
        auto it = altSurfaces.find(name);
        if (it == altSurfaces.end())
            return;

        altSurfaces.erase(it);
        if (altSurfaces.empty())
        {
            alternateSurfaces.erase(alternateSurfacesIt);
            body->features &= ~BodyFeatures::AlternateSurfaces;
        }
    }
    else
    {
        auto [alternateSurfacesIt, createdNew] = alternateSurfaces.try_emplace(body);
        if (createdNew)
            alternateSurfacesIt->second = std::make_unique<AltSurfaceTable>();

        // C++26 provides additional overloads that allow transparent key updates
        // which would allow a small optimization in the case of replacing an
        // existing alternate surface to avoid constructing the redundant string.
        (*alternateSurfacesIt->second)[std::string(name)] = std::move(altSurface);
        body->features |= BodyFeatures::AlternateSurfaces;
    }
}

/*! Add a new reference mark.
 */
void
BodyFeaturesManager::addReferenceMark(Body* body, std::unique_ptr<ReferenceMark>&& refMark)
{
    assert(refMark != nullptr);
    referenceMarks.emplace(body, std::move(refMark));
    body->features |= BodyFeatures::ReferenceMarks;
    body->recomputeCullingRadius();
}

/*! Remove the first reference mark with the specified tag.
 */
bool
BodyFeaturesManager::removeReferenceMark(Body* body, std::string_view tag)
{
    if (!util::is_set(body->features, BodyFeatures::ReferenceMarks))
        return false;

    auto [start, end] = referenceMarks.equal_range(body);
    assert(start != end);

    auto next = start;
    ++next;
    bool isLastElement = next == end;

    auto it = std::find_if(start, end, [&tag](const auto& rm) { return rm.second->getTag() == tag; });
    if (it == end)
        return false;

    referenceMarks.erase(it);
    if (isLastElement)
        body->features &= ~BodyFeatures::ReferenceMarks;

    body->recomputeCullingRadius();
    return true;
}

/*! Find the first reference mark with the specified tag. If the body has
 *  no reference marks with the specified tag, this method will return
 *  nullptr.
 */
const ReferenceMark*
BodyFeaturesManager::findReferenceMark(const Body* body, std::string_view tag) const
{
    if (!util::is_set(body->features, BodyFeatures::ReferenceMarks))
        return nullptr;

    auto [start, end] = referenceMarks.equal_range(body);
    auto it = std::find_if(start, end, [&tag](const auto& rm) { return rm.second->getTag() == tag; });
    return it == end ? nullptr : it->second.get();
}

void
BodyFeaturesManager::addLocation(Body* body, std::unique_ptr<Location>&& loc)
{
    assert(loc != nullptr);
    auto& bodyLocations = locations[body];
    loc->setParentBody(body);
    bodyLocations.locations.push_back(std::move(loc));
    body->features |= BodyFeatures::Locations;
}

Location*
BodyFeaturesManager::findLocation(const Body* body, std::string_view name, bool i18n) const
{
    if (!util::is_set(body->features, BodyFeatures::Locations))
        return nullptr;

    auto bodyLocationsIt = locations.find(body);
    assert(bodyLocationsIt != locations.end());

    auto& bodyLocations = bodyLocationsIt->second.locations;

    auto iter = i18n
        ? std::find_if(bodyLocations.begin(), bodyLocations.end(),
                       [&name](const auto& loc) { return UTF8StringCompare(name, loc->getName(false)) == 0 ||
                                                         UTF8StringCompare(name, loc->getName(true)) == 0; })
        : std::find_if(bodyLocations.begin(), bodyLocations.end(),
                       [&name](const auto& loc) { return UTF8StringCompare(name, loc->getName(false)) == 0; });

    return iter == bodyLocations.end() ? nullptr : iter->get();
}

bool
BodyFeaturesManager::hasLocations(const Body* body) const
{
    return util::is_set(body->features, BodyFeatures::Locations);
}

// Compute the positions of locations on an irregular object using ray-mesh
// intersections.  This is not automatically done when a location is added
// because it would force the loading of all meshes for objects with
// defined locations; on-demand (i.e. when the object becomes visible to
// a user) loading of meshes is preferred.
void
BodyFeaturesManager::computeLocations(const Body* body)
{
    if (!util::is_set(body->features, BodyFeatures::Locations))
        return;

    auto it = locations.find(body);
    assert(it != locations.end());

    auto& bodyLocations = it->second;
    if (bodyLocations.locationsComputed)
        return;

    bodyLocations.locationsComputed = true;

    // No work to do if there's no mesh, or if the mesh cannot be loaded
    auto geometry = body->getGeometry();
    if (geometry == InvalidResource)
        return;

    const Geometry* g = engine::GetGeometryManager()->find(geometry);
    if (g == nullptr)
        return;

    // TODO: Implement separate radius and bounding radius so that this hack is
    // not necessary.
    double boundingRadius = 2.0;
    auto radius = body->getRadius();
    for (const auto& location : bodyLocations.locations)
    {
        Location* loc = location.get();
        Eigen::Vector3f v = loc->getPosition();
        float alt = v.norm() - radius;
        if (alt > 0.1f * radius) // assume we don't have locations with height > 0.1*radius
            continue;
        if (alt != -radius)
            v.normalize();
        v *= (float) boundingRadius;

        Eigen::ParametrizedLine<double, 3> ray(v.cast<double>(), -v.cast<double>());
        double t = 0.0;
        if (g->pick(ray, t))
        {
            v *= (float) ((1.0 - t) * radius + alt);
            loc->setPosition(v);
        }
    }
}

bool
BodyFeaturesManager::getOrbitColor(const Body* body, Color& color) const
{
    if (!util::is_set(body->features, BodyFeatures::OrbitColor))
        return false;

    auto it = orbitColors.find(body);
    assert(it != orbitColors.end());
    color = it->second;
    return true;
}

void
BodyFeaturesManager::setOrbitColor(const Body* body, const Color& color)
{
    orbitColors[body] = color;
}

bool
BodyFeaturesManager::getOrbitColorOverridden(const Body* body) const
{
    return util::is_set(body->features, BodyFeatures::OrbitColor);
}

void
BodyFeaturesManager::setOrbitColorOverridden(Body* body, bool overridden)
{
    // don't allow setting this value unless there is an override color
    if (overridden && orbitColors.find(body) == orbitColors.end())
        overridden = false;
    util::set_or_unset(body->features, BodyFeatures::OrbitColor, overridden);
}

void
BodyFeaturesManager::unsetOrbitColor(Body* body)
{
    orbitColors.erase(body);
    body->features &= ~BodyFeatures::OrbitColor;
}

Color
BodyFeaturesManager::getCometTailColor(const Body* body) const
{
    if (!util::is_set(body->features, BodyFeatures::CometTailColor))
        return defaultCometTailColor;

    auto it = cometTailColors.find(body);
    assert(it != cometTailColors.end());
    return it->second;
}

void
BodyFeaturesManager::setCometTailColor(Body* body, const Color& color)
{
    cometTailColors[body] = color;
    body->features |= BodyFeatures::CometTailColor;
}

void
BodyFeaturesManager::unsetCometTailColor(Body* body)
{
    cometTailColors.erase(body);
    body->features &= ~BodyFeatures::CometTailColor;
}

void
BodyFeaturesManager::removeFeatures(Body* body)
{
    atmospheres.erase(body);
    rings.erase(body);
    alternateSurfaces.erase(body);
    referenceMarks.erase(body);
    locations.erase(body);
    orbitColors.erase(body);
    cometTailColors.erase(body);
    body->features = BodyFeatures::None;
    // could recompute the culling radius here - not currently necessary
    // as we only use this when we're deleting the Body
}

BodyFeaturesManager*
GetBodyFeaturesManager()
{
    static BodyFeaturesManager* const manager = std::make_unique<BodyFeaturesManager>().release(); //NOSONAR
    return manager;
}
