// body.cpp
//
// Copyright (C) 2001-2006 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstdlib>
// Missing from g++ . . . why???
// #include <limits>
#include <cassert>
#include <celmath/mathlib.h>
#include <celutil/util.h>
#include <celutil/utf8.h>
#include "mesh.h"
#include "meshmanager.h"
#include "body.h"
#include "frame.h"

using namespace std;

Body::Body(PlanetarySystem* _system) :
    orbit(NULL),
    orbitBarycenter(_system ? _system->getPrimaryBody() : NULL),
    orbitFrame(NULL),
    bodyFrame(NULL),
    rotationModel(NULL),
    radius(1.0f),
    semiAxes(1.0f, 1.0f, 1.0f),
    mass(0.0f),
    albedo(0.5),
    orientation(1.0f),
    // Ugh.  Numeric_limits class is missing from g++
    // protos(-numeric_limits<double>::infinity()),
    // eschatos(numeric_limits<double>::infinity()),
    // Do it the ugly way instead:
    protos(-1.0e+50),
    eschatos(1.0e+50),
    model(InvalidResource),
    surface(Color(1.0f, 1.0f, 1.0f)),
    atmosphere(NULL),
    rings(NULL),
    satellites(NULL),
    classification(Unknown),
    altSurfaces(NULL),
    locations(NULL),
    locationsComputed(false),
    referenceMarks(0),
    visible(1),
    clickable(1),
    visibleAsPoint(1),
    overrideOrbitColor(0),
    orbitVisibility(UseClassVisibility),
    frameRefStar(NULL)
{
    system = _system;
}


Body::~Body()
{
    // clean up orbit, atmosphere, etc.
    if (system != NULL)
        system->removeBody(this);

    if (orbitFrame != NULL)
        orbitFrame->release();
    if (bodyFrame != NULL)
        bodyFrame->release();
}


PlanetarySystem* Body::getSystem() const
{
    return system;
}


string Body::getName(bool i18n) const
{
    if (!i18n || i18nName == "") return name;
    return i18nName;
}


void Body::setName(const string _name)
{
    name = _name;
    i18nName = _(_name.c_str());
    if (name == i18nName) i18nName = "";
}


Orbit* Body::getOrbit() const
{
    return orbit;
}


void Body::setOrbit(Orbit* _orbit)
{
    delete orbit;
    orbit = _orbit;
}


// TODO: orbitBarycenter is superceded by frames and should be removed.
const Body* Body::getOrbitBarycenter() const
{
    return orbitBarycenter;
}


void Body::setOrbitBarycenter(const Body* barycenterBody)
{
    assert(barycenterBody == NULL || barycenterBody->getSystem()->getStar() == getSystem()->getStar());
    if (barycenterBody != NULL)
    {
    }
    orbitBarycenter = barycenterBody;
}

const ReferenceFrame* Body::getOrbitFrame() const
{
    return orbitFrame;
}


void
Body::setOrbitFrame(const ReferenceFrame* f)
{
    // Update reference counts
    if (f != NULL)
        f->addRef();
    if (orbitFrame != NULL)
        orbitFrame->release();

    orbitFrame = f;

    // Temporary hack: keep track of the star this frame is ultimately
    // referenced to if it is different from the star at the root of
    // the body's name hierarchy.
    Star* frameRoot = getFrameReferenceStar();
    Star* refStar = getReferenceStar();
    if (frameRoot != refStar)
        frameRefStar = frameRoot;
}


const ReferenceFrame* Body::getBodyFrame() const
{
    return bodyFrame;
}


void
Body::setBodyFrame(const ReferenceFrame* f)
{
    // Update reference counts
    if (f != NULL)
        f->addRef();
    if (bodyFrame != NULL)
        bodyFrame->release();

    bodyFrame = f;
}


// For an irregular object, the radius is defined to be the largest semi-axis
// of the axis-aligned bounding box.  The radius of the smallest sphere
// containing the object is potentially larger by a factor of sqrt(3)
float Body::getBoundingRadius() const
{
    if (model == InvalidResource)
        return radius;
    else
        return radius * 1.7320508f; // sqrt(3)
}


float Body::getMass() const
{
    return mass;
}


void Body::setMass(float _mass)
{
    mass = _mass;
}


float Body::getAlbedo() const
{
    return albedo;
}


void Body::setAlbedo(float _albedo)
{
    albedo = _albedo;
}


Quatf Body::getOrientation() const
{
    return orientation;
}


void Body::setOrientation(const Quatf& q)
{
    orientation = q;
}


const RotationModel* Body::getRotationModel() const
{
    return rotationModel;
}

void Body::setRotationModel(const RotationModel* rm)
{
    rotationModel = rm;
}


/*! Set the semiaxes of a body.
 */
void Body::setSemiAxes(const Vec3f& _semiAxes)
{
    semiAxes = _semiAxes;

    // Radius will always be the largest of the three semi axes
    radius = max(semiAxes.x, max(semiAxes.y, semiAxes.z));
}


/*! Retrieve the body's semiaxes
 */
Vec3f Body::getSemiAxes() const
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
    return (model == InvalidResource) &&
           (semiAxes.x == semiAxes.y) && 
           (semiAxes.x == semiAxes.z);
}


/*! Return true if the body is ellipsoidal, with geometry determined
 *  completely by its semiaxes rather than a triangle based model.
 */
bool Body::isEllipsoid() const
{
    return model == InvalidResource;
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


ResourceHandle Body::getModel() const
{
    return model;
}

void Body::setModel(ResourceHandle _model)
{
    model = _model;
}


PlanetarySystem* Body::getSatellites() const
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


const Atmosphere* Body::getAtmosphere() const
{
    return atmosphere;
}

Atmosphere* Body::getAtmosphere()
{
    return atmosphere;
}

void Body::setAtmosphere(const Atmosphere& _atmosphere)
{
    if (atmosphere == NULL)
        atmosphere = new Atmosphere();
    *atmosphere = _atmosphere;
}


// Get a matrix which converts from local to heliocentric coordinates
Mat4d Body::getLocalToHeliocentric(double tjd) const
{
    Point3d pos = orbit->positionAtTime(tjd);

    if (orbitFrame != NULL)
    {
        Point3d p = orbitFrame->convertFromAstrocentric(pos, tjd);

        // Temporary hack; won't be necessary post-1.5.0 when this function
        // is redefined to return position with respect to frame root object
        // instead of name hierarchy root.
        if (frameRefStar != NULL)
        {
            Vec3d frameOffset(0.0, 0.0, 0.0);
            Star* refStar = getReferenceStar();
            if (refStar != NULL)
            {
                frameOffset = (frameRefStar->getPosition(tjd) - refStar->getPosition(tjd)) *
                    astro::microLightYearsToKilometers(1.0);
            }

            return Mat4d::translation(p + frameOffset);
        }
        else
        {
            return Mat4d::translation(p);
        }
    }
    else
    {
        Mat4d frame;

        // TODO: inconsistent with orbitFrame != NULL case; shouldn't apply
        // rotation model of body, only of parents.
        frame = getRotationModel()->equatorOrientationAtTime(tjd).toMatrix4() *
            Mat4d::translation(pos);

        // Recurse up the hierarchy . . .
        if (orbitBarycenter != NULL)
            frame = frame * orbitBarycenter->getLocalToHeliocentric(tjd);
        return frame;
    }
}


// Return the position of the center of the body in heliocentric coordinates
Point3d Body::getHeliocentricPosition(double tjd) const
{
    Point3d pos = orbit->positionAtTime(tjd);

    if (orbitFrame != NULL)
    {
        Point3d p = orbitFrame->convertFromAstrocentric(pos, tjd);

        // Temporary hack; won't be necessary post-1.5.0 when this function
        // is redefined to return position with respect to frame root object
        // instead of name hierarchy root.
        if (frameRefStar != NULL)
        {
            Vec3d frameOffset(0.0, 0.0, 0.0);
            Star* refStar = getReferenceStar();
            if (refStar != NULL)
            {
                frameOffset = (frameRefStar->getPosition(tjd) - refStar->getPosition(tjd)) *
                    astro::microLightYearsToKilometers(1.0);
            }

            return p + frameOffset;
        }
        else
        {
            return p;
        }
    }
    else
    {
        // No orbit frame was specified; use the default orbit frame, which
        // is typically the mean equatorial frame of the parent object. The
        // exception is when the parent object is a star; in this case, the
        // orbit frame is just the ecliptical frame.
        if (orbitBarycenter != NULL)
        {
            // Object orbits a non-stellar object; get the parent's equatorial
            // frame and convert to ecliptical coordinates.
            Quatd orientation = orbitBarycenter->getEclipticalToEquatorial(tjd);
            return orbitBarycenter->getHeliocentricPosition(tjd) +
                pos * orientation.toMatrix3();
        }
        else
        {
            // Parent is a star; pos is already in ecliptical coordinates
            return pos;
        }
    }
}


Quatd Body::getEclipticalToFrame(double tjd) const
{
    Quatd q(1.0);

    if (bodyFrame != NULL)
    {
        q = bodyFrame->getOrientation(tjd);
    }
    else
    {
        if (orbitBarycenter != NULL)
            q = orbitBarycenter->getEclipticalToEquatorial(tjd);
    }

    return q;
}


Quatd Body::getEclipticalToEquatorial(double tjd) const
{
    Quatd q = getRotationModel()->equatorOrientationAtTime(tjd);
        
    if (bodyFrame != NULL)
    {
        return q * bodyFrame->getOrientation(tjd);
    }
    else
    {
        // Recurse up the hierarchy . . .
        if (orbitBarycenter != NULL)
            q = q * orbitBarycenter->getEclipticalToEquatorial(tjd);
    }
        
    return q;
}


Quatd Body::getEclipticalToBodyFixed(double when) const
{
    return getEquatorialToBodyFixed(when) * getEclipticalToEquatorial(when);
}


// The body-fixed coordinate system has an origin at the center of the
// body, y-axis parallel to the rotation axis, x-axis through the prime
// meridian, and z-axis at a right angle the xy plane.
Quatd Body::getEquatorialToBodyFixed(double when) const
{
    return rotationModel->spin(when);
}


Mat4d Body::getBodyFixedToHeliocentric(double when) const
{
    return getEquatorialToBodyFixed(when).toMatrix4() *
        getLocalToHeliocentric(when);
}


Vec3d Body::planetocentricToCartesian(double lon, double lat, double alt) const
{
    double phi = -degToRad(lat) + PI / 2;
    double theta = degToRad(lon) - PI;

    Vec3d pos(cos(theta) * sin(phi),
              cos(phi),
              -sin(theta) * sin(phi));

    return pos * (getRadius() + alt);
}


Vec3d Body::planetocentricToCartesian(const Vec3d& lonLatAlt) const
{
    return planetocentricToCartesian(lonLatAlt.x, lonLatAlt.y, lonLatAlt.z);
}


/*! Convert cartesian body-fixed coordinates to spherical planetocentric
 *  coordinates.
 */
Vec3d Body::cartesianToPlanetocentric(const Vec3d& v) const
{
    Vec3d w = v;
    w.normalize();

    double lat = PI / 2.0 - acos(w.y);
    double lon = atan2(w.z, -w.x);

    return Vec3d(lon, lat, v.length() - getRadius());
}


/*! Convert body-centered ecliptic coordinates to spherical planetocentric
 *  coordinates.
 */
Vec3d Body::eclipticToPlanetocentric(const Vec3d& ecl, double tdb) const
{
    Quatd q = getEclipticalToBodyFixed(tdb);
    Vec3d bf = ecl * (~q).toMatrix3();

    return cartesianToPlanetocentric(bf);
}



bool Body::extant(double t) const
{
    return t >= protos && t < eschatos;
}


void Body::setLifespan(double begin, double end)
{
    protos = begin;
    eschatos = end;
}


void Body::getLifespan(double& begin, double& end) const
{
    begin = protos;
    end = eschatos;
}


#define SOLAR_IRRADIANCE   1367.6
#define SOLAR_POWER           3.8462e26  // Watts

float Body::getLuminosity(const Star& sun,
                          float distanceFromSun) const
{
    return getLuminosity(sun.getLuminosity(), distanceFromSun);
}


float Body::getLuminosity(float sunLuminosity,
                          float distanceFromSun) const
{
    // Compute the total power of the star in Watts
    double power = SOLAR_POWER * sunLuminosity;

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


float Body::getApparentMagnitude(float sunLuminosity,
                                 float distanceFromSun,
                                 float distanceFromViewer) const
{
    return astro::lumToAppMag(getLuminosity(sunLuminosity, distanceFromSun),
                              astro::kilometersToLightYears(distanceFromViewer));
}


// Return the apparent magnitude of the body, corrected for the phase.
float Body::getApparentMagnitude(const Star& sun,
                                 const Vec3d& sunPosition,
                                 const Vec3d& viewerPosition) const
{
    return getApparentMagnitude(sun.getLuminosity(),
                                sunPosition,
                                viewerPosition);
}


float Body::getApparentMagnitude(float sunLuminosity,
                                 const Vec3d& sunPosition,
                                 const Vec3d& viewerPosition) const
{
    double distanceToViewer = viewerPosition.length();
    double distanceToSun = sunPosition.length();
    float illuminatedFraction = (float) (1.0 + (viewerPosition / distanceToViewer) *
                                         (sunPosition / distanceToSun)) / 2.0f;

    return astro::lumToAppMag(getLuminosity(sunLuminosity, (float) distanceToSun) * illuminatedFraction, (float) astro::kilometersToLightYears(distanceToViewer));
}


int Body::getClassification() const
{
    return classification;
}

void Body::setClassification(int _classification)
{
    classification = _classification;
}


string Body::getInfoURL() const
{
    return infoURL;
}

void Body::setInfoURL(const string& _infoURL)
{
    infoURL = _infoURL;
}


Surface* Body::getAlternateSurface(const string& name) const
{
    if (altSurfaces == NULL)
        return NULL;

    AltSurfaceTable::iterator iter = altSurfaces->find(name);
    if (iter == altSurfaces->end())
        return NULL;
    else
        return iter->second;
}


void Body::addAlternateSurface(const string& name, Surface* surface)
{
    if (altSurfaces == NULL)
        altSurfaces = new AltSurfaceTable();

    //altSurfaces->insert(AltSurfaceTable::value_type(name, surface));
    (*altSurfaces)[name] = surface;
}


vector<string>* Body::getAlternateSurfaceNames() const
{
    vector<string>* names = new vector<string>();
    if (altSurfaces != NULL)
    {
        for (AltSurfaceTable::const_iterator iter = altSurfaces->begin();
             iter != altSurfaces->end(); iter++)
        {
            names->insert(names->end(), iter->first);
        }
    }

    return names;
}


void Body::addLocation(Location* loc)
{
    assert(loc != NULL);
    if (loc == NULL)
        return;

    if (locations == NULL)
        locations = new vector<Location*>();
    locations->insert(locations->end(), loc);
    loc->setParentBody(this);
}


vector<Location*>* Body::getLocations() const
{
    return locations;
}


Location* Body::findLocation(const string& name, bool i18n) const
{
    if (locations == NULL)
        return NULL;

    for (vector<Location*>::const_iterator iter = locations->begin();
         iter != locations->end(); iter++)
    {
        if (!UTF8StringCompare(name, (*iter)->getName(i18n)))
            return *iter;
    }

    return NULL;
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

    // No work to do if there's no mesh, or if the mesh cannot be loaded
    if (model == InvalidResource)
        return;
    Model* m = GetModelManager()->find(model);
    if (m == NULL)
        return;

    // TODO: Implement separate radius and bounding radius so that this hack is
    // not necessary.
    double boundingRadius = 2.0;

    for (vector<Location*>::const_iterator iter = locations->begin();
         iter != locations->end(); iter++)
    {
        Vec3f v = (*iter)->getPosition();
        float alt = v.length() - radius;
        if (alt != -radius)
            v.normalize();
        v *= (float) boundingRadius;

        Ray3d ray(Point3d(v.x, v.y, v.z), Vec3d(-v.x, -v.y, -v.z));
        double t = 0.0;
        if (m->pick(ray, t))
        {
            v *= (float) ((1.0 - t) * radius + alt);
            (*iter)->setPosition(v);
        }
    }
}


bool
Body::referenceMarkVisible(uint32 refmark) const
{
    return (referenceMarks & refmark) != 0;
}


uint32
Body::getVisibleReferenceMarks() const
{
    return referenceMarks;
}


void
Body::setVisibleReferenceMarks(uint32 refmarks)
{
	referenceMarks = refmarks;
}


// Get the star at the root of the name hierarchy
// NOTE: This method shouldn't be required after cleanup of frame and name
// hierarchies post 1.5.0.
Star*
Body::getReferenceStar() const
{
    const Body* body = this;

    while (body->orbitBarycenter != NULL)
        body = body->orbitBarycenter;

    if (body->getSystem() != NULL)
        return body->getSystem()->getStar();
    else
        return NULL;
}


// Get the star at the root of the frame hierarchy
// NOTE: This method shouldn't be required after cleanup of frame and name
// hierarchies post 1.5.0.
Star*
Body::getFrameReferenceStar() const
{
    const Body* body = this;

    for (;;)
    {
        // Body has an orbit frame, parent is the frame center
        if (body->orbitFrame != NULL)
        {
            if (body->orbitFrame->getCenter().star() != NULL)
            {
                return body->orbitFrame->getCenter().star();
            }
            else if (body->orbitFrame->getCenter().body() != NULL)
            {
                body = body->orbitFrame->getCenter().body();
            }
            else
            {
                // Bad frame center: either a location, deep sky object, or
                // null object.
                return NULL;
            }
        }
        // Otherwise, use object's parent in the name hierarchy
        else
        {
            if (body->orbitBarycenter != NULL)
            {
                body = body->orbitBarycenter;
            }
            else
            {
                if (body->getSystem() != NULL)
                    return body->getSystem()->getStar();
                else
                    return NULL;
            }
        }
    }

    return NULL;
}


/*! Sets whether or not the object is visible.
 */
void Body::setVisible(bool _visible)
{
    visible = _visible ? 1 : 0;
}


/*! Sets whether or not the object can be selected by clicking on
 *  it. If set to false, the object is completely ignored when the
 *  user clicks it, making it possible to select background objects.
 */
void Body::setClickable(bool _clickable)
{
    clickable = _clickable ? 1 : 0;
}


/*! Set whether or not the object is visible as a starlike point
 *  when it occupies less than a pixel onscreen. This is appropriate
 *  for planets and moons, but generally not desireable for buildings
 *  or spacecraft components.
 */
void Body::setVisibleAsPoint(bool _visibleAsPoint)
{
    visibleAsPoint = _visibleAsPoint ? 1 : 0;
}


/*! The orbitColorOverride flag is set to true if an alternate orbit
 *  color should be used (specified via setOrbitColor) instead of the
 *  default class orbit color.
 */
void Body::setOrbitColorOverridden(bool override)
{
    overrideOrbitColor = override ? 1 : 0;
}


/*! Set the visibility policy for the orbit of this object:
 *  - NeverVisibile: Never show the orbit of this object.
 *  - UseClassVisibility: (Default) Show the orbit of this object
 *    its class is enabled in the orbit mask.
 *  - AlwaysVisibile: Always show the orbit of this object whenever
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



/**** Implementation of PlanetarySystem ****/

PlanetarySystem::PlanetarySystem(Body* _primary) : primary(_primary)
{
    if (primary != NULL && primary->getSystem() != NULL)
        star = primary->getSystem()->getStar();
    else
        star = NULL;
}

PlanetarySystem::PlanetarySystem(Star* _star) :
    star(_star), primary(NULL)
{
}


void PlanetarySystem::addBody(Body* body)
{
    satellites.insert(satellites.end(), body);
    objectIndex.insert(make_pair(body->getName(), body));
    i18nObjectIndex.insert(make_pair(body->getName(true), body));
}


void PlanetarySystem::removeBody(Body* body)
{
    for (vector<Body*>::iterator iter = satellites.begin();
         iter != satellites.end(); iter++)
    {
        if (*iter == body)
        {
            satellites.erase(iter);
            break;
        }
    }

    // Erase the object from the object indices
    for (ObjectIndex::iterator iter = objectIndex.begin();
         iter != objectIndex.end(); iter++)
    {
        if (iter->second == body)
        {
            objectIndex.erase(iter);
            break;
        }
    }

    for (ObjectIndex::iterator iter = i18nObjectIndex.begin();
         iter != i18nObjectIndex.end(); iter++)
    {
        if (iter->second == body)
        {
            i18nObjectIndex.erase(iter);
            break;
        }
    }

}


void PlanetarySystem::replaceBody(Body* oldBody, Body* newBody)
{
    for (vector<Body*>::iterator iter = satellites.begin();
         iter != satellites.end(); iter++)
    {
        if (*iter == oldBody)
        {
            *iter = newBody;
            break;
        }
    }

    // Erase the object from the object indices
    for (ObjectIndex::iterator iter = objectIndex.begin();
         iter != objectIndex.end(); iter++)
    {
        if (iter->second == oldBody)
        {
            objectIndex.erase(iter);
            break;
        }
    }

    for (ObjectIndex::iterator iter = i18nObjectIndex.begin();
         iter != i18nObjectIndex.end(); iter++)
    {
        if (iter->second == oldBody)
        {
            i18nObjectIndex.erase(iter);
            break;
        }
    }

    // Add the replacement to the object indices
    objectIndex.insert(make_pair(newBody->getName(), newBody));
    i18nObjectIndex.insert(make_pair(newBody->getName(true), newBody));
}


Body* PlanetarySystem::find(const string& _name, bool deepSearch, bool i18n) const
{
    ObjectIndex index = i18n?i18nObjectIndex:objectIndex;
    ObjectIndex::const_iterator firstMatch = index.find(_name);
    if (firstMatch != index.end())
    {
        return firstMatch->second;
    }

    if (deepSearch)
    {
        for (vector<Body*>::const_iterator iter = satellites.begin();
             iter != satellites.end(); iter++)
        {
            if (UTF8StringCompare((*iter)->getName(i18n), _name) == 0)
            {
                return *iter;
            }
            else if (deepSearch && (*iter)->getSatellites() != NULL)
            {
                Body* body = (*iter)->getSatellites()->find(_name, deepSearch, i18n);
                if (body != NULL)
                    return body;
            }
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

std::vector<std::string> PlanetarySystem::getCompletion(const std::string& _name, bool rec) const
{
    std::vector<std::string> completion;
    int _name_length = UTF8Length(_name);

    for (vector<Body*>::const_iterator iter = satellites.begin();
         iter != satellites.end(); iter++)
    {
        if (UTF8StringCompare((*iter)->getName(true), _name, _name_length) == 0)
        {
            completion.push_back((*iter)->getName(true));
        }
        if (rec && (*iter)->getSatellites() != NULL)
        {
            std::vector<std::string> bodies = (*iter)->getSatellites()->getCompletion(_name);
            completion.insert(completion.end(), bodies.begin(), bodies.end());
        }
    }

    return completion;
}

