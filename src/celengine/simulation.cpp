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

#include <celmath/mathlib.h>
#include <celmath/vecmath.h>
#include <celmath/perlin.h>
#include <celmath/solve.h>

#include "astro.h"
#include "simulation.h"

using namespace std;

#define VELOCITY_CHANGE_TIME      0.25f


Simulation::Simulation(Universe* _universe) :
    realTime(0.0),
    simTime(0.0),
    timeScale(1.0),
    universe(_universe),
    closestSolarSystem(NULL),
    selection(),
    targetSpeed(0.0),
    targetVelocity(0.0, 0.0, 0.0),
    initialVelocity(0.0, 0.0, 0.0),
    beginAccelTime(0.0),
    observerMode(Free),
    faintestVisible(5.0f)
{
}

Simulation::~Simulation()
{
    // TODO: Clean up!
}


static const Star* getSun(Body* body)
{
    PlanetarySystem* system = body->getSystem();
    if (system == NULL)
        return NULL;
    else
        return system->getStar();
}


static RigidTransform toUniversal(const FrameOfReference& frame,
                                  const RigidTransform& xform,
                                  double t)
{
    // Handle the easy case . . .
    if (frame.coordSys == astro::Universal)
        return xform;
    UniversalCoord origin = frame.refObject.getPosition(t);

    if (frame.coordSys == astro::Geographic)
    {
        Quatd rotation(1, 0, 0, 0);
        if (frame.refObject.body != NULL)
            rotation = frame.refObject.body->getEclipticalToGeographic(t);
        else if (frame.refObject.star != NULL)
            rotation = Quatd(1, 0, 0, 0);
        Point3d p = (Point3d) xform.translation * rotation.toMatrix4();

        return RigidTransform(origin + Vec3d(p.x, p.y, p.z),
                              xform.rotation * rotation);
    }
    else if (frame.coordSys == astro::PhaseLock)
    {
        Mat3d m;
        if (frame.refObject.body != NULL)
        {
            Body* body = frame.refObject.body;
            Vec3d lookDir = frame.refObject.getPosition(t) -
                frame.targetObject.getPosition(t);
            Vec3d axisDir = Vec3d(0, 1, 0) * body->getEclipticalToEquatorial().toMatrix3();
            lookDir.normalize();
            Vec3d v = lookDir ^ axisDir;
            v.normalize();
            Vec3d u = v ^ lookDir;
            m = Mat3d(v, u, -lookDir);
        }

        Point3d p = (Point3d) xform.translation * m;

        return RigidTransform(origin + Vec3d(p.x, p.y, p.z),
                              xform.rotation * Quatd(m));
    }
    else if (frame.coordSys == astro::Chase)
    {
        Mat3d m;
        if (frame.refObject.body != NULL)
        {
            Body* body = frame.refObject.body;
            Vec3d lookDir = body->getOrbit()->positionAtTime(t) -
                body->getOrbit()->positionAtTime(t - 1.0 / 1440.0);
            Vec3d axisDir = Vec3d(0, 1, 0) * body->getEclipticalToEquatorial().toMatrix3();
            lookDir.normalize();
            Vec3d v = lookDir ^ axisDir;
            v.normalize();
            Vec3d u = v ^ lookDir;
            m = Mat3d(v, u, -lookDir);
        }

        Point3d p = (Point3d) xform.translation * m;

        return RigidTransform(origin + Vec3d(p.x, p.y, p.z),
                              xform.rotation * Quatd(m));
    }
    else
    {
        return RigidTransform(origin + xform.translation, xform.rotation);
    }
}


static RigidTransform fromUniversal(const FrameOfReference& frame,
                                    const RigidTransform& xform,
                                    double t)
{
    // Handle the easy case . . .
    if (frame.coordSys == astro::Universal)
        return xform;
    UniversalCoord origin = frame.refObject.getPosition(t);

    if (frame.coordSys == astro::Geographic)
    {
        Quatd rotation(1, 0, 0, 0);
        if (frame.refObject.body != NULL)
            rotation = frame.refObject.body->getEclipticalToGeographic(t);
        else if (frame.refObject.star != NULL)
            rotation = Quatd(1, 0, 0, 0);
        Vec3d v = (xform.translation - origin) * (~rotation).toMatrix4();
        
        return RigidTransform(UniversalCoord(v.x, v.y, v.z),
                              xform.rotation * ~rotation);
    }
    else if (frame.coordSys == astro::PhaseLock)
    {
        Mat3d m;
        if (frame.refObject.body != NULL)
        {
            Body* body = frame.refObject.body;
            Vec3d lookDir = frame.refObject.getPosition(t) -
                frame.targetObject.getPosition(t);
            Vec3d axisDir = Vec3d(0, 1, 0) * body->getEclipticalToEquatorial().toMatrix3();
            lookDir.normalize();
            Vec3d v = lookDir ^ axisDir;
            v.normalize();
            Vec3d u = v ^ lookDir;
            m = Mat3d(v, u, -lookDir);
        }

        Vec3d v = (xform.translation - origin) * m.transpose();

        return RigidTransform(UniversalCoord(v.x, v.y, v.z),
                              xform.rotation * ~Quatd(m));
    }
    else if (frame.coordSys == astro::Chase)
    {
        Mat3d m;
        if (frame.refObject.body != NULL)
        {
            Body* body = frame.refObject.body;
            Vec3d lookDir = body->getOrbit()->positionAtTime(t) -
                body->getOrbit()->positionAtTime(t - 1.0 / 1440.0);
            Vec3d axisDir = Vec3d(0, 1, 0) * body->getEclipticalToEquatorial().toMatrix3();
            lookDir.normalize();
            Vec3d v = lookDir ^ axisDir;
            v.normalize();
            Vec3d u = v ^ lookDir;
            m = Mat3d(v, u, -lookDir);
        }

        Vec3d v = (xform.translation - origin) * m.transpose();

        return RigidTransform(UniversalCoord(v.x, v.y, v.z),
                              xform.rotation * ~Quatd(m));
    }
    else
    {
        return RigidTransform(xform.translation.difference(origin),
                              xform.rotation);
    }
}



void Simulation::render(Renderer& renderer)
{
    renderer.render(observer,
                    *universe,
                    faintestVisible,
                    selection,
                    simTime);
}


static Quatf lookAt(Point3f from, Point3f to, Vec3f up)
{
    Vec3f n = to - from;
    n.normalize();
    Vec3f v = n ^ up;
    v.normalize();
    Vec3f u = v ^ n;
    return Quatf(Mat3f(v, u, -n));
}


Universe* Simulation::getUniverse() const
{
    return universe;
}


// Get the time (Julian date)
double Simulation::getTime() const
{
    return simTime;
}

// Set the time to the specified Julian date
void Simulation::setTime(double jd)
{
    simTime = jd;
}


// Set the observer position and orientation based on the frame of reference
void Simulation::updateObserver()
{
    RigidTransform t = toUniversal(frame, transform, simTime);
    observer.setPosition(t.translation);
    observer.setOrientation(t.rotation);
}


// Tick the simulation by dt seconds
void Simulation::update(double dt)
{
    realTime += dt;
    simTime += (dt / 86400.0) * timeScale;

    if (observerMode == Travelling)
    {
        float t = clamp((realTime - journey.startTime) / journey.duration);

        Vec3d jv = journey.to - journey.from;
        UniversalCoord p;
#if 0
        {
            // Smooth out the linear interpolation of position with sin(sin())
            float u = (float) sin(sin(t * PI / 2) * PI / 2);
            p = journey.from + jv * (double) u;
        }
#endif

        // Another interpolation method . . . accelerate exponentially,
        // maintain a constant velocity for a period of time, then
        // decelerate.  The portion of the trip spent accelerating is
        // controlled by the parameter journey.accelTime; a value of 1 means
        // that the entire first half of the trip will be spent accelerating
        // and there will be no coasting at constant velocity.
        {
            double u = t < 0.5 ? t * 2 : (1 - t) * 2;
            double x;
            if (u < journey.accelTime)
            {
                x = exp(journey.expFactor * u) - 1.0;
            }
            else
            {
                x = exp(journey.expFactor * journey.accelTime) *
                    (journey.expFactor * (u - journey.accelTime) + 1) - 1;
            }

            Vec3d v = jv;
            v.normalize();
            if (t < 0.5)
                p = journey.from + v * astro::kilometersToMicroLightYears(x);
            else
                p = journey.to - v * astro::kilometersToMicroLightYears(x);
        }

        // Spherically interpolate the orientation over the first half
        // of the journey.
        Quatf orientation;
        if (t < 0.5f)
        {
            // Smooth out the interpolation to avoid jarring changes in
            // orientation
            double v = sin(t * PI);

            // Be careful to choose the shortest path when interpolating
            if (norm(journey.initialOrientation - journey.finalOrientation) <
                norm(journey.initialOrientation + journey.finalOrientation))
            {
                orientation = Quatf::slerp(journey.initialOrientation,
                                           journey.finalOrientation, v);
            }
            else
            {
                orientation = Quatf::slerp(journey.initialOrientation,
                                           -journey.finalOrientation, v);
            }
        }
        else
        {
            orientation = journey.finalOrientation;
        }

        transform = RigidTransform(p, orientation);

        // If the journey's complete, reset to manual control
        if (t == 1.0f)
        {
            transform = RigidTransform(journey.to, journey.finalOrientation);
            observerMode = Free;
            observer.setVelocity(Vec3d(0, 0, 0));
//            targetVelocity = Vec3d(0, 0, 0);
        }
    }

    if (observer.getVelocity() != targetVelocity)
    {
        double t = clamp((realTime - beginAccelTime) / VELOCITY_CHANGE_TIME);
        observer.setVelocity(observer.getVelocity() * (1.0 - t) +
                             targetVelocity * t);
    }

    // Update the observer's position
    transform.translation = transform.translation + observer.getVelocity() * dt;

    if (observerMode == Free)
    {
        // Update the observer's orientation
        Vec3f fAV = observer.getAngularVelocity();
        Vec3d AV(fAV.x, fAV.y, fAV.z);
        Quatd dr = 0.5 * (AV * transform.rotation);
        transform.rotation += dt * dr;
        transform.rotation.normalize();
    }

    updateObserver();

    if (!trackObject.empty())
    {
        Vec3f up = Vec3f(0, 1, 0) * observer.getOrientation().toMatrix4();
        Vec3d vn = trackObject.getPosition(simTime) - observer.getPosition();
        Point3f to((float) vn.x, (float) vn.y, (float) vn.z);
        observer.setOrientation(lookAt(Point3f(0, 0, 0), to, up));
    }

    // Find the closest solar system
    closestSolarSystem = universe->getNearestSolarSystem(observer.getPosition());
}


Selection Simulation::getSelection() const
{
    return selection;
}


void Simulation::setSelection(const Selection& sel)
{
    selection = sel;
}


Selection Simulation::getTrackedObject() const
{
    return trackObject;
}


void Simulation::setTrackedObject(const Selection& sel)
{
    trackObject = sel;
}


Selection Simulation::pickObject(Vec3f pickRay, float tolerance)
{
    return universe->pick(observer.getPosition(),
                          pickRay * observer.getOrientation().toMatrix4(),
                          simTime,
                          faintestVisible,
                          tolerance);
}


Vec3d toUniversal(const Vec3d& v,
                  const Observer& observer,
                  const Selection& sel,
                  double t,
                  astro::CoordinateSystem frame)
{
    switch (frame)
    {
    case astro::ObserverLocal:
        {
            Quatf q = observer.getOrientation();
            Quatd qd(q.w, q.x, q.y, q.z);
            return v * qd.toMatrix3();
        }
        
    case astro::Geographic:
        if (sel.body == NULL)
            return v;
        else
            return v * sel.body->getGeographicToHeliocentric(t);
        
    case astro::Equatorial:
        if (sel.body == NULL)
            return v;
        else
            return v * sel.body->getLocalToHeliocentric(t);

    case astro::Ecliptical:
        // TODO: Multiply this by the planetary system's ecliptic orientation,
        // once this field is added.
        return v;

    case astro::Universal:
        return v;

    default:
        // assert(0);
        return v;
    }
}


struct TravelExpFunc : public unary_function<double, double>
{
    double dist, s;

    TravelExpFunc(double d, double _s) : dist(d), s(_s) {};

    double operator()(double x) const
    {
        // return (1.0 / x) * (exp(x / 2.0) - 1.0) - 0.5 - dist / 2.0;
        return exp(x * s) * (x * (1 - s) + 1) - 1 - dist;
    }
};


void Simulation::computeGotoParameters(Selection& destination,
                                       JourneyParams& jparams,
                                       double gotoTime,
                                       Vec3d offset,
                                       astro::CoordinateSystem offsetFrame,
                                       Vec3f up,
                                       astro::CoordinateSystem upFrame)
{
    UniversalCoord targetPosition = selection.getPosition(simTime);
    Vec3d v = targetPosition - observer.getPosition();
    v.normalize();

    jparams.duration = gotoTime;
    jparams.startTime = realTime;

    // Right where we are now . . .
    jparams.from = observer.getPosition();

    // The destination position lies along the line between the current
    // position and the star
    offset = toUniversal(offset, observer, selection, simTime, offsetFrame);
    jparams.to = targetPosition + offset;
    jparams.initialFocus = jparams.from +
        (Vec3f(0, 0, -1.0f) * observer.getOrientation().toMatrix4());
    jparams.finalFocus = targetPosition;

    Vec3d upd(up.x, up.y, up.z);
    upd = toUniversal(upd, observer, selection, simTime, upFrame);
    jparams.up = Vec3f((float) upd.x, (float) upd.y, (float) upd.z);

    jparams.initialOrientation = observer.getOrientation();
    Vec3d vn = targetPosition - jparams.to;
    Point3f focus((float) vn.x, (float) vn.y, (float) vn.z);
    jparams.finalOrientation = lookAt(Point3f(0, 0, 0), focus, jparams.up);

    jparams.accelTime = 0.5;
    double distance = astro::microLightYearsToKilometers(jparams.from.distanceTo(jparams.to)) / 2.0;
    pair<double, double> sol = solve_bisection(TravelExpFunc(distance, jparams.accelTime),
                                               0.0001, 100.0,
                                               1e-10);
    jparams.expFactor = sol.first;

    setFrame(frame.coordSys, destination);

    // Convert to frame coordinates
    RigidTransform from(jparams.from, jparams.initialOrientation);
    from = fromUniversal(frame, from, simTime);
    jparams.from = from.translation;
    jparams.initialOrientation= Quatf((float) from.rotation.w, (float) from.rotation.x,
                                      (float) from.rotation.y, (float) from.rotation.z);
    RigidTransform to(jparams.to, jparams.finalOrientation);
    to = fromUniversal(frame, to, simTime);
    jparams.to = to.translation;
    jparams.finalOrientation= Quatf((float) to.rotation.w, (float) to.rotation.x,
                                    (float) to.rotation.y, (float) to.rotation.z);
}


void Simulation::computeCenterParameters(Selection& destination,
                                         JourneyParams& jparams,
                                         double centerTime)
{
    UniversalCoord targetPosition = selection.getPosition(simTime);

    jparams.duration = centerTime;
    jparams.startTime = realTime;

    // Don't move through space, just rotate the camera
    jparams.from = observer.getPosition();
    jparams.to = jparams.from;

    jparams.initialFocus = jparams.from +
        (Vec3f(0, 0, -1.0f) * observer.getOrientation().toMatrix4());
    jparams.finalFocus = targetPosition;
    jparams.up = Vec3f(0, 1, 0) * observer.getOrientation().toMatrix4();

    jparams.initialOrientation = observer.getOrientation();
    Vec3d vn = targetPosition - jparams.to;
    Point3f focus((float) vn.x, (float) vn.y, (float) vn.z);
    jparams.finalOrientation = lookAt(Point3f(0, 0, 0), focus, jparams.up);

    jparams.accelTime = 0.5;
    jparams.expFactor = 0;

    // Convert to frame coordinates
    RigidTransform from(jparams.from, jparams.initialOrientation);
    from = fromUniversal(frame, from, simTime);
    jparams.from = from.translation;
    jparams.initialOrientation= Quatf((float) from.rotation.w, (float) from.rotation.x,
                                      (float) from.rotation.y, (float) from.rotation.z);
    RigidTransform to(jparams.to, jparams.finalOrientation);
    to = fromUniversal(frame, to, simTime);
    jparams.to = to.translation;
    jparams.finalOrientation= Quatf((float) to.rotation.w, (float) to.rotation.x,
                                    (float) to.rotation.y, (float) to.rotation.z);
}

Observer& Simulation::getObserver()
{
    return observer;
}


void Simulation::setObserverPosition(const UniversalCoord& pos)
{
    RigidTransform rt(pos, observer.getOrientation());
    transform = fromUniversal(frame, rt, simTime);
    observer.setPosition(pos);
}

void Simulation::setObserverOrientation(const Quatf& orientation)
{
    RigidTransform rt(observer.getPosition(), orientation);
    transform = fromUniversal(frame, rt, simTime);
    observer.setOrientation(orientation);
}


Simulation::ObserverMode Simulation::getObserverMode() const
{
    return observerMode;
}

void Simulation::setObserverMode(Simulation::ObserverMode mode)
{
    observerMode = mode;
}

void Simulation::setFrame(astro::CoordinateSystem coordSys,
                          const Selection& sel)
{
    frame = FrameOfReference(coordSys, sel);

    // Set the orientation and position in frame coordinates
    transform = fromUniversal(frame,
                              RigidTransform(observer.getPosition(), observer.getOrientation()),
                              simTime);
}

FrameOfReference Simulation::getFrame() const
{
    return frame;
}

// Rotate the observer about its center.
void Simulation::rotate(Quatf q)
{
    Quatd qd(q.w, q.x, q.y, q.z);
    transform.rotation = qd * transform.rotation;
    updateObserver();
}

// Orbit around the selection (if there is one.)  This involves changing
// both the observer's position and orientation.
void Simulation::orbit(Quatf q)
{
    if (!selection.empty())
    {
        // Before orbiting, make sure that the reference object matches the
        // selection.
        if (selection != frame.refObject)
            setFrame(frame.coordSys, selection);

        // Get the focus position (center of rotation) in frame
        // coordinates; in order to make this function work in all
        // frames of reference, it's important to work in frame
        // coordinates.
        UniversalCoord focusPosition = selection.getPosition(simTime);
        focusPosition = fromUniversal(frame, RigidTransform(focusPosition), simTime).translation;

        // v = the vector from the observer's position to the focus
        Vec3d v = transform.translation - focusPosition;

        // Get a double precision version of the rotation
        Quatd qd(q.w, q.x, q.y, q.z);

        // To give the right feel for rotation, we want to premultiply
        // the current orientation by q.  However, because of the order in
        // which we apply transformations later on, we can't pre-multiply.
        // To get around this, we compute a rotation q2 such
        // that q1 * r = r * q2.
        Quatd qd2 = ~transform.rotation * qd * transform.rotation;
        qd2.normalize();

        // Roundoff errors will accumulate and cause the distance between
        // viewer and focus to drift unless we take steps to keep the
        // length of v constant.
        double distance = v.length();
        v = v * qd2.toMatrix3();
        v.normalize();
        v *= distance;

        transform.rotation = transform.rotation * qd2;
        transform.translation = focusPosition + v;

        updateObserver();
    }
}


// Exponential camera dolly--move toward or away from the selected object
// at a rate dependent on the observer's distance from the object.
void Simulation::changeOrbitDistance(float d)
{
    if (!selection.empty())
    {
        // Before orbiting, make sure that the reference object matches the
        // selection.
        if (selection != frame.refObject)
            setFrame(frame.coordSys, selection);

        UniversalCoord focusPosition = selection.getPosition(simTime);
        
        double size = selection.radius();

        // Somewhat arbitrary parameters to chosen to give the camera movement
        // a nice feel.  They should probably be function parameters.
        double minOrbitDistance = astro::kilometersToMicroLightYears(size);
        double naturalOrbitDistance = astro::kilometersToMicroLightYears(4.0 * size);

        // Determine distance and direction to the selected object
        Vec3d v = observer.getPosition() - focusPosition;
        double currentDistance = v.length();

        // TODO: This is sketchy . . .
        if (currentDistance < minOrbitDistance)
            minOrbitDistance = currentDistance * 0.5;

        if (currentDistance >= minOrbitDistance && naturalOrbitDistance != 0)
        {
            double r = (currentDistance - minOrbitDistance) / naturalOrbitDistance;
            double newDistance = minOrbitDistance + naturalOrbitDistance * exp(log(r) + d);
            v = v * (newDistance / currentDistance);
            RigidTransform framePos = fromUniversal(frame,
                                                    RigidTransform(focusPosition + v),
                                                    simTime);
            transform.translation = framePos.translation;
        }

        updateObserver();
    }
}


void Simulation::setTargetSpeed(float s)
{
    targetSpeed = s;
    Vec3f v;

    if (trackObject.empty())
    {
        trackingOrientation = observer.getOrientation();
        // Generate vector for velocity using current orientation
        // and specified speed.
        v = Vec3f(0, 0, -s) * observer.getOrientation().toMatrix4();
    }
    else
    {
        // Use tracking orientation vector to generate target velocity
        v = Vec3f(0, 0, -s) * trackingOrientation.toMatrix4();
    }

    targetVelocity = Vec3d(v.x, v.y, v.z);
    initialVelocity = observer.getVelocity();
    beginAccelTime = realTime;
}

float Simulation::getTargetSpeed()
{
    return targetSpeed;
}

void Simulation::gotoSelection(double gotoTime, 
                               Vec3f up,
                               astro::CoordinateSystem upFrame)
{
    if (!selection.empty())
    {
        UniversalCoord pos = selection.getPosition(simTime);
        Vec3d v = pos - observer.getPosition();
        double distance = v.length();

        double maxOrbitDistance;
        if (selection.body != NULL)
            maxOrbitDistance = astro::kilometersToMicroLightYears(5.0f * selection.body->getRadius());
        else if (selection.galaxy != NULL)
            maxOrbitDistance = 5.0f * selection.galaxy->getRadius() * 1e6f;
        else if (selection.star != NULL)
            maxOrbitDistance = astro::kilometersToMicroLightYears(100.0f * selection.star->getRadius());
        else
            maxOrbitDistance = 0.5f;

        double radius = selection.radius();
        double minOrbitDistance = astro::kilometersToMicroLightYears(1.01 * radius);

        double orbitDistance = (distance > maxOrbitDistance * 10.0f) ? maxOrbitDistance : distance * 0.1f;
        if (orbitDistance < minOrbitDistance)
            orbitDistance = minOrbitDistance;

        computeGotoParameters(selection, journey, gotoTime,
                              v * -(orbitDistance / distance), astro::Universal,
                              up, upFrame);
        observerMode = Travelling;
    }
}

void Simulation::gotoSelection(double gotoTime,
                               double distance,
                               Vec3f up,
                               astro::CoordinateSystem upFrame)
{
    if (!selection.empty())
    {
        UniversalCoord pos = selection.getPosition(simTime);
        Vec3d v = pos - observer.getPosition();
        v.normalize();

        computeGotoParameters(selection, journey, gotoTime,
                              v * -distance * 1e6, astro::Universal,
                              up, upFrame);
        observerMode = Travelling;
    }
}

void Simulation::gotoSelectionLongLat(double gotoTime,
                                      double distance,
                                      float longitude,
                                      float latitude,
                                      Vec3f up)
{
    if (!selection.empty())
    {
        double phi = -latitude + PI / 2;
        double theta = longitude - PI;
        double x = cos(theta) * sin(phi);
        double y = cos(phi);
        double z = -sin(theta) * sin(phi);
        computeGotoParameters(selection, journey, gotoTime,
                              Vec3d(x, y, z) * distance * 1e6, astro::Geographic,
                              up, astro::Geographic);
        observerMode = Travelling;
    }
}

void Simulation::getSelectionLongLat(double& distance,
                                     double& longitude,
                                     double& latitude)
{
    // Compute distance (km) and lat/long (degrees) of observer with
    // respect to currently selected object.
    if (!selection.empty())
    {
        FrameOfReference refFrame(astro::Geographic, selection.body);
        RigidTransform xform = fromUniversal(refFrame,
                                             RigidTransform(observer.getPosition(), observer.getOrientation()),
                                             simTime);

        Point3d pos = (Point3d) xform.translation;

        distance = pos.distanceFromOrigin();
        longitude = -radToDeg(atan2(-pos.z, -pos.x));
        latitude = radToDeg(PI/2 - acos(pos.y / distance));

        // Convert distance from light years to kilometers.
        distance = astro::microLightYearsToKilometers(distance);
    }
}

void Simulation::cancelMotion()
{
    observerMode = Free;
}

void Simulation::centerSelection(double centerTime)
{
    if (!selection.empty())
    {
        computeCenterParameters(selection, journey, centerTime);
        observerMode = Travelling;
    }
}

void Simulation::follow()
{
    if (selection.body != NULL)
    {
        frame = FrameOfReference(astro::Ecliptical, selection.body);

        transform = fromUniversal(frame,
                                  RigidTransform(observer.getPosition(), observer.getOrientation()),
                                  simTime);
    }
}

void Simulation::geosynchronousFollow()
{
    if (selection.body != NULL)
    {
        frame = FrameOfReference(astro::Geographic, selection.body);
        transform = fromUniversal(frame,
                                  RigidTransform(observer.getPosition(), observer.getOrientation()),
                                  simTime);
    }
}

void Simulation::phaseLock()
{
    
    if (frame.refObject.body != NULL)
    {
        if (selection == frame.refObject)
        {
            frame = FrameOfReference(astro::PhaseLock, selection,
                                     Selection(selection.body->getSystem()->getStar()));
        }
        else
        {
            frame = FrameOfReference(astro::PhaseLock, frame.refObject, selection);
        }
        transform = fromUniversal(frame,
                                  RigidTransform(observer.getPosition(), observer.getOrientation()),
                                  simTime);
    }
}

void Simulation::chase()
{
    if (selection.body != NULL)
    {
        frame = FrameOfReference(astro::Chase, selection.body);
        transform = fromUniversal(frame,
                                  RigidTransform(observer.getPosition(), observer.getOrientation()),
                                  simTime);
    }
}


void Simulation::selectStar(uint32 catalogNo)
{
    StarDatabase* stardb = universe->getStarCatalog();
    if (stardb != NULL)
        selection = Selection(stardb->find(catalogNo));
}


// Choose a planet around a star given it's index in the planetary system.
// The planetary system is either the system of the selected object, or the
// nearest planetary system if no object is selected.  If index is less than
// zero, pick the star.  This function should probably be in celestiacore.cpp.
void Simulation::selectPlanet(int index)
{
    if (index < 0)
    {
        if (selection.body != NULL)
        {
            PlanetarySystem* system = selection.body->getSystem();
            if (system != NULL)
                selectStar(system->getStar()->getCatalogNumber());
        }
    }
    else
    {
        const Star* star = NULL;
        if (selection.star != NULL)
            star = selection.star;
        else if (selection.body != NULL)
            star = getSun(selection.body);

        SolarSystem* solarSystem = NULL;
        if (star != NULL)
            solarSystem = universe->getSolarSystem(star);
        else
            solarSystem = closestSolarSystem;

        if (solarSystem != NULL &&
            index < solarSystem->getPlanets()->getSystemSize())
        {
            selection = Selection(solarSystem->getPlanets()->getBody(index));
        }
    }
}


// Select an object by name, with the following priority:
//   1. Try to look up the name in the star database
//   2. Search the galaxy catalog for a matching name.
//   3. Search the planets and moons in the planetary system of the currently selected
//      star
//   4. Search the planets and moons in any 'nearby' (< 0.1 ly) planetary systems
Selection Simulation::findObject(string s)
{
    PlanetarySystem* path[2];
    int nPathEntries = 0;

    if (selection.star != NULL)
    {
        SolarSystem* sys = universe->getSolarSystem(selection.star);
        if (sys != NULL)
            path[nPathEntries++] = sys->getPlanets();
    }
    else if (selection.body != NULL)
    {
        PlanetarySystem* sys = selection.body->getSystem();
        while (sys != NULL && sys->getPrimaryBody() != NULL)
            sys = sys->getPrimaryBody()->getSystem();
        path[nPathEntries++] = sys;
    }

    if (closestSolarSystem != NULL)
        path[nPathEntries++] = closestSolarSystem->getPlanets();

    return universe->find(s, path, nPathEntries);
}


// Find an object from a path, for example Sol/Earth/Moon or Upsilon And/b
// Currently, 'absolute' paths starting with a / are not supported nor are
// paths that contain galaxies.
Selection Simulation::findObjectFromPath(string s)
{
    PlanetarySystem* path[2];
    int nPathEntries = 0;

    if (selection.star != NULL)
    {
        SolarSystem* sys = universe->getSolarSystem(selection.star);
        if (sys != NULL)
            path[nPathEntries++] = sys->getPlanets();
    }
    else if (selection.body != NULL)
    {
        PlanetarySystem* sys = selection.body->getSystem();
        while (sys != NULL && sys->getPrimaryBody() != NULL)
            sys = sys->getPrimaryBody()->getSystem();
        path[nPathEntries++] = sys;
    }

    if (closestSolarSystem != NULL)
        path[nPathEntries++] = closestSolarSystem->getPlanets();

    return universe->findPath(s, path, nPathEntries);
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
}


SolarSystem* Simulation::getNearestSolarSystem() const
{
    return closestSolarSystem;
}
