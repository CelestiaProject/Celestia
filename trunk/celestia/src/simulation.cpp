// simulation.cpp
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// The core of Celestia--tracks an observer moving through a
// stars and their solar systems.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>

#include "mathlib.h"
#include "vecmath.h"
#include "perlin.h"
#include "astro.h"
#include "simulation.h"

using namespace std;

#define VELOCITY_CHANGE_TIME      1.0f


Simulation::Simulation() :
    realTime(0.0),
    simTime(0.0),
    timeScale(1.0),
    stardb(NULL),
    solarSystemCatalog(NULL),
    visibleStars(NULL),
    closestSolarSystem(NULL),
    selection(),
    targetSpeed(0.0),
    targetVelocity(0.0, 0.0, 0.0),
    initialVelocity(0.0, 0.0, 0.0),
    beginAccelTime(0.0),
    observerMode(Free),
    hudDetail(1),
    faintestVisible(5.0f)
{
}


Simulation::~Simulation()
{
    // TODO: Clean up!
}


static void displayDistance(Console& console, double distance)
{
    if (distance >= astro::AUtoLightYears(1000.0f))
        console.printf("%.3f ly", distance);
    else if (distance >= astro::kilometersToLightYears(10000000.0))
        console.printf("%.3f au", astro::lightYearsToAU(distance));
    else
        console.printf("%.3f km", astro::lightYearsToKilometers(distance));
}


static void displayStarInfo(Console& console,
                            Star& star,
                            StarDatabase& starDB,
                            double distance)
{
    StarNameDatabase* starNameDB = starDB.getNameDatabase();

    // Print the star name and designations
    StarNameDatabase::iterator iter = starNameDB->find(star.getCatalogNumber());
    if (iter != starNameDB->end())
    {
        StarName* starName = iter->second;

        if (starName->getName() != "")
            console << starName->getName() << "  /  ";

        Constellation* constellation = starName->getConstellation();
        if (constellation != NULL)
            console << starName->getDesignation() << ' ' <<  constellation->getGenitive() << "  /  ";
    }
    if ((star.getCatalogNumber() & 0xf0000000) == 0)
        console << "HD " << star.getCatalogNumber() << '\n';
    else
        console << "HIP " << (star.getCatalogNumber() & 0x0fffffff) << '\n';
    
    console.printf("Abs (app) mag = %.2f (%.2f)\n",
                   star.getAbsoluteMagnitude(),
                   astro::absToAppMag(star.getAbsoluteMagnitude(), (float) distance));
    console << "Class: " << star.getStellarClass() << '\n';
    console.printf("Radius: %.2f Rsun\n", star.getRadius() / 696000.0f);
}


static void displayPlanetInfo(Console& console,
                              Body& body,
                              double distance)
{
    console << body.getName() << '\n';
    console << "Radius: " << body.getRadius() << " km\n";
    console << "Day length: " << body.getRotationPeriod() << " hours\n";
}


void Simulation::displaySelectionInfo(Console& console)
{
    if (selection.star != NULL)
    {
        Vec3d v = astro::universalPosition(Point3d(0, 0, 0), selection.star->getPosition()) - 
            observer.getPosition();
        console << "Distance to target: ";
        displayDistance(console, v.length());
        console << '\n';
        displayStarInfo(console, *selection.star, *stardb, v.length());
    }
    else if (selection.body != NULL)
    {
        uint32 starNumber = Star::InvalidStar;
        if (selection.body->getSystem() != NULL)
            starNumber = selection.body->getSystem()->getStarNumber();
        Star *star = stardb->find(starNumber);
        
        if (star != NULL)
        {
            Vec3d v = astro::universalPosition(selection.body->getHeliocentricPosition(simTime / 86400.0), star->getPosition()) - observer.getPosition();
            console << "Distance to target: ";
            displayDistance(console, v.length());
            console << '\n';
            displayPlanetInfo(console, *selection.body, v.length());
        }
    }
}


void  Simulation::render(Renderer& renderer)
{
    Console* console = renderer.getConsole();
    console->clear();
    console->home();

    // Temporary hack, just for this version . . .
    if (realTime < 15.0)
    {
        console->printf("Welcome to Celestia 1.05 (Mir Edition)\n");
        console->printf("Right drag mouse to rotate around Mir\n");
        console->printf("Hit ESC to stop tracking Mir and explore the rest of the universe\n\n");
    }

    if (hudDetail > 0)
    {
        console->printf("Visible stars = %d\n", visibleStars->getVisibleSet()->size());

        // Display the velocity
        {
            double v = observer.getVelocity().length();
            char* units;

            if (v < astro::AUtoLightYears(1000))
            {
                if (v < astro::kilometersToLightYears(10000000.0f))
                {
                    v = astro::lightYearsToKilometers(v);
                    units = "km/s";
                }
                else
                {
                    v = astro::lightYearsToAU(v);
                    units = "AU/s";
                }
            }
            else
            {
                units = "ly/s";
            }

            console->printf("Speed: %f %s\n", v, units);
        }

        // Display the date
        *console << astro::Date(simTime / 86400.0) << '\n';

        displaySelectionInfo(*console);
    }

    renderer.render(observer,
                    *stardb,
                    *visibleStars,
                    closestSolarSystem,
                    selection,
                    simTime);
}


static Quatf lookAt(Point3f from, Point3f to, Vec3f up)
{
    Vec3f n = from - to;
    n.normalize();
    Vec3f v = up ^ n;
    v.normalize();
    Mat4f r = Mat4f::rotation(v, (float) PI / 2);
    Vec3f u = n * r;
    return ~Quatf(Mat3f(v, u, n));
}


SolarSystem* Simulation::getSolarSystem(Star* star)
{
    uint32 starNum = star->getCatalogNumber();
    
    SolarSystemCatalog::iterator iter = solarSystemCatalog->find(starNum);
    if (iter == solarSystemCatalog->end())
        return NULL;
    else
        return iter->second;
}


Star* Simulation::getSun(Body* body)
{
    PlanetarySystem* system = body->getSystem();
    if (system == NULL)
        return NULL;
    else
        return stardb->find(system->getStarNumber());
}


UniversalCoord Simulation::getSelectionPosition(Selection& sel, double when)
{
    if (sel.body != NULL)
    {
        Point3f sunPos(0.0f, 0.0f, 0.0f);
        Star* sun = getSun(sel.body);
        if (sun != NULL)
            sunPos = sun->getPosition();
        return astro::universalPosition(sel.body->getHeliocentricPosition(when / 86400.0),
                                        sunPos);
    }
    else if (sel.star != NULL)
    {
        return astro::universalPosition(Point3d(0.0, 0.0, 0.0), sel.star->getPosition());
    }
    else
    {
        return UniversalCoord(Point3d(0.0, 0.0, 0.0));
    }
}


float getSelectionSize(Selection& sel)
{
    if (sel.body != NULL)
        return sel.body->getRadius();
    else if (sel.star != NULL)
        return sel.star->getRadius();
    else
        return 0.0f;
}


void Simulation::setStarDatabase(StarDatabase* db,
                                 SolarSystemCatalog* catalog)
{
    stardb = db;
    solarSystemCatalog = catalog;

    if (visibleStars != NULL)
    {
        delete visibleStars;
        visibleStars = NULL;
    }
    if (db != NULL)
    {
        visibleStars = new VisibleStarSet(stardb);
        visibleStars->setLimitingMagnitude(faintestVisible);
        visibleStars->setCloseDistance(10.0f);
        visibleStars->updateAll(observer);
    }
}


// Get the time in seconds (JD * 86400)
double Simulation::getTime() const
{
    return simTime;
}

// Set the time in seconds (JD * 86400)
void Simulation::setTime(double t)
{
    simTime = t;
}


void Simulation::update(double dt)
{
    realTime += dt;
    simTime += dt * timeScale;

    if (observerMode == Travelling)
    {
        float t = clamp((realTime - journey.startTime) / journey.duration);

        // Smooth out the linear interpolation of position
        float u = (float) sin(sin(t * PI / 2) * PI / 2);
        Vec3d jv = journey.to - journey.from;
        UniversalCoord p = journey.from + jv * (double) u;

        // Interpolate the orientation using lookAt()
        // We gradually change the focus point for lookAt() from the initial 
        // focus to the destination star over the first half of the journey
        Vec3d lookDir0 = journey.initialFocus - journey.from;
        Vec3d lookDir1 = journey.finalFocus - journey.to;
        lookDir0.normalize();
        lookDir1.normalize();
        Vec3d lookDir;
        if (t < 0.5f)
        {
            // Smooth out the interpolation of the focus point to avoid
            // jarring changes in orientation
            double v = sin(t * PI);

            double c = lookDir0 * lookDir1;
            double angle = acos(c);
            if (c >= 1.0 || angle < 1.0e-6)
            {
                lookDir = lookDir0;
            }
            else
            {
                double s = sin(angle);
                double is = 1.0 / s;

                // Spherically interpolate the look direction between the
                // intial and final directions.
                lookDir = lookDir0 * (sin((1 - v) * angle) * is) +
                          lookDir1 * (sin(v * angle) * is);

                // Linear interpolation wasn't such a good idea . . .
                // lookDir = lookDir0 + (lookDir1 - lookDir0) * v;
            }
        }
        else
        {
            lookDir = lookDir1;
        }

        observer.setOrientation(lookAt(Point3f(0, 0, 0),
                                       Point3f((float) lookDir.x,
                                               (float) lookDir.y,
                                               (float) lookDir.z),
                                       journey.up));
        observer.setPosition(p);

        // If the journey's complete, reset to manual control
        if (t == 1.0f)
        {
            observer.setPosition(journey.to);
            observerMode = Free;
            observer.setVelocity(Vec3d(0, 0, 0));
            targetVelocity = Vec3d(0, 0, 0);
        }
    }
    else if (observerMode == Following)
    {
        Point3d posRelToSun = followInfo.body->getHeliocentricPosition(simTime / 86400.0) + followInfo.offset;
        observer.setPosition(astro::universalPosition(posRelToSun,
                                                      followInfo.sun->getPosition()));
    }
    else
    {
        if (observer.getVelocity() != targetVelocity)
        {
            double t = clamp((realTime - beginAccelTime) / VELOCITY_CHANGE_TIME);
            observer.setVelocity(observer.getVelocity() * (1.0 - t) +
                                 targetVelocity * t);
        }

        observer.update(dt);
    }

    if (visibleStars != NULL)
        visibleStars->update(observer, 0.05f);

    // Find the closest solar system
    Point3f observerPos = (Point3f) observer.getPosition();
    vector<uint32>* closeStars = visibleStars->getCloseSet();
    for (int i = 0; i < closeStars->size(); i++)
    {
        uint32 starIndex = (*closeStars)[i];
        Star* star = stardb->getStar(starIndex);
        if (observerPos.distanceTo(star->getPosition()) < 1.0f)
        {
            SolarSystem* solarSystem = getSolarSystem(star);
            if (solarSystem != NULL)
                closestSolarSystem = solarSystem;
        }
    }

}


Selection Simulation::getSelection() const
{
    return selection;
}


void Simulation::setSelection(const Selection& sel)
{
    selection = sel;
}


struct PlanetPickInfo
{
    float cosClosestAngle;
    double closestDistance;
    Body* closestBody;
    Vec3d direction;
    Point3d origin;
    double jd;
};

bool ApproxPlanetPickTraversal(Body* body, void* info)
{
    PlanetPickInfo* pickInfo = (PlanetPickInfo*) info;

    Point3d bpos = body->getHeliocentricPosition(pickInfo->jd);
    Vec3d bodyDir = bpos - pickInfo->origin;
    bodyDir.normalize();
    double cosAngle = bodyDir * pickInfo->direction;
    if (cosAngle > pickInfo->cosClosestAngle)
    {
        pickInfo->cosClosestAngle = cosAngle;
        pickInfo->closestBody = body;
    }

    return true;
}


// Perform an intersection test between the pick ray and a body
bool ExactPlanetPickTraversal(Body* body, void* info)
{
    PlanetPickInfo* pickInfo = (PlanetPickInfo*) info;

    Point3d bpos = body->getHeliocentricPosition(pickInfo->jd);
    Vec3d bodyDir = bpos - pickInfo->origin;

    // This intersection test naively assumes that the body is spherical.
    double v = bodyDir * pickInfo->direction;
    double disc = square(body->getRadius()) - ((bodyDir * bodyDir) - square(v));

    if (disc > 0.0)
    {
        double distance = v - sqrt(disc);
        if (distance > 0.0 && distance < pickInfo->closestDistance)
        {
            pickInfo->closestDistance = distance;
            pickInfo->closestBody = body;
        }
    }

    return true;
}


Selection Simulation::pickPlanet(Observer& observer,
                                 Star& sun,
                                 SolarSystem& solarSystem,
                                 Vec3f pickRay)
{
    PlanetPickInfo pickInfo;

    // Transform the pick direction
    pickRay = pickRay * observer.getOrientation().toMatrix4();
    pickInfo.direction = Vec3d((double) pickRay.x,
                               (double) pickRay.y,
                               (double) pickRay.z);
    pickInfo.origin    = astro::heliocentricPosition(observer.getPosition(),
                                                     sun.getPosition());
    pickInfo.cosClosestAngle = -1.0f;
    pickInfo.closestDistance = 1.0e50;
    pickInfo.closestBody = NULL;
    pickInfo.jd = simTime / 86400.0f;

    // First see if there's a planet that the pick ray intersects.
    // Select the closest planet intersected.
    solarSystem.getPlanets()->traverse(ExactPlanetPickTraversal,
                                       (void*) &pickInfo);
    if (pickInfo.closestBody != NULL)
    {
        return Selection(pickInfo.closestBody);
    }

    // If no planet was intersected by the pick ray, choose the planet
    // with the smallest angular separation from the pick ray.  Very distant
    // planets are likley to fail the intersection test even if the user
    // clicks on a pixel where the planet's disc has been rendered--in order
    // to make distant planets visible on the screen at all, their apparent size
    // has to be greater than their actual disc size.
    solarSystem.getPlanets()->traverse(ApproxPlanetPickTraversal,
                                       (void*) &pickInfo);
    if (pickInfo.cosClosestAngle > cos(degToRad(0.5f)))
        return Selection(pickInfo.closestBody);
    else
        return Selection();
}


Selection Simulation::pickStar(Vec3f pickRay)
{
    // Transform the pick direction
    pickRay = pickRay * observer.getOrientation().toMatrix4();

    Point3f observerPos = (Point3f) observer.getPosition();
    vector<uint32>* vis = visibleStars->getVisibleSet();
    int nStars = vis->size();


    float cosAngleClosest = -1.0f;
    int closest = -1;

    for (int i = 0; i < nStars; i++)
    {
        int starIndex = (*vis)[i];
        Star* star = stardb->getStar(starIndex);
        Vec3f starDir = star->getPosition() - observerPos;
        starDir.normalize();
        
        float cosAngle = starDir * pickRay;
        if (cosAngle > cosAngleClosest)
        {
            cosAngleClosest = cosAngle;
            closest = starIndex;
        }
    }

    if (cosAngleClosest > cos(degToRad(0.5f)))
        return Selection(stardb->getStar(closest));
    else
        return Selection();
}


Selection Simulation::pickObject(Vec3f pickRay)
{
    Point3f observerPos = (Point3f) observer.getPosition();
    vector<uint32>* closeStars = visibleStars->getCloseSet();
    Selection sel;

    for (int i = 0; i < closeStars->size(); i++)
    {
        uint32 starIndex = (*closeStars)[i];
        Star* star = stardb->getStar(starIndex);

        // Only attempt to pick planets if the star is less
        // than one light year away.  Seems like a reasonable limit . . .
        if (observerPos.distanceTo(star->getPosition()) < 1.0f)
        {
            SolarSystem* solarSystem = getSolarSystem(star);
            if (solarSystem != NULL)
                sel = pickPlanet(observer, *star, *solarSystem, pickRay);
        }
    }

    if (sel.empty())
        sel = pickStar(pickRay);

    return sel;
}


void Simulation::computeGotoParameters(Selection& destination, JourneyParams& jparams,
                                       double gotoTime)
{
    UniversalCoord targetPosition = getSelectionPosition(selection, simTime);
    Vec3d v = targetPosition - observer.getPosition();
    double distanceToTarget = v.length();
    double maxOrbitDistance = (selection.body != NULL) ? astro::kilometersToLightYears(5.0f * selection.body->getRadius()) : 0.5f;
    double orbitDistance = (distanceToTarget > maxOrbitDistance * 10.0f) ? maxOrbitDistance : distanceToTarget * 0.1f;

    v.normalize();

    jparams.duration = gotoTime;
    jparams.startTime = realTime;

    // Right where we are now . . .
    jparams.from = observer.getPosition();

    // The destination position lies along the line between the current
    // position and the star
    jparams.to = targetPosition - v * orbitDistance;
    jparams.initialFocus = jparams.from +
        (Vec3f(0, 0, -1.0f) * observer.getOrientation().toMatrix4());
    jparams.finalFocus = targetPosition;
    jparams.up = Vec3f(0, 1, 0) * observer.getOrientation().toMatrix4();
}


void Simulation::computeCenterParameters(Selection& destination, JourneyParams& jparams,
                                         double centerTime)
{
    UniversalCoord targetPosition = getSelectionPosition(selection, simTime);

    jparams.duration = centerTime;
    jparams.startTime = realTime;

    // Don't move through space, just rotate the camera
    jparams.from = observer.getPosition();
    jparams.to = jparams.from;

    jparams.initialFocus = jparams.from +
        (Vec3f(0, 0, -1.0f) * observer.getOrientation().toMatrix4());
    jparams.finalFocus = targetPosition;
    jparams.up = Vec3f(0, 1, 0) * observer.getOrientation().toMatrix4();
}


void Simulation::applyForce(Vec3f force, float dt)
{
    Vec3d f(force.x * dt, force.y * dt, force.z * dt);
    observer.setVelocity(observer.getVelocity() + f);
}

void Simulation::applyForceRelativeToOrientation(Vec3f force, float dt)
{
    applyForce(force * observer.getOrientation().toMatrix4(), dt);
}

Quatf Simulation::getOrientation()
{
    return observer.getOrientation();
}

void Simulation::setOrientation(Quatf q)
{
    observer.setOrientation(q);
}

// Orbit around the selection (if there is one.)  This involves changing
// both the observer's position and orientation.
void Simulation::orbit(Quatf q)
{
    if (selection.body != NULL || selection.star != NULL)
    {
        UniversalCoord focusPosition = getSelectionPosition(selection, simTime);
        Vec3d v = observer.getPosition() - focusPosition;
        double distance = v.length();

        Mat3f m = conjugate(observer.getOrientation()).toMatrix3();


        // Convert the matrix to double precision so we can multiply it
        // by the double precision vector.  Yuck.  VC doesn't seem to
        // be able to figure out that the constructor declared in
        // vecmath.h should allow: md = m
        Mat3d md;
        md[0][0] = m[0][0]; md[0][1] = m[0][1]; md[0][2] = m[0][2];
        md[1][0] = m[1][0]; md[1][1] = m[1][1]; md[1][2] = m[1][2];
        md[2][0] = m[2][0]; md[2][1] = m[2][1]; md[2][2] = m[2][2];
        v = v * md;

        observer.setOrientation(observer.getOrientation() * q);
        
        m = observer.getOrientation().toMatrix3();
        md[0][0] = m[0][0]; md[0][1] = m[0][1]; md[0][2] = m[0][2];
        md[1][0] = m[1][0]; md[1][1] = m[1][1]; md[1][2] = m[1][2];
        md[2][0] = m[2][0]; md[2][1] = m[2][1]; md[2][2] = m[2][2];
        v = v * md;

        // Roundoff errors will accumulate and cause the distance between
        // viewer and focus to change unless we take steps to keep the
        // length of v constant.
        v.normalize();
        v *= distance;

        observer.setPosition(focusPosition + v);

        if (observerMode == Following)
            followInfo.offset = v * astro::lightYearsToKilometers(1.0);
    }
}


// Exponential camera dolly--move toward or away from the selected object
// at a rate dependent on the observer's distance from the object.
void Simulation::changeOrbitDistance(float d)
{
    if (selection.body != NULL || selection.star != NULL)
    {
        UniversalCoord focusPosition = getSelectionPosition(selection, simTime);
        double size = getSelectionSize(selection);

        // Somewhat arbitrary parameters to chosen to give the camera movement
        // a nice feel.  They should probably be function parameters.
        double minOrbitDistance = astro::kilometersToLightYears(1.05 * size);
        double naturalOrbitDistance = astro::kilometersToLightYears(4.0 * size);

        // Determine distance and direction to the selected object
        Vec3d v = observer.getPosition() - focusPosition;
        double currentDistance = v.length();

        if (currentDistance >= minOrbitDistance && naturalOrbitDistance != 0)
        {
            double r = (currentDistance - minOrbitDistance) / naturalOrbitDistance;
            double newDistance = minOrbitDistance + naturalOrbitDistance * exp(log(r) + d);
            v = v * (newDistance / currentDistance);
            observer.setPosition(focusPosition + v);

            if (observerMode == Following)
                followInfo.offset = v * astro::lightYearsToKilometers(1.0);
        }
    }
}


void Simulation::setTargetSpeed(float s)
{
    targetSpeed = s;
    Vec3f v = Vec3f(0, 0, -s) * observer.getOrientation().toMatrix4();
    targetVelocity = Vec3d(v.x, v.y, v.z);
    initialVelocity = observer.getVelocity();
    beginAccelTime = realTime;
}

float Simulation::getTargetSpeed()
{
    return targetSpeed;
}

void Simulation::gotoSelection(double gotoTime)
{
    if (selection.body != NULL || selection.star != NULL)
    {
        computeGotoParameters(selection, journey, gotoTime);
        observerMode = Travelling;
    }
}

void Simulation::cancelMotion()
{
    observerMode = Free;
}

void Simulation::centerSelection(double centerTime)
{
    if (selection.body != NULL || selection.star != NULL)
    {
        computeCenterParameters(selection, journey, centerTime);
        observerMode = Travelling;
    }
}

void Simulation::follow()
{
    if (observerMode == Following)
    {
        observerMode = Free;
    }
    else
    {
        if (selection.body != NULL)
        {
            Star* sun = getSun(selection.body);
            if (sun != NULL)
            {
                observerMode = Following;
                followInfo.sun = sun;
                followInfo.body = selection.body;
                Point3d planetPos = selection.body->getHeliocentricPosition(simTime / 86400.0);
                Point3d observerPos = astro::heliocentricPosition(observer.getPosition(),
                                                                  sun->getPosition());
                followInfo.offset = observerPos - planetPos;
            }
        }
    }
}


void Simulation::selectStar(uint32 catalogNo)
{
    selection = Selection(stardb->find(catalogNo));
}


void Simulation::selectPlanet(int index)
{
    if (index < 0)
    {
        if (selection.body != NULL)
        {
            PlanetarySystem* system = selection.body->getSystem();
            if (system != NULL)
                selectStar(system->getStarNumber());
        }
    }
    else
    {
        Star* star = NULL;
        if (selection.star != NULL)
        {
            star = selection.star;
        }
        else if (selection.body != NULL)
        {
            star = getSun(selection.body);
        }

        SolarSystem* solarSystem = NULL;
        if (star != NULL)
            solarSystem = getSolarSystem(star);
        else
            solarSystem = closestSolarSystem;

        if (solarSystem != NULL && index < solarSystem->getPlanets()->getSystemSize())
            selection = Selection(solarSystem->getPlanets()->getBody(index));
    }
}


void Simulation::selectBody(string s)
{
    Star* star = stardb->find(s);
    if (star != NULL)
    {
        selectStar(star->getCatalogNumber());
    }
    else if (closestSolarSystem != NULL)
    {
        Body* body = closestSolarSystem->getPlanets()->find(s, true);
        if (body != NULL)
            selection = Selection(body);
    }
}


double Simulation::getTimeScale()
{
    return timeScale;
}

void Simulation::setTimeScale(double _timeScale)
{
    timeScale = _timeScale;
}


float Simulation::getFaintestVisible() const
{
    return faintestVisible;
}

void Simulation::setFaintestVisible(float magnitude)
{
    faintestVisible = magnitude;
    visibleStars->setLimitingMagnitude(faintestVisible);
}


int Simulation::getHUDDetail() const
{
    return hudDetail;
}

void Simulation::setHUDDetail(int detail)
{
    hudDetail = detail;
}


SolarSystem* Simulation::getNearestSolarSystem() const
{
    return closestSolarSystem;
}
