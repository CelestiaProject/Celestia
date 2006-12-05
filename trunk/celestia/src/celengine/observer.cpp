// observer.cpp
//
// Copyright (C) 2001-2006, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celmath/mathlib.h>
#include <celmath/solve.h>
#include "observer.h"
#include "simulation.h"

using namespace std;


#define LY 9466411842000.000
#define VELOCITY_CHANGE_TIME      0.25f


static Vec3d slerp(double t, const Vec3d& v0, const Vec3d& v1)
{
    double r0 = v0.length();
    double r1 = v1.length();
    Vec3d u = v0 / r0;
    Vec3d n = u ^ (v1 / r1);
    n.normalize();
    Vec3d v = n ^ u;
    if (v * v1 < 0.0)
        v = -v;

    double cosTheta = u * (v1 / r1);
    double theta = acos(cosTheta);

    return (cos(theta * t) * u + sin(theta * t) * v) * Mathd::lerp(t, r0, r1);
}


Observer::Observer() :
    simTime(0.0),
    velocity(0.0, 0.0, 0.0),
    angularVelocity(0.0f, 0.0f, 0.0f),
    realTime(0.0),
    targetSpeed(0.0),
    targetVelocity(0.0, 0.0, 0.0),
    initialVelocity(0.0, 0.0, 0.0),
    beginAccelTime(0.0),
    observerMode(Free),
    trackingOrientation(1.0f, 0.0f, 0.0f, 0.0f),
    fov((float) (PI / 4.0)),
    reverseFlag(false),
    locationFilter(~0u)
{
}


double Observer::getTime() const
{
    return simTime;
};

void Observer::setTime(double jd)
{
    simTime = jd;
}


UniversalCoord Observer::getPosition() const
{
    // TODO: Optimize this!  Dirty bit should be set by Simulation::setTime and
    // by Observer::update(), Observer::setOrientation(), Observer::setPosition()
    return frame.toUniversal(situation, getTime()).translation;
}


Point3d Observer::getRelativePosition(const Point3d& p) const
{
    BigFix x(p.x);
    BigFix y(p.y);
    BigFix z(p.z);

    UniversalCoord position = getPosition();
    double dx = (double) (position.x - x);
    double dy = (double) (position.y - y);
    double dz = (double) (position.z - z);

    return Point3d(dx / LY, dy / LY, dz / LY);
}


Quatf Observer::getOrientation() const
{
    Quatd q = frame.toUniversal(situation, getTime()).rotation;
    return Quatf((float) q.w, (float) q.x, (float) q.y, (float) q.z);
}


void Observer::setOrientation(const Quatf& q)
{
    RigidTransform rt = frame.toUniversal(situation, getTime());
    rt.rotation = Quatd(q.w, q.x, q.y, q.z);
    situation = frame.fromUniversal(rt, getTime());
}


void Observer::setOrientation(const Quatd& q)
{
    setOrientation(Quatf((float) q.w, (float) q.x, (float) q.y, (float) q.z));
}


Vec3d Observer::getVelocity() const
{
    return velocity;
}


void Observer::setVelocity(const Vec3d& v)
{
    velocity = v;
}


Vec3f Observer::getAngularVelocity() const
{
    return angularVelocity;
}


void Observer::setAngularVelocity(const Vec3f& v)
{
    angularVelocity = v;
}


void Observer::setPosition(const Point3d& p)
{
    setPosition(UniversalCoord(p));
}


void Observer::setPosition(const UniversalCoord& p)
{
    RigidTransform rt = frame.toUniversal(situation, getTime());
    rt.translation = p;
    situation = frame.fromUniversal(rt, getTime());
}


RigidTransform Observer::getSituation() const
{
    return frame.toUniversal(situation, getTime());
}



void Observer::setSituation(const RigidTransform& xform)
{
    situation = frame.fromUniversal(xform, getTime());
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
        if (sel.getType() != Selection::Type_Body)
            return v;
        else
            return v * sel.body()->getBodyFixedToHeliocentric(t);

    case astro::Equatorial:
        if (sel.getType() != Selection::Type_Body)
            return v;
        else
            return v * sel.body()->getLocalToHeliocentric(t);

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


/*! Determine an orientation that will make the negative z-axis point from
 *  from the observer to the target, with the y-axis pointing in direction
 *  of the component of 'up' that is orthogonal to the z-axis.
 */
// TODO: This is a generally useful function that should be moved to
// the celmath package.
template<class T> static Quaternion<T> 
lookAt(Point3<T> from, Point3<T> to, Vector3<T> up)
{
    Vector3<T> n = to - from;
    n.normalize();
    Vector3<T> v = n ^ up;
    v.normalize();
    Vector3<T> u = v ^ n;

    return Quaternion<T>::matrixToQuaternion(Matrix3<T>(v, u, -n));
}


double Observer::getArrivalTime() const
{
    if (observerMode != Travelling)
        return realTime;
    else
        return journey.startTime + journey.duration;
}


// Tick the simulation by dt seconds
void Observer::update(double dt, double timeScale)
{
    realTime += dt;
    simTime += (dt / 86400.0) * timeScale;

    if (observerMode == Travelling)
    {
        // Compute the fraction of the trip that has elapsed; handle zero
        // durations correctly by skipping directly to the destination.
        float t = 1.0;
        if (journey.duration > 0)
            t = (float) clamp((realTime - journey.startTime) / journey.duration);

        Vec3d jv = journey.to - journey.from;
        UniversalCoord p;

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

            if (journey.traj == Linear)
            {
                Vec3d v = jv;
                if (v.length() == 0.0)
                {
                    p = journey.from;
                }
                else
                {
                    v.normalize();
                    if (t < 0.5)
                        p = journey.from + v * astro::kilometersToMicroLightYears(x);
                    else
                        p = journey.to - v * astro::kilometersToMicroLightYears(x);
                }
            }
            else if (journey.traj == GreatCircle)
            {
                Selection centerObj = frame.refObject;
                if (centerObj.body() != NULL)
                {
                    Body* body = centerObj.body();
                    if (body->getSystem())
                    {
                        if (body->getSystem()->getPrimaryBody() != NULL)
                            centerObj = Selection(body->getSystem()->getPrimaryBody());
                        else
                            centerObj = Selection(body->getSystem()->getStar());
                    }
                }

                UniversalCoord ufrom  = frame.toUniversal(RigidTransform(journey.from, Quatf(1.0f)), simTime).translation;
                UniversalCoord uto    = frame.toUniversal(RigidTransform(journey.to, Quatf(1.0f)), simTime).translation;
                UniversalCoord origin = centerObj.getPosition(simTime);
                Vec3d v0 = ufrom - origin;
                Vec3d v1 = uto - origin;

                if (jv.length() == 0.0)
                {
                    p = journey.from;
                }
                else
                {
                    x = astro::kilometersToMicroLightYears(x / jv.length());
                    Vec3d v;

                    if (t < 0.5)
                        v = slerp(x, v0, v1);
                    else
                        v = slerp(x, v1, v0);

                    p = origin + v;
                    p = frame.fromUniversal(RigidTransform(p, Quatf(1.0f)), simTime).translation;
                }
            }
            else if (journey.traj == CircularOrbit)
            {
                Selection centerObj = frame.refObject;

                UniversalCoord ufrom  = frame.toUniversal(RigidTransform(journey.from, Quatf(1.0f)), simTime).translation;
                UniversalCoord uto    = frame.toUniversal(RigidTransform(journey.to, Quatf(1.0f)), simTime).translation;
                UniversalCoord origin = centerObj.getPosition(simTime);
                Vec3d v0 = ufrom - origin;
                Vec3d v1 = uto - origin;

                if (jv.length() == 0.0)
                {
                    p = journey.from;
                }
                else
                {
                    //x = astro::kilometersToMicroLightYears(x / jv.length());
                    Quatd q0(1.0);
                    Quatd q1(journey.rotation1.w, journey.rotation1.x,
                             journey.rotation1.y, journey.rotation1.z);
                    p = origin + v0 * Quatd::slerp(q0, q1, t).toMatrix3();

                    p = frame.fromUniversal(RigidTransform(p, Quatf(1.0f)),
                                            simTime).translation;
                }
            }
        }

        // Spherically interpolate the orientation over the first half
        // of the journey.
        Quatf orientation;
        if (t >= journey.startInterpolation && t < journey.endInterpolation )
        {
            // Smooth out the interpolation to avoid jarring changes in
            // orientation
            double v;
            if (journey.traj == CircularOrbit)
            {
                // In circular orbit mode, interpolation of orientation must
                // match the interpolation of position.
                v = t;
            }
            else
            {
                v = pow(sin((t - journey.startInterpolation) /
                            (journey.endInterpolation - journey.startInterpolation) * PI / 2), 2);
            }

            // Be careful to choose the shortest path when interpolating
            if (norm(journey.initialOrientation - journey.finalOrientation) <
                norm(journey.initialOrientation + journey.finalOrientation))
            {
                orientation = Quatf::slerp(journey.initialOrientation,
                                           journey.finalOrientation, (float)v);
            }
            else
            {
                orientation = Quatf::slerp(journey.initialOrientation,
                                           -journey.finalOrientation,(float)v);
            }
        }
        else if (t < journey.startInterpolation)
        {
            orientation = journey.initialOrientation;
        }
        else // t >= endInterpolation
        {
            orientation = journey.finalOrientation;
        }

        situation = RigidTransform(p, orientation);

        // If the journey's complete, reset to manual control
        if (t == 1.0f)
        {
            if (journey.traj != CircularOrbit)
                situation = RigidTransform(journey.to, journey.finalOrientation);
            observerMode = Free;
            setVelocity(Vec3d(0, 0, 0));
//            targetVelocity = Vec3d(0, 0, 0);
        }
    }

    if (getVelocity() != targetVelocity)
    {
        double t = clamp((realTime - beginAccelTime) / VELOCITY_CHANGE_TIME);
        Vec3d v = getVelocity() * (1.0 - t) + targetVelocity * t;

        // At some threshold, we just set the velocity to zero; otherwise,
        // we'll end up with ridiculous velocities like 10^-40 m/s.
        if (v.length() < 1.0e-12)
            v = Vec3d(0.0, 0.0, 0.0);
        setVelocity(v);
    }

    // Update the position
    situation.translation = situation.translation + getVelocity() * dt;

    if (observerMode == Free)
    {
        // Update the observer's orientation
        Vec3f fAV = getAngularVelocity();
        Vec3d AV(fAV.x, fAV.y, fAV.z);
        Quatd dr = 0.5 * (AV * situation.rotation);
        situation.rotation += dt * dr;
        situation.rotation.normalize();
    }

    if (!trackObject.empty())
    {
        Vec3f up = Vec3f(0, 1, 0) * getOrientation().toMatrix3();
        Vec3d viewDir = trackObject.getPosition(getTime()) - getPosition();
        
        setOrientation(lookAt(Point3d(0, 0, 0),
                              Point3d(viewDir.x, viewDir.y, viewDir.z),
                              Vec3d(up.x, up.y, up.z)));
    }
}


Selection Observer::getTrackedObject() const
{
    return trackObject;
}


void Observer::setTrackedObject(const Selection& sel)
{
    trackObject = sel;
}


const string& Observer::getDisplayedSurface() const
{
    return displayedSurface;
}


void Observer::setDisplayedSurface(const string& surf)
{
    displayedSurface = surf;
}


uint32 Observer::getLocationFilter() const
{
    return locationFilter;
}


void Observer::setLocationFilter(uint32 _locationFilter)
{
    locationFilter = _locationFilter;
}


void Observer::reverseOrientation()
{
    Quatf q = getOrientation();
    q.yrotate((float) PI);
    setOrientation(q);
    reverseFlag = !reverseFlag;
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


void Observer::computeGotoParameters(const Selection& destination,
                                     JourneyParams& jparams,
                                     double gotoTime,
                                     double startInter,
                                     double endInter,
                                     Vec3d offset,
                                     astro::CoordinateSystem offsetFrame,
                                     Vec3f up,
                                     astro::CoordinateSystem upFrame)
{
    if (frame.coordSys == astro::PhaseLock)
    {
        setFrame(FrameOfReference(astro::Ecliptical, destination));
    }
    else
    {
        setFrame(FrameOfReference(frame.coordSys, destination));
    }

    UniversalCoord targetPosition = destination.getPosition(getTime());
    Vec3d v = targetPosition - getPosition();
    v.normalize();

    jparams.traj = Linear;
    jparams.duration = gotoTime;
    jparams.startTime = realTime;

    // Right where we are now . . .
    jparams.from = getPosition();

    offset = toUniversal(offset, *this, destination, getTime(), offsetFrame);
    jparams.to = targetPosition + offset;

    Vec3d upd(up.x, up.y, up.z);
    upd = toUniversal(upd, *this, destination, getTime(), upFrame);
    Vec3f upf = Vec3f((float) upd.x, (float) upd.y, (float) upd.z);

    jparams.initialOrientation = getOrientation();
    Vec3d vn = targetPosition - jparams.to;
    Point3f focus((float) vn.x, (float) vn.y, (float) vn.z);
    jparams.finalOrientation = lookAt(Point3f(0, 0, 0), focus, upf);
    jparams.startInterpolation = min(startInter, endInter);
    jparams.endInterpolation   = max(startInter, endInter);

    jparams.accelTime = 0.5;
    double distance = astro::microLightYearsToKilometers(jparams.from.distanceTo(jparams.to)) / 2.0;
    pair<double, double> sol = solve_bisection(TravelExpFunc(distance, jparams.accelTime),
                                               0.0001, 100.0,
                                               1e-10);
    jparams.expFactor = sol.first;

    // Convert to frame coordinates
    RigidTransform from(jparams.from, jparams.initialOrientation);
    from = frame.fromUniversal(from, getTime());
    jparams.from = from.translation;
    jparams.initialOrientation= Quatf((float) from.rotation.w,
                                      (float) from.rotation.x,
                                      (float) from.rotation.y,
                                      (float) from.rotation.z);
    RigidTransform to(jparams.to, jparams.finalOrientation);
    to = frame.fromUniversal(to, getTime());
    jparams.to = to.translation;
    jparams.finalOrientation= Quatf((float) to.rotation.w,
                                    (float) to.rotation.x,
                                    (float) to.rotation.y,
                                    (float) to.rotation.z);
}


void Observer::computeGotoParametersGC(const Selection& destination,
                                       JourneyParams& jparams,
                                       double gotoTime,
                                       double startInter,
                                       double endInter,
                                       Vec3d offset,
                                       astro::CoordinateSystem offsetFrame,
                                       Vec3f up,
                                       astro::CoordinateSystem upFrame,
                                       const Selection& centerObj)
{
    setFrame(FrameOfReference(frame.coordSys, destination));

    UniversalCoord targetPosition = destination.getPosition(getTime());
    Vec3d v = targetPosition - getPosition();
    v.normalize();

    jparams.traj = GreatCircle;
    jparams.duration = gotoTime;
    jparams.startTime = realTime;

    jparams.centerObject = centerObj;

    // Right where we are now . . .
    jparams.from = getPosition();

    offset = toUniversal(offset, *this, destination, getTime(), offsetFrame);
    jparams.to = targetPosition + offset;

    Vec3d upd(up.x, up.y, up.z);
    upd = toUniversal(upd, *this, destination, getTime(), upFrame);
    Vec3f upf = Vec3f((float) upd.x, (float) upd.y, (float) upd.z);

    jparams.initialOrientation = getOrientation();
    Vec3d vn = targetPosition - jparams.to;
    Point3f focus((float) vn.x, (float) vn.y, (float) vn.z);
    jparams.finalOrientation = lookAt(Point3f(0, 0, 0), focus, upf);
    jparams.startInterpolation = min(startInter, endInter);
    jparams.endInterpolation   = max(startInter, endInter);

    jparams.accelTime = 0.5;
    double distance = astro::microLightYearsToKilometers(jparams.from.distanceTo(jparams.to)) / 2.0;
    pair<double, double> sol = solve_bisection(TravelExpFunc(distance, jparams.accelTime),
                                               0.0001, 100.0,
                                               1e-10);
    jparams.expFactor = sol.first;

    // Convert to frame coordinates
    RigidTransform from(jparams.from, jparams.initialOrientation);
    from = frame.fromUniversal(from, getTime());
    jparams.from = from.translation;
    jparams.initialOrientation= Quatf((float) from.rotation.w,
                                      (float) from.rotation.x,
                                      (float) from.rotation.y,
                                      (float) from.rotation.z);
    RigidTransform to(jparams.to, jparams.finalOrientation);
    to = frame.fromUniversal(to, getTime());
    jparams.to = to.translation;
    jparams.finalOrientation= Quatf((float) to.rotation.w,
                                    (float) to.rotation.x,
                                    (float) to.rotation.y,
                                    (float) to.rotation.z);
}


void Observer::computeCenterParameters(const Selection& destination,
                                       JourneyParams& jparams,
                                       double centerTime)
{
    UniversalCoord targetPosition = destination.getPosition(getTime());

    jparams.duration = centerTime;
    jparams.startTime = realTime;
    jparams.traj = Linear;

    // Don't move through space, just rotate the camera
    jparams.from = getPosition();
    jparams.to = jparams.from;

    Vec3f up = Vec3f(0, 1, 0) * getOrientation().toMatrix4();

    jparams.initialOrientation = getOrientation();
    Vec3d vn = targetPosition - jparams.to;
    Point3f focus((float) vn.x, (float) vn.y, (float) vn.z);
    jparams.finalOrientation = lookAt(Point3f(0, 0, 0), focus, up);
    jparams.startInterpolation = 0;
    jparams.endInterpolation   = 1;


    jparams.accelTime = 0.5;
    jparams.expFactor = 0;

    // Convert to frame coordinates
    RigidTransform from(jparams.from, jparams.initialOrientation);
    from = frame.fromUniversal(from, getTime());
    jparams.from = from.translation;
    jparams.initialOrientation= Quatf((float) from.rotation.w,
                                      (float) from.rotation.x,
                                      (float) from.rotation.y,
                                      (float) from.rotation.z);

    RigidTransform to(jparams.to, jparams.finalOrientation);
    to = frame.fromUniversal(to, getTime());
    jparams.to = to.translation;
    jparams.finalOrientation= Quatf((float) to.rotation.w,
                                    (float) to.rotation.x,
                                    (float) to.rotation.y,
                                    (float) to.rotation.z);
}


void Observer::computeCenterCOParameters(const Selection& destination,
                                         JourneyParams& jparams,
                                         double centerTime)
{
    jparams.duration = centerTime;
    jparams.startTime = realTime;
    jparams.traj = CircularOrbit;

    jparams.centerObject = frame.refObject;
    jparams.expFactor = 0.5;

    Vec3d v = destination.getPosition(getTime()) - getPosition();
    Vec3f wf = Vec3f(0.0, 0.0, -1.0) * getOrientation().toMatrix3();
    Vec3d w(wf.x, wf.y, wf.z);
    v.normalize();

    Selection centerObj = frame.refObject;
    UniversalCoord centerPos = centerObj.getPosition(getTime());
    UniversalCoord targetPosition = destination.getPosition(getTime());

    Quatd qd = Quatd::vecToVecRotation(v, w);
    Quatf q((float) qd.w, (float) qd.x, (float) qd.y, (float) qd.z);

    jparams.from = getPosition();
    jparams.to = centerPos + ((getPosition() - centerPos) * qd.toMatrix3());
    jparams.initialOrientation = getOrientation();
    jparams.finalOrientation = getOrientation() * q;

    jparams.startInterpolation = 0.0;
    jparams.endInterpolation = 1.0;

    jparams.rotation1 = q;

    // Convert to frame coordinates
    RigidTransform from(jparams.from, jparams.initialOrientation);
    from = frame.fromUniversal(from, getTime());
    jparams.from = from.translation;
    jparams.initialOrientation= Quatf((float) from.rotation.w,
                                      (float) from.rotation.x,
                                      (float) from.rotation.y,
                                      (float) from.rotation.z);

    RigidTransform to(jparams.to, jparams.finalOrientation);
    to = frame.fromUniversal(to, getTime());
    jparams.to = to.translation;
    jparams.finalOrientation= Quatf((float) to.rotation.w,
                                    (float) to.rotation.x,
                                    (float) to.rotation.y,
                                    (float) to.rotation.z);
}


Observer::ObserverMode Observer::getMode() const
{
    return observerMode;
}


// Center the selection by moving on a circular orbit arround
// the primary body (refObject).
void Observer::centerSelectionCO(const Selection& selection, double centerTime)
{
    if (!selection.empty() && !frame.refObject.empty())
    {
        computeCenterCOParameters(selection, journey, centerTime);
        observerMode = Travelling;
    }
}


void Observer::setMode(Observer::ObserverMode mode)
{
    observerMode = mode;
}


void Observer::setFrame(const FrameOfReference& _frame)
{
    RigidTransform transform = frame.toUniversal(situation, getTime());
    if (observerMode == Travelling)
    {
        RigidTransform from = frame.toUniversal(RigidTransform(journey.from, journey.initialOrientation), getTime());
        RigidTransform to = frame.toUniversal(RigidTransform(journey.to, journey.finalOrientation), getTime());

        frame = _frame;
        situation = frame.fromUniversal(transform, getTime());

        from = frame.fromUniversal(from, getTime());
        journey.from = from.translation;
        journey.initialOrientation = Quatf((float) from.rotation.w,
                                           (float) from.rotation.x,
                                           (float) from.rotation.y,
                                           (float) from.rotation.z);
        to = frame.fromUniversal(to, getTime());
        journey.to = to.translation;
        journey.finalOrientation = Quatf((float) to.rotation.w,
                                         (float) to.rotation.x,
                                         (float) to.rotation.y,
                                         (float) to.rotation.z);
    }
    else
    {
        frame = _frame;
        situation = frame.fromUniversal(transform, getTime());
    }
}


FrameOfReference Observer::getFrame() const
{
    return frame;
}


// Rotate the observer about its center.
void Observer::rotate(Quatf q)
{
    Quatd qd(q.w, q.x, q.y, q.z);
    situation.rotation = qd * situation.rotation;
}


// Orbit around the reference object (if there is one.)  This involves changing
// both the observer's position and orientation.
void Observer::orbit(const Selection& selection, Quatf q)
{
    Selection center = frame.refObject;
    if (center.empty() && !selection.empty())
    {
        center = selection;
        setFrame(FrameOfReference(frame.coordSys, center));
    }

    if (!center.empty())
    {
        // Get the focus position (center of rotation) in frame
        // coordinates; in order to make this function work in all
        // frames of reference, it's important to work in frame
        // coordinates.
        UniversalCoord focusPosition = center.getPosition(getTime());
        focusPosition = frame.fromUniversal(RigidTransform(focusPosition), getTime()).translation;

        // v = the vector from the observer's position to the focus
        Vec3d v = situation.translation - focusPosition;

        // Get a double precision version of the rotation
        Quatd qd(q.w, q.x, q.y, q.z);

        // To give the right feel for rotation, we want to premultiply
        // the current orientation by q.  However, because of the order in
        // which we apply transformations later on, we can't pre-multiply.
        // To get around this, we compute a rotation q2 such
        // that q1 * r = r * q2.
        Quatd qd2 = ~situation.rotation * qd * situation.rotation;
        qd2.normalize();

        // Roundoff errors will accumulate and cause the distance between
        // viewer and focus to drift unless we take steps to keep the
        // length of v constant.
        double distance = v.length();
        v = v * qd2.toMatrix3();
        v.normalize();
        v *= distance;

        situation.rotation = situation.rotation * qd2;
        situation.translation = focusPosition + v;
    }
}


// Exponential camera dolly--move toward or away from the selected object
// at a rate dependent on the observer's distance from the object.
void Observer::changeOrbitDistance(const Selection& selection, float d)
{
    Selection center = frame.refObject;
    if (center.empty() && !selection.empty())
    {
        center = selection;
        setFrame(FrameOfReference(frame.coordSys, center));
    }

    if (!center.empty())
    {
        UniversalCoord focusPosition = center.getPosition(getTime());

        double size = center.radius();

        // Somewhat arbitrary parameters to chosen to give the camera movement
        // a nice feel.  They should probably be function parameters.
        double minOrbitDistance = astro::kilometersToMicroLightYears(size);
        double naturalOrbitDistance = astro::kilometersToMicroLightYears(4.0 * size);

        // Determine distance and direction to the selected object
        Vec3d v = getPosition() - focusPosition;
        double currentDistance = v.length();

        // TODO: This is sketchy . . .
        if (currentDistance < minOrbitDistance)
            minOrbitDistance = currentDistance * 0.5;

        if (currentDistance >= minOrbitDistance && naturalOrbitDistance != 0)
        {
            double r = (currentDistance - minOrbitDistance) / naturalOrbitDistance;
            double newDistance = minOrbitDistance + naturalOrbitDistance * exp(log(r) + d);
            v = v * (newDistance / currentDistance);
            RigidTransform framePos = frame.fromUniversal(RigidTransform(focusPosition + v),
                                                          getTime());
            situation.translation = framePos.translation;
        }
    }
}


void Observer::setTargetSpeed(float s)
{
    targetSpeed = s;
    Vec3f v;
    if (reverseFlag)
        s = -s;
    if (trackObject.empty())
    {
        trackingOrientation = getOrientation();
        // Generate vector for velocity using current orientation
        // and specified speed.
        v = Vec3f(0, 0, -s) * getOrientation().toMatrix4();
    }
    else
    {
        // Use tracking orientation vector to generate target velocity
        v = Vec3f(0, 0, -s) * trackingOrientation.toMatrix4();
    }

    targetVelocity = Vec3d(v.x, v.y, v.z);
    initialVelocity = getVelocity();
    beginAccelTime = realTime;
}


float Observer::getTargetSpeed()
{
    return (float) targetSpeed;
}


void Observer::gotoJourney(const JourneyParams& params)
{
    journey = params;
    double distance = astro::microLightYearsToKilometers(journey.from.distanceTo(journey.to)) / 2.0;
    pair<double, double> sol = solve_bisection(TravelExpFunc(distance, journey.accelTime),
                                               0.0001, 100.0,
                                               1e-10);
    journey.expFactor = sol.first;
    journey.startTime = realTime;
    observerMode = Travelling;
}

void Observer::gotoSelection(const Selection& selection,
                             double gotoTime,
                             Vec3f up,
                             astro::CoordinateSystem upFrame)
{
    gotoSelection(selection, gotoTime, 0.0, 0.5, up, upFrame);
}


// Return the preferred distance for viewing an object
static double getPreferredDistance(const Selection& selection)
{
    switch (selection.getType())
    {
    case Selection::Type_Body:
        return 5.0 * selection.radius();
    case Selection::Type_DeepSky:
        return 5.0 * selection.radius();
    case Selection::Type_Star:
        if (selection.star()->getVisibility())
            return 100.0 * selection.radius();
        else
            return astro::AUtoKilometers(1.0);
    case Selection::Type_Location:
        {
            double maxDist = getPreferredDistance(selection.location()->getParentBody());
            return max(min(selection.location()->getSize() * 50.0, maxDist),
                       1.0);
        }
    default:
        return 1.0;
    }
}


// Given an object and its current distance from the camera, determine how
// close we should go on the next goto.
static double getOrbitDistance(const Selection& selection,
                               double currentDistance)
{
    // If further than 10 times the preferrred distance, goto the
    // preferred distance.  If closer, zoom in 10 times closer or to the
    // minimum distance.
    double maxDist = astro::kilometersToMicroLightYears(getPreferredDistance(selection));
    double minDist = astro::kilometersToMicroLightYears(1.01 * selection.radius());
    double dist = (currentDistance > maxDist * 10.0) ? maxDist : currentDistance * 0.1;

    return max(dist, minDist);
}


void Observer::gotoSelection(const Selection& selection,
                             double gotoTime,
                             double startInter,
                             double endInter,
                             Vec3f up,
                             astro::CoordinateSystem upFrame)
{
    if (!selection.empty())
    {
        UniversalCoord pos = selection.getPosition(getTime());
        Vec3d v = pos - getPosition();
        double distance = v.length();

        double orbitDistance = getOrbitDistance(selection, distance);

        computeGotoParameters(selection, journey, gotoTime,
                              startInter, endInter,
                              v * -(orbitDistance / distance),
                              astro::Universal,
                              up, upFrame);
        observerMode = Travelling;
    }
}


// Like normal goto, except we'll follow a great circle trajectory.  Useful
// for travelling between surface locations, where we'd rather not go straight
// through the middle of a planet.
void Observer::gotoSelectionGC(const Selection& selection,
                               double gotoTime,
                               double /*startInter*/,       //TODO: remove parameter??
                               double /*endInter*/,         //TODO: remove parameter??
                               Vec3f up,
                               astro::CoordinateSystem upFrame)
{
    if (!selection.empty())
    {
        Selection centerObj = selection.parent();

        UniversalCoord pos = selection.getPosition(getTime());
        Vec3d v = pos - centerObj.getPosition(getTime());
        double distanceToCenter = v.length();
        Vec3d viewVec = pos - getPosition();
        double orbitDistance = getOrbitDistance(selection,
                                                viewVec.length());
        if (selection.location() != NULL)
        {
            Selection parent = selection.parent();
            double maintainDist = astro::kilometersToMicroLightYears(getPreferredDistance(parent));
            Vec3d parentPos = parent.getPosition(getTime()) - getPosition();
            double parentDist = parentPos.length() -
                astro::kilometersToMicroLightYears(parent.radius());

            if (parentDist <= maintainDist && parentDist > orbitDistance)
            {
                orbitDistance = parentDist;
            }
        }

        computeGotoParametersGC(selection, journey, gotoTime,
                                //startInter, endInter,
                                0.25, 0.75,
                                v * (orbitDistance / distanceToCenter),
                                astro::Universal,
                                up, upFrame,
                                centerObj);
        observerMode = Travelling;
    }
}


void Observer::gotoSelection(const Selection& selection,
                             double gotoTime,
                             double distance,
                             Vec3f up,
                             astro::CoordinateSystem upFrame)
{
    if (!selection.empty())
    {
        UniversalCoord pos = selection.getPosition(getTime());
        // The destination position lies along the line between the current
        // position and the star
        Vec3d v = pos - getPosition();
        v.normalize();

        computeGotoParameters(selection, journey, gotoTime, 0.25, 0.75,
                              v * -distance * 1e6, astro::Universal,
                              up, upFrame);
        observerMode = Travelling;
    }
}


void Observer::gotoSelectionGC(const Selection& selection,
                               double gotoTime,
                               double distance,
                               Vec3f up,
                               astro::CoordinateSystem upFrame)
{
    if (!selection.empty())
    {
        Selection centerObj = selection.parent();

        UniversalCoord pos = selection.getPosition(getTime());
        Vec3d v = pos - centerObj.getPosition(getTime());
        v.normalize();

        // The destination position lies along a line extended from the center
        // object to the target object
        computeGotoParametersGC(selection, journey, gotoTime, 0.25, 0.75,
                                v * -distance * 1e6, astro::Universal,
                                up, upFrame,
                                centerObj);
        observerMode = Travelling;
    }
}


void Observer::gotoSelectionLongLat(const Selection& selection,
                                    double gotoTime,
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
        computeGotoParameters(selection, journey, gotoTime, 0.25, 0.75,
                              Vec3d(x, y, z) * distance * 1e6, astro::Geographic,
                              up, astro::Geographic);
        observerMode = Travelling;
    }
}


void Observer::gotoLocation(const RigidTransform& transform,
                            double duration)
{
    journey.startTime = realTime;
    journey.duration = duration;

    RigidTransform from(getPosition(), getOrientation());
    from = frame.fromUniversal(from, getTime());
    journey.from = from.translation;
    journey.initialOrientation= Quatf((float) from.rotation.w, (float) from.rotation.x,
                                      (float) from.rotation.y, (float) from.rotation.z);

    journey.to = transform.translation;
    journey.finalOrientation = Quatf((float) transform.rotation.w,
                                     (float) transform.rotation.x,
                                     (float) transform.rotation.y,
                                     (float) transform.rotation.z);
    journey.startInterpolation = 0.25f;
    journey.endInterpolation   = 0.75f;


    journey.accelTime = 0.5;
    double distance = astro::microLightYearsToKilometers(journey.from.distanceTo(journey.to)) / 2.0;
    pair<double, double> sol = solve_bisection(TravelExpFunc(distance, journey.accelTime),
                                               0.0001, 100.0,
                                               1e-10);
    journey.expFactor = sol.first;

    observerMode = Travelling;
}


void Observer::getSelectionLongLat(const Selection& selection,
                                   double& distance,
                                   double& longitude,
                                   double& latitude)
{
    // Compute distance (km) and lat/long (degrees) of observer with
    // respect to currently selected object.
    if (!selection.empty())
    {
        FrameOfReference refFrame(astro::Geographic, selection);
        RigidTransform xform = refFrame.fromUniversal(RigidTransform(getPosition(), getOrientation()),
                                                      getTime());

        Point3d pos = (Point3d) xform.translation;

        distance = pos.distanceFromOrigin();
        longitude = -radToDeg(atan2(-pos.z, -pos.x));
        latitude = radToDeg(PI/2 - acos(pos.y / distance));

        // Convert distance from light years to kilometers.
        distance = astro::microLightYearsToKilometers(distance);
    }
}


void Observer::gotoSurface(const Selection& sel, double duration)
{
    Vec3d vd = getPosition() - sel.getPosition(getTime());
    Vec3f vf((float) vd.x, (float) vd.y, (float) vd.z);
    vf.normalize();
    Vec3f viewDir = Vec3f(0, 0, -1) * getOrientation().toMatrix3();
    Vec3f up = Vec3f(0, 1, 0) * getOrientation().toMatrix3();
    Quatf q = getOrientation();
    if (vf * viewDir < 0.0f)
    {
        q = lookAt(Point3f(0, 0, 0), Point3f(0.0f, 0.0f, 0.0f) + up, vf);
    }
    else
    {
    }

    FrameOfReference frame(astro::Geographic, sel);
    RigidTransform rt = frame.fromUniversal(RigidTransform(getPosition(), q),
                                            getTime());

    double height = 1.0001 * astro::kilometersToMicroLightYears(sel.radius());
    Vec3d dir = rt.translation - Point3d(0.0, 0.0, 0.0);
    dir.normalize();
    dir *= height;

    rt.translation = UniversalCoord(dir.x, dir.y, dir.z);
    gotoLocation(rt, duration);
};


void Observer::cancelMotion()
{
    observerMode = Free;
}


void Observer::centerSelection(const Selection& selection, double centerTime)
{
    if (!selection.empty())
    {
        computeCenterParameters(selection, journey, centerTime);
        observerMode = Travelling;
    }
}


void Observer::follow(const Selection& selection)
{
    if (!selection.empty())
    {
        setFrame(FrameOfReference(astro::Ecliptical, selection));
    }
}


void Observer::geosynchronousFollow(const Selection& selection)
{
    if (selection.body() != NULL ||
        selection.location() != NULL ||
        selection.star() != NULL)
    {
        setFrame(FrameOfReference(astro::Geographic, selection));
    }
}


void Observer::phaseLock(const Selection& selection)
{
    if (frame.refObject.body() != NULL)
    {
        if (selection == frame.refObject)
        {
            setFrame(FrameOfReference(astro::PhaseLock, selection,
                                      Selection(selection.body()->getSystem()->getStar())));
        }
        else
        {
            setFrame(FrameOfReference(astro::PhaseLock, frame.refObject, selection));
        }
    }
    else if (frame.refObject.star() != NULL)
    {
        if (selection != frame.refObject)
        {
            setFrame(FrameOfReference(astro::PhaseLock, frame.refObject, selection));
        }
    }
}

void Observer::chase(const Selection& selection)
{
    if (selection.body() != NULL)
    {
        setFrame(FrameOfReference(astro::Chase, selection));
    }
    else if (selection.star() != NULL)
    {
        setFrame(FrameOfReference(astro::Chase, selection));
    }
}


float Observer::getFOV() const
{
    return fov;
}


void Observer::setFOV(float _fov)
{
    fov = _fov;
}


Vec3f Observer::getPickRay(float x, float y) const
{
    float s = 2 * (float) tan(fov / 2.0);

    Vec3f pickDirection = Vec3f(x * s, y * s, -1.0f);
    pickDirection.normalize();

    return pickDirection;
}
