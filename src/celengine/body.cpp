// body.cpp
//
// Copyright (C) 2001-2006 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstdlib>
#include <cassert>
#include <algorithm>
#include <utility>
#include <celcompat/numbers.h>
#include <celmath/mathlib.h>
#include <celutil/gettext.h>
#include <celutil/utf8.h>
#include "geometry.h"
#include "meshmanager.h"
#include "body.h"
#include "atmosphere.h"
#include "frame.h"
#include "timeline.h"
#include "timelinephase.h"
#include "frametree.h"
#include "referencemark.h"
#include "selection.h"

using namespace Eigen;
using namespace std;

namespace astro = celestia::astro;
namespace math = celestia::math;

Body::Body(PlanetarySystem* _system, const std::string& _name) :
    system(_system),
    orbitVisibility(UseClassVisibility)
{
    setName(_name);
    recomputeCullingRadius();
}


Body::~Body() = default;


/*! Reset body attributes to their default values. The object hierarchy is left untouched,
 *  i.e. child objects are not removed. Alternate surfaces and locations are not removed
 *  either.
 */
void Body::setDefaultProperties()
{
    radius = 1.0f;
    semiAxes = Vector3f::Ones();
    mass = 0.0f;
    density = 0.0f;
    bondAlbedo = 0.5f;
    geomAlbedo = 0.5f;
    reflectivity = 0.5f;
    temperature = 0.0f;
    tempDiscrepancy = 0.0f;
    geometryOrientation = Quaternionf::Identity();
    geometry = InvalidResource;
    surface = Surface(Color::White);
    atmosphere.reset();
    rings.reset();
    classification = Unknown;
    visible = true;
    clickable = true;
    visibleAsPoint = true;
    overrideOrbitColor = false;
    orbitVisibility = UseClassVisibility;
    recomputeCullingRadius();
}


/*! Return the list of all names (non-localized) by which this
 *  body is known.
 */
const vector<string>& Body::getNames() const
{
    return names;
}


/*! Return the primary name for the body; if i18n, return the
 *  localized name of the body.
 */
string Body::getName(bool i18n) const
{
    if (i18n && hasLocalizedName())
        return localizedName;
    return names[0];
}


/*! Get the localized name for the body. If no localized name
 *  has been set, the primary name is returned.
 */
string Body::getLocalizedName() const
{
    return hasLocalizedName() ? localizedName : names[0];
}


bool Body::hasLocalizedName() const
{
    return primaryNameLocalized;
}


/*! Set the primary name of the body. The localized name is updated
 *  automatically as well.
 *  Note: setName() is private, and only called from the Body constructor.
 *  It shouldn't be called elsewhere.
 */
void Body::setName(const string& name)
{
    names[0] = name;
    string localizedName = D_(name.c_str());
    if (name == localizedName)
    {
        // No localized name; set the localized name index to zero to
        // indicate this.
        primaryNameLocalized = false;
    }
    else
    {
        this->localizedName = localizedName;
        primaryNameLocalized = true;
    }
}


/*! Add a new name for this body. Aliases are non localized.
 */
void Body::addAlias(const string& alias)
{
    // Don't add an alias if it matches the primary name
    if (alias != names[0])
    {
        names.push_back(alias);
        system->addAlias(this, alias);
    }
}


PlanetarySystem* Body::getSystem() const
{
    return system;
}


FrameTree* Body::getFrameTree() const
{
    return frameTree.get();
}


FrameTree* Body::getOrCreateFrameTree()
{
    if (!frameTree)
        frameTree = std::make_unique<FrameTree>(this);
    return frameTree.get();
}


const Timeline* Body::getTimeline() const
{
    return timeline.get();
}


void Body::setTimeline(std::unique_ptr<Timeline>&& newTimeline)
{
    timeline = std::move(newTimeline);
    markChanged();
}


void Body::markChanged()
{
    if (timeline)
        timeline->markChanged();
}


void Body::markUpdated()
{
    if (frameTree)
        frameTree->markUpdated();
}


const ReferenceFrame::SharedConstPtr& Body::getOrbitFrame(double tdb) const
{
    return timeline->findPhase(tdb)->orbitFrame();
}


const celestia::ephem::Orbit* Body::getOrbit(double tdb) const
{
    return timeline->findPhase(tdb)->orbit();
}


const ReferenceFrame::SharedConstPtr& Body::getBodyFrame(double tdb) const
{
    return timeline->findPhase(tdb)->bodyFrame();
}


const celestia::ephem::RotationModel*
Body::getRotationModel(double tdb) const
{
    return timeline->findPhase(tdb)->rotationModel();
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
float Body::getBoundingRadius() const
{
    if (geometry == InvalidResource)
        return radius;

    return radius * celestia::numbers::sqrt3_v<float>;
}


/*! Return the radius of sphere large enough to contain any geometry
 *  associated with this object: the primary geometry, comet tail,
 *  rings, atmosphere shell, cloud layers, or reference marks.
 */
float Body::getCullingRadius() const
{
    return cullingRadius;
}


float Body::getMass() const
{
    return mass;
}


void Body::setMass(float _mass)
{
    mass = _mass;
}


float Body::getDensity() const
{
    if (density > 0.0f)
        return density;

    if (radius == 0.0f || !isEllipsoid())
        return 0.0f;

    // @mass unit is mass of Earth
    // @astro::EarthMass unit is kg
    // @radius unit km
    // so we divide density by 1e9 to have kg/m^3
    float volume = 4.0f / 3.0f * celestia::numbers::pi_v<float> * semiAxes.prod();
    return volume == 0.0f ? 0.0f : mass * static_cast<float>(astro::EarthMass / 1e9) / volume;
}


void Body::setDensity(float _density)
{
    density = _density;
}


float Body::getGeomAlbedo() const
{
    return geomAlbedo;
}


void Body::setGeomAlbedo(float _geomAlbedo)
{
    geomAlbedo = _geomAlbedo;
}


float Body::getBondAlbedo() const
{
    return bondAlbedo;
}


void Body::setBondAlbedo(float _bondAlbedo)
{
    bondAlbedo = _bondAlbedo;
}


float Body::getReflectivity() const
{
    return reflectivity;
}


void Body::setReflectivity(float _reflectivity)
{
    reflectivity = _reflectivity;
}


float Body::getTemperature(double time) const
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
        float distFromSun = (float)getAstrocentricPosition(time).norm();
        temp = sun->getTemperature() *
               pow(1.0f - getBondAlbedo(), 0.25f) *
               sqrt(sun->getRadius() / (2.0f * distFromSun));
    }
    else // the sun is a barycenter
    {
        const auto* orbitingStars = sun->getOrbitingStars();
        if (orbitingStars == nullptr || orbitingStars->empty())
            return 0.0f;

        const UniversalCoord bodyPos = getPosition(time);
        float flux = 0.0f;
        for (const auto *s : *orbitingStars)
        {
            float distFromSun = (float)s->getPosition(time).distanceFromKm(bodyPos);
            float lum = math::square(s->getRadius()) * pow(s->getTemperature(), 4.0f);
            flux += lum / math::square(distFromSun);
        }
        temp = std::pow((1.0f - getBondAlbedo()) * flux, 0.25f) * (celestia::numbers::sqrt2_v<float> * 0.5f);
    }
    return getTempDiscrepancy() + temp;
}


void Body::setTemperature(float _temperature)
{
    temperature = _temperature;
}


float Body::getTempDiscrepancy() const
{
    return tempDiscrepancy;
}


void Body::setTempDiscrepancy(float _tempDiscrepancy)
{
    tempDiscrepancy = _tempDiscrepancy;
}


Quaternionf Body::getGeometryOrientation() const
{
    return geometryOrientation;
}


void Body::setGeometryOrientation(const Quaternionf& orientation)
{
    geometryOrientation = orientation;
}


/*! Set the semiaxes of a body.
 */
void Body::setSemiAxes(const Vector3f& _semiAxes)
{
    semiAxes = _semiAxes;

    // Radius will always be the largest of the three semi axes
    radius = semiAxes.maxCoeff();
    recomputeCullingRadius();
}


/*! Retrieve the body's semiaxes
 */
Vector3f Body::getSemiAxes() const
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
float Body::getRadius() const
{
    return radius;
}


/*! Return true if the body is a perfect sphere.
*/
bool Body::isSphere() const
{
    return (geometry == InvalidResource) &&
           (semiAxes.x() == semiAxes.y()) &&
           (semiAxes.x() == semiAxes.z());
}


/*! Return true if the body is ellipsoidal, with geometry determined
 *  completely by its semiaxes rather than a triangle based model.
 */
bool Body::isEllipsoid() const
{
    return geometry == InvalidResource;
}


const Surface& Body::getSurface() const
{
    return surface;
}


Surface& Body::getSurface()
{
    return surface;
}


void Body::setSurface(const Surface& surf)
{
    surface = surf;
}


void Body::setGeometry(ResourceHandle _geometry)
{
    geometry = _geometry;
}


/*! Set the scale factor for geometry; this is only used with unnormalized meshes.
 *  When a mesh is normalized, the effective scale factor is the radius.
 */
void Body::setGeometryScale(float scale)
{
    geometryScale = scale;
}


PlanetarySystem* Body::getSatellites() const
{
    return satellites.get();
}

PlanetarySystem* Body::getOrCreateSatellites()
{
    if (satellites == nullptr)
        satellites = std::make_unique<PlanetarySystem>(this);
    return satellites.get();
}


RingSystem* Body::getRings() const
{
    return rings.get();
}

void Body::setRings(std::unique_ptr<RingSystem>&& _rings)
{
    rings = std::move(_rings);
    recomputeCullingRadius();
}

void Body::scaleRings(float scaleFactor)
{
    if (rings == nullptr)
        return;

    rings->innerRadius *= scaleFactor;
    rings->outerRadius *= scaleFactor;
    recomputeCullingRadius();
}


const Atmosphere* Body::getAtmosphere() const
{
    return atmosphere.get();
}

Atmosphere* Body::getAtmosphere()
{
    return atmosphere.get();
}

void Body::setAtmosphere(std::unique_ptr<Atmosphere>&& _atmosphere)
{
    atmosphere = std::move(_atmosphere);
    recomputeCullingRadius();
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
UniversalCoord Body::getPosition(double tdb) const
{
    Vector3d position = Vector3d::Zero();

    const TimelinePhase* phase = timeline->findPhase(tdb).get();
    Vector3d p = phase->orbit()->positionAtTime(tdb);
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
Quaterniond Body::getOrientation(double tdb) const
{
    const TimelinePhase* phase = timeline->findPhase(tdb).get();
    return phase->rotationModel()->orientationAtTime(tdb) * phase->bodyFrame()->getOrientation(tdb);
}


/*! Get the velocity of the body in the universal frame.
 */
Vector3d Body::getVelocity(double tdb) const
{
    const TimelinePhase* phase = timeline->findPhase(tdb).get();

    const ReferenceFrame* orbitFrame = phase->orbitFrame().get();

    Vector3d v = phase->orbit()->velocityAtTime(tdb);
    v = orbitFrame->getOrientation(tdb).conjugate() * v + orbitFrame->getCenter().getVelocity(tdb);

    if (!orbitFrame->isInertial())
    {
        Vector3d r = Selection(const_cast<Body*>(this)).getPosition(tdb).offsetFromKm(orbitFrame->getCenter().getPosition(tdb));
        v += orbitFrame->getAngularVelocity(tdb).cross(r);
    }

    return v;
}


/*! Get the angular velocity of the body in the universal frame.
 */
Vector3d Body::getAngularVelocity(double tdb) const
{
    const TimelinePhase* phase = timeline->findPhase(tdb).get();

    Vector3d v = phase->rotationModel()->angularVelocityAtTime(tdb);

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
Matrix4d Body::getLocalToAstrocentric(double tdb) const
{
    const TimelinePhase* phase = timeline->findPhase(tdb).get();
    Vector3d p = phase->orbitFrame()->convertToAstrocentric(phase->orbit()->positionAtTime(tdb), tdb);
    return Eigen::Transform<double, 3, Affine>(Translation3d(p)).matrix();
}


/*! Get the position of the center of the body in astrocentric ecliptic coordinates.
 */
Vector3d Body::getAstrocentricPosition(double tdb) const
{
    // TODO: Switch the iterative method used in getPosition
    const TimelinePhase* phase = timeline->findPhase(tdb).get();
    return phase->orbitFrame()->convertToAstrocentric(phase->orbit()->positionAtTime(tdb), tdb);
}


/*! Get a rotation that converts from the ecliptic frame to the body frame.
 */
Quaterniond Body::getEclipticToFrame(double tdb) const
{
    const TimelinePhase* phase = timeline->findPhase(tdb).get();
    return phase->bodyFrame()->getOrientation(tdb);
}


/*! Get a rotation that converts from the ecliptic frame to the body's
 *  mean equatorial frame.
 */
Quaterniond Body::getEclipticToEquatorial(double tdb) const
{
    const TimelinePhase* phase = timeline->findPhase(tdb).get();
    return phase->rotationModel()->equatorOrientationAtTime(tdb) * phase->bodyFrame()->getOrientation(tdb);
}


/*! Get a rotation that converts from the ecliptic frame to this
 *  objects's body fixed frame.
 */
Quaterniond Body::getEclipticToBodyFixed(double tdb) const
{
    const TimelinePhase* phase = timeline->findPhase(tdb).get();
    return phase->rotationModel()->orientationAtTime(tdb) * phase->bodyFrame()->getOrientation(tdb);
}


// The body-fixed coordinate system has an origin at the center of the
// body, y-axis parallel to the rotation axis, x-axis through the prime
// meridian, and z-axis at a right angle the xy plane.
Quaterniond Body::getEquatorialToBodyFixed(double tdb) const
{
    const TimelinePhase* phase = timeline->findPhase(tdb).get();
    return phase->rotationModel()->spin(tdb);
}


/*! Get a transformation to convert from the object's body fixed frame
 *  to the astrocentric ecliptic frame.
 */
Matrix4d Body::getBodyFixedToAstrocentric(double tdb) const
{
    Matrix4d m = Eigen::Affine3d(getEquatorialToBodyFixed(tdb)).matrix();
    return m * getLocalToAstrocentric(tdb);
}

Vector3d Body::planetocentricToCartesian(double lon, double lat, double alt) const
{

    using celestia::numbers::pi;
    double phi = -math::degToRad(lat) + pi * 0.5;
    double theta = math::degToRad(lon) - pi;

    Vector3d pos(cos(theta) * sin(phi),
                 cos(phi),
                 -sin(theta) * sin(phi));

    return pos * (getRadius() + alt);
}


Vector3d Body::planetocentricToCartesian(const Vector3d& lonLatAlt) const
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
Vector3d Body::geodeticToCartesian(double lon, double lat, double alt) const
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


Vector3d Body::geodeticToCartesian(const Vector3d& lonLatAlt) const
{
    return geodeticToCartesian(lonLatAlt.x(), lonLatAlt.y(), lonLatAlt.z());
}


/*! Convert cartesian body-fixed coordinates to spherical planetocentric
 *  coordinates.
 */
Vector3d Body::cartesianToPlanetocentric(const Vector3d& v) const
{
    Vector3d w = v.normalized();

    double lat = celestia::numbers::pi / 2.0 - acos(w.y());
    double lon = atan2(w.z(), -w.x());

    return Vector3d(lon, lat, v.norm() - getRadius());
}


/*! Convert body-centered ecliptic coordinates to spherical planetocentric
 *  coordinates.
 */
Vector3d Body::eclipticToPlanetocentric(const Vector3d& ecl, double tdb) const
{
    Vector3d bf = getEclipticToBodyFixed(tdb) * ecl;
    return cartesianToPlanetocentric(bf);
}


bool Body::extant(double t) const
{
    return timeline->includes(t);
}


void Body::getLifespan(double& begin, double& end) const
{
    begin = timeline->startTime();
    end = timeline->endTime();
}


float Body::getLuminosity(const Star& sun,
                          float distanceFromSun) const
{
    return getLuminosity(sun.getLuminosity(), distanceFromSun);
}


float Body::getLuminosity(float sunLuminosity,
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
    return (float) (reflectedEnergy / astro::SOLAR_POWER);
}


/*! Get the apparent magnitude of the body, neglecting the phase (as if
 *  the body was at opposition.
 */
float Body::getApparentMagnitude(const Star& sun,
                                 float distanceFromSun,
                                 float distanceFromViewer) const
{
    return astro::lumToAppMag(getLuminosity(sun, distanceFromSun),
                              astro::kilometersToLightYears(distanceFromViewer));
}


/*! Get the apparent magnitude of the body, neglecting the phase (as if
 *  the body was at opposition.
 */
float Body::getApparentMagnitude(float sunLuminosity,
                                 float distanceFromSun,
                                 float distanceFromViewer) const
{
    return astro::lumToAppMag(getLuminosity(sunLuminosity, distanceFromSun),
                              astro::kilometersToLightYears(distanceFromViewer));
}

/*! Get the apparent magnitude of the body, corrected for its phase.
 */
float Body::getApparentMagnitude(const Star& sun,
                                 const Vector3d& sunPosition,
                                 const Vector3d& viewerPosition) const
{
    return getApparentMagnitude(sun.getLuminosity(),
                                sunPosition,
                                viewerPosition);
}


/*! Get the apparent magnitude of the body, corrected for its phase.
 */
float Body::getApparentMagnitude(float sunLuminosity,
                                 const Vector3d& sunPosition,
                                 const Vector3d& viewerPosition) const
{
    double distanceToViewer = viewerPosition.norm();
    double distanceToSun = sunPosition.norm();
    float illuminatedFraction = (float) (1.0 + (viewerPosition / distanceToViewer).dot(sunPosition / distanceToSun)) / 2.0f;

    return astro::lumToAppMag(getLuminosity(sunLuminosity, (float) distanceToSun) * illuminatedFraction, (float) astro::kilometersToLightYears(distanceToViewer));
}


int Body::getClassification() const
{
    return classification;
}

void Body::setClassification(int _classification)
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
int Body::getOrbitClassification() const
{
    if (classification != Invisible || !frameTree)
        return classification;

    int orbitClass = frameTree->childClassMask();
    if ((orbitClass & Planet) != 0)
        return Planet;
    if ((orbitClass & DwarfPlanet) != 0)
        return DwarfPlanet;
    if ((orbitClass & Asteroid) != 0)
        return Asteroid;
    if ((orbitClass & Moon) != 0)
        return Moon;
    if ((orbitClass & MinorMoon) != 0)
        return MinorMoon;
    if ((orbitClass & Spacecraft) != 0)
        return Spacecraft;
    return Invisible;
}


const string& Body::getInfoURL() const
{
    return infoURL;
}

void Body::setInfoURL(const string& _infoURL)
{
    infoURL = _infoURL;
}


Surface* Body::getAlternateSurface(const string& name) const
{
    if (!altSurfaces)
        return nullptr;

    auto iter = altSurfaces->find(name);
    if (iter == altSurfaces->end())
        return nullptr;

    return iter->second.get();
}


void Body::addAlternateSurface(const string& name, std::unique_ptr<Surface>&& altSurface)
{
    if (!altSurfaces)
        altSurfaces = std::make_unique<AltSurfaceTable>();

    (*altSurfaces)[name] = std::move(altSurface);
}


void Body::addLocation(std::unique_ptr<Location>&& loc)
{
    if (!loc)
        return;

    if (!locations)
        locations = std::make_unique<std::vector<std::unique_ptr<Location>>>();
    loc->setParentBody(this);
    locations->push_back(std::move(loc));
}


Location* Body::findLocation(std::string_view name, bool i18n) const
{
    if (!locations)
        return nullptr;

    auto iter = i18n
        ? std::find_if(locations->cbegin(), locations->cend(),
                       [&name](const auto& loc) { return UTF8StringCompare(name, loc->getName(false)) == 0 ||
                                                         UTF8StringCompare(name, loc->getName(true)) == 0; })
        : std::find_if(locations->cbegin(), locations->cend(),
                       [&name](const auto& loc) { return UTF8StringCompare(name, loc->getName(false)) == 0; });

    return iter == locations->cend() ? nullptr : iter->get();
}


// Compute the positions of locations on an irregular object using ray-mesh
// intersections.  This is not automatically done when a location is added
// because it would force the loading of all meshes for objects with
// defined locations; on-demand (i.e. when the object becomes visible to
// a user) loading of meshes is preferred.
void Body::computeLocations()
{
    if (locationsComputed)
        return;

    locationsComputed = true;

    if (locations == nullptr)
        return;

    // No work to do if there's no mesh, or if the mesh cannot be loaded
    if (geometry == InvalidResource)
        return;
    Geometry* g = GetGeometryManager()->find(geometry);
    if (g == nullptr)
        return;

    // TODO: Implement separate radius and bounding radius so that this hack is
    // not necessary.
    double boundingRadius = 2.0;

    for (const auto& location : *locations)
    {
        Location* loc = location.get();
        Vector3f v = loc->getPosition();
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


/*! Add a new reference mark.
 */
void
Body::addReferenceMark(std::unique_ptr<ReferenceMark>&& refMark)
{
    if (referenceMarks == nullptr)
        referenceMarks = std::make_unique<std::list<std::unique_ptr<ReferenceMark>>>();
    referenceMarks->push_back(std::move(refMark));
    recomputeCullingRadius();
}


/*! Remove the first reference mark with the specified tag.
 */
void
Body::removeReferenceMark(const string& tag)
{
    if (referenceMarks == nullptr)
        return;

    auto iter = std::find_if(referenceMarks->begin(),
                             referenceMarks->end(),
                             [&tag](const auto& rm) { return rm->getTag() == tag; });
    if (iter == referenceMarks->end())
        return;

    referenceMarks->erase(iter);
    recomputeCullingRadius();
}


/*! Find the first reference mark with the specified tag. If the body has
 *  no reference marks with the specified tag, this method will return
 *  nullptr.
 */
const ReferenceMark*
Body::findReferenceMark(const string& tag) const
{
    if (referenceMarks == nullptr)
        return nullptr;

    auto iter = std::find_if(referenceMarks->begin(),
                             referenceMarks->end(),
                             [&tag](const auto& rm) { return rm->getTag() == tag; });
    return iter == referenceMarks->end()
        ? nullptr
        : iter->get();
}


/*! Sets whether or not the object is visible.
 */
void Body::setVisible(bool _visible)
{
    visible = _visible;
}


/*! Sets whether or not the object can be selected by clicking on
 *  it. If set to false, the object is completely ignored when the
 *  user clicks it, making it possible to select background objects.
 */
void Body::setClickable(bool _clickable)
{
    clickable = _clickable;
}


/*! Set whether or not the object is visible as a starlike point
 *  when it occupies less than a pixel onscreen. This is appropriate
 *  for planets and moons, but generally not desireable for buildings
 *  or spacecraft components.
 */
void Body::setVisibleAsPoint(bool _visibleAsPoint)
{
    visibleAsPoint = _visibleAsPoint;
}


/*! The orbitColorOverride flag is set to true if an alternate orbit
 *  color should be used (specified via setOrbitColor) instead of the
 *  default class orbit color.
 */
void Body::setOrbitColorOverridden(bool _override)
{
    overrideOrbitColor = _override;
}


/*! Set the visibility policy for the orbit of this object:
 *  - NeverVisible: Never show the orbit of this object.
 *  - UseClassVisibility: (Default) Show the orbit of this object
 *    its class is enabled in the orbit mask.
 *  - AlwaysVisible: Always show the orbit of this object whenever
 *    orbit paths are enabled.
 */
void Body::setOrbitVisibility(VisibilityPolicy _orbitVisibility)
{
    orbitVisibility = _orbitVisibility;
}


/*! Set the color used when rendering the orbit. This is only used
 *  when the orbitColorOverride flag is set to true; otherwise, the
 *  standard orbit color for all objects of the class is used.
 */
void Body::setOrbitColor(const Color& c)
{
    orbitColor = c;
}


/*! Set the comet tail color
 *
 */
void Body::setCometTailColor(const Color& c)
{
    cometTailColor = c;
}


/*! Set whether or not the object should be considered when calculating
 *  secondary illumination (e.g. planetshine.)
 */
void Body::setSecondaryIlluminator(bool enable)
{
    if (enable != secondaryIlluminator)
    {
        markChanged();
        secondaryIlluminator = enable;
    }
}


void Body::recomputeCullingRadius()
{
    float r = getBoundingRadius();

    if (atmosphere)
        r += max(atmosphere->height, atmosphere->cloudHeight);

    if (rings)
        r = max(r, rings->outerRadius);

    if (referenceMarks)
    {
        for (const auto& rm : *referenceMarks)
        {
            r = max(r, rm->boundingSphereRadius());
        }
    }

    if (classification == Body::Comet)
        r = max(r, astro::AUtoKilometers(1.0f));

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


/*! Add a new alias for an object. If an object with the specified
 *  alias already exists in the planetary system, the old entry will
 *  be replaced.
 */
void PlanetarySystem::addAlias(Body* body, const string& alias)
{
    assert(body->getSystem() == this);

    objectIndex.try_emplace(alias, body);
}


Body* PlanetarySystem::addBody(const std::string& name)
{
    auto body = std::make_unique<Body>(this, name);
    addBodyToNameIndex(body.get());
    return satellites.emplace_back(std::move(body)).get();
}


void PlanetarySystem::removeBody(const Body* body)
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
void PlanetarySystem::addBodyToNameIndex(Body* body)
{
    const std::vector<std::string>& names = body->getNames();
    for (const auto& name : names)
    {
        objectIndex.try_emplace(name, body);
    }
}


void PlanetarySystem::removeBodyFromNameIndex(const Body* body)
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
Body* PlanetarySystem::find(std::string_view _name, bool deepSearch, bool i18n) const
{
    auto firstMatch = objectIndex.find(_name);
    if (firstMatch != objectIndex.end())
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


void PlanetarySystem::getCompletion(std::vector<std::string>& completion,
                                    std::string_view _name,
                                    bool deepSearch) const
{
    // Search through all names in this planetary system.
    for (const auto& index : objectIndex)
    {
        const string& alias = index.first;

        if (UTF8StartsWith(alias, _name))
        {
            completion.push_back(alias);
        }
        else
        {
            std::string lname = D_(alias.c_str());
            if (lname != alias && UTF8StartsWith(lname, _name))
                completion.push_back(lname);
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
