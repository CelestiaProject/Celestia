// observer.cpp
//
// Copyright (C) 2001-2008, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celmath/mathlib.h>
#include <celmath/solve.h>
#include "observer.h"
#include "simulation.h"
#include "frametree.h"

static const double maximumSimTime = 730486721060.00073; // 2000000000 Jan 01 12:00:00 UTC
static const double minimumSimTime = -730498278941.99951; // -2000000000 Jan 01 12:00:00 UTC

using namespace std;


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


/*! Notes on the Observer class
 *  The values position and orientation are in observer's reference frame. positionUniv
 *  and orientationUniv are the equivalent values in the universal coordinate system.
 *  They must be kept in sync. Generally, it's position and orientation that are modified;
 *  after they're changed, the method updateUniversal is called. However, when the observer
 *  frame is changed, positionUniv and orientationUniv are not changed, but the position
 *  and orientation within the frame /do/ change. Thus, a 'reverse' update is necessary.
 *
 *  There are two types of 'automatic' updates to position and orientation that may
 *  occur when the observer's update method is called: updates from free travel, and
 *  updates due to an active goto operation.
 */

Observer::Observer() :
    simTime(0.0),
    position(0.0, 0.0, 0.0),
    orientation(1.0),
    velocity(0.0, 0.0, 0.0),
    angularVelocity(0.0, 0.0, 0.0),
    frame(NULL),
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
    frame = new ObserverFrame();
    updateUniversal();
}


/*! Copy constructor. */
Observer::Observer(const Observer& o) :
	simTime(o.simTime),
	position(o.position),
	orientation(o.orientation),
	velocity(o.velocity),
	angularVelocity(o.angularVelocity),
	frame(NULL),
	realTime(o.realTime),
	targetSpeed(o.targetSpeed),
	targetVelocity(o.targetVelocity),
	beginAccelTime(o.beginAccelTime),
	observerMode(o.observerMode),
	journey(o.journey),
	trackObject(o.trackObject),
	trackingOrientation(o.trackingOrientation),
	fov(o.fov),
	reverseFlag(o.reverseFlag),
	locationFilter(o.locationFilter),
	displayedSurface(o.displayedSurface)
{
	frame = new ObserverFrame(*o.frame);
	updateUniversal();
}


Observer& Observer::operator=(const Observer& o)
{
	simTime = o.simTime;
	position = o.position;
	orientation = o.orientation;
	velocity = o.velocity;
	angularVelocity = o.angularVelocity;
	frame = NULL;
	realTime = o.realTime;
	targetSpeed = o.targetSpeed;
	targetVelocity = o.targetVelocity;
	beginAccelTime = o.beginAccelTime;
	observerMode = o.observerMode;
	journey = o.journey;
	trackObject = o.trackObject;
	trackingOrientation = o.trackingOrientation;
	fov = o.fov;
	reverseFlag = o.reverseFlag;
	locationFilter = o.locationFilter;
	displayedSurface = o.displayedSurface;

	setFrame(*o.frame);
	updateUniversal();

	return *this;
}


/*! Get the current simulation time. The time returned is a Julian date,
 *  and the time standard is TDB.
 */
double Observer::getTime() const
{
    return simTime;
};


/*! Get the current real time. The time returned is a Julian date,
 *  and the time standard is TDB.
 */
double Observer::getRealTime() const
{
    return realTime;
};


/*! Set the simulation time (Julian date, TDB time standard)
*/
void Observer::setTime(double jd)
{
    simTime = jd;
	updateUniversal();
}


/*! Return the position of the observer in universal coordinates. The origin
 *  The origin of this coordinate system is the Solar System Barycenter, and
 *  axes are defined by the J2000 ecliptic and equinox.
 */
UniversalCoord Observer::getPosition() const
{
    return positionUniv;
}


// TODO: Low-precision set position that should be removed
void Observer::setPosition(const Point3d& p)
{
    setPosition(UniversalCoord(p));
}


/*! Set the position of the observer; position is specified in the universal
 *  coordinate system.
 */
void Observer::setPosition(const UniversalCoord& p)
{
    positionUniv = p;
    position = frame->convertFromUniversal(p, getTime());
}


/*! Return the orientation of the observer in the universal coordinate
 *  system.
 */
Quatd Observer::getOrientation() const
{
    return orientationUniv;
}

/*! Reduced precision version of getOrientation()
 */
Quatf Observer::getOrientationf() const
{
    Quatd q = getOrientation();
    return Quatf((float) q.w, (float) q.x, (float) q.y, (float) q.z);
}


/* Set the orientation of the observer. The orientation is specified in
 * the universal coordinate system.
 */
void Observer::setOrientation(const Quatf& q)
{
    /*
    RigidTransform rt = frame.toUniversal(situation, getTime());
    rt.rotation = Quatd(q.w, q.x, q.y, q.z);
    situation = frame.fromUniversal(rt, getTime());
     */
    setOrientation(Quatd(q.w, q.x, q.y, q.z));
}


/*! Set the orientation of the observer. The orientation is specified in
 *  the universal coordinate system.
 */
void Observer::setOrientation(const Quatd& q)
{
    orientationUniv = q;
    orientation = frame->convertFromUniversal(q, getTime());
}


/*! Get the velocity of the observer within the observer's reference frame.
 */
Vec3d Observer::getVelocity() const
{
    return velocity;
}


/*! Set the velocity of the observer within the observer's reference frame.
*/
void Observer::setVelocity(const Vec3d& v)
{
    velocity = v;
}


Vec3d Observer::getAngularVelocity() const
{
    return angularVelocity;
}


void Observer::setAngularVelocity(const Vec3d& v)
{
    angularVelocity = v;
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


/*! Tick the simulation by dt seconds. Update the observer position
 *  and orientation due to an active goto command or non-zero velocity
 *  or angular velocity.
 */
void Observer::update(double dt, double timeScale)
{
    realTime += dt;
    simTime += (dt / 86400.0) * timeScale;

    if (simTime >= maximumSimTime)
        simTime = maximumSimTime;
    if (simTime <= minimumSimTime)
        simTime = minimumSimTime;

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
                Selection centerObj = frame->getRefObject();
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

                UniversalCoord ufrom  = frame->convertToUniversal(journey.from, simTime);
                UniversalCoord uto    = frame->convertToUniversal(journey.to, simTime);
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

                    p = frame->convertFromUniversal(origin + v, simTime);
                }
            }
            else if (journey.traj == CircularOrbit)
            {
                Selection centerObj = frame->getRefObject();

                UniversalCoord ufrom = frame->convertToUniversal(journey.from, simTime);
                UniversalCoord uto   = frame->convertToUniversal(journey.to, simTime);
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
                    p = frame->convertFromUniversal(p, simTime);
                }
            }
        }

        // Spherically interpolate the orientation over the first half
        // of the journey.
        Quatd q;
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
                q = Quatd::slerp(journey.initialOrientation,
                                 journey.finalOrientation, v);
            }
            else
            {
                q = Quatd::slerp(journey.initialOrientation,
                                -journey.finalOrientation, v);
            }
        }
        else if (t < journey.startInterpolation)
        {
            q = journey.initialOrientation;
        }
        else // t >= endInterpolation
        {
            q = journey.finalOrientation;
        }

        position = p;
        orientation = q;
        
        // If the journey's complete, reset to manual control
        if (t == 1.0f)
        {
            if (journey.traj != CircularOrbit)
            {
                //situation = RigidTransform(journey.to, journey.finalOrientation);
                position = journey.to;
                orientation = journey.finalOrientation;
            }
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
    position = position + getVelocity() * dt;

    if (observerMode == Free)
    {
        // Update the observer's orientation
        Vec3d AV = getAngularVelocity();
        Quatd dr = 0.5 * (AV * orientation);
        orientation += dt * dr;
        orientation.normalize();
    }
   
    updateUniversal();

    // Update orientation for tracking--must occur after updateUniversal(), as it
    // relies on the universal position and orientation of the observer.
    if (!trackObject.empty())
    {
        Vec3d up = Vec3d(0, 1, 0) * getOrientation().toMatrix3();
        Vec3d viewDir = trackObject.getPosition(getTime()) - getPosition();
        
        setOrientation(lookAt(Point3d(0, 0, 0),
                              Point3d(viewDir.x, viewDir.y, viewDir.z),
                              up));
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
    Quatd q = getOrientation();
    q.yrotate(PI);
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
                                     ObserverFrame::CoordinateSystem offsetCoordSys,
                                     Vec3f up,
                                     ObserverFrame::CoordinateSystem upCoordSys)
{
    if (frame->getCoordinateSystem() == ObserverFrame::PhaseLock)
    {
        //setFrame(FrameOfReference(astro::Ecliptical, destination));
        setFrame(ObserverFrame::Ecliptical, destination);
    }
    else
    {
        setFrame(frame->getCoordinateSystem(), destination);
    }
    
    UniversalCoord targetPosition = destination.getPosition(getTime());
    Vec3d v = targetPosition - getPosition();
    v.normalize();
    
    jparams.traj = Linear;
    jparams.duration = gotoTime;
    jparams.startTime = realTime;
    
    // Right where we are now . . .
    jparams.from = getPosition();
    
    if (offsetCoordSys == ObserverFrame::ObserverLocal)
    {
        offset = offset * orientationUniv.toMatrix3();
    }
    else
    {
        ObserverFrame offsetFrame(offsetCoordSys, destination);
        offset = offset * offsetFrame.getFrame()->getOrientation(getTime()).toMatrix3();
    }
    jparams.to = targetPosition + offset;
    
    Vec3d upd(up.x, up.y, up.z);
    if (upCoordSys == ObserverFrame::ObserverLocal)
    {
        upd = upd * orientationUniv.toMatrix3();
    }
    else
    {
        ObserverFrame upFrame(upCoordSys, destination);
        upd = upd * upFrame.getFrame()->getOrientation(getTime()).toMatrix3();
    }
    
    jparams.initialOrientation = getOrientation();
    Vec3d vn = targetPosition - jparams.to;
    Point3d focus(vn.x, vn.y, vn.z);
    jparams.finalOrientation = lookAt(Point3d(0, 0, 0), focus, upd);
    jparams.startInterpolation = min(startInter, endInter);
    jparams.endInterpolation   = max(startInter, endInter);
    
    jparams.accelTime = 0.5;
    double distance = astro::microLightYearsToKilometers(jparams.from.distanceTo(jparams.to)) / 2.0;
    pair<double, double> sol = solve_bisection(TravelExpFunc(distance, jparams.accelTime),
                                               0.0001, 100.0,
                                               1e-10);
    jparams.expFactor = sol.first;
    
    // Convert to frame coordinates
    jparams.from = frame->convertFromUniversal(jparams.from, getTime());
    jparams.initialOrientation = frame->convertFromUniversal(jparams.initialOrientation, getTime());
    jparams.to = frame->convertFromUniversal(jparams.to, getTime());
    jparams.finalOrientation = frame->convertFromUniversal(jparams.finalOrientation, getTime());        
}


void Observer::computeGotoParametersGC(const Selection& destination,
                                       JourneyParams& jparams,
                                       double gotoTime,
                                       double startInter,
                                       double endInter,
                                       Vec3d offset,
                                       ObserverFrame::CoordinateSystem offsetCoordSys,
                                       Vec3f up,
                                       ObserverFrame::CoordinateSystem upCoordSys,
                                       const Selection& centerObj)
{
    setFrame(frame->getCoordinateSystem(), destination);

    UniversalCoord targetPosition = destination.getPosition(getTime());
    Vec3d v = targetPosition - getPosition();
    v.normalize();

    jparams.traj = GreatCircle;
    jparams.duration = gotoTime;
    jparams.startTime = realTime;

    jparams.centerObject = centerObj;

    // Right where we are now . . .
    jparams.from = getPosition();

    ObserverFrame offsetFrame(offsetCoordSys, destination);
    offset = offset * offsetFrame.getFrame()->getOrientation(getTime()).toMatrix3();
    
    jparams.to = targetPosition + offset;

    Vec3d upd(up.x, up.y, up.z);
    if (upCoordSys == ObserverFrame::ObserverLocal)
    {
        upd = upd * orientationUniv.toMatrix3();
    }
    else
    {
        ObserverFrame upFrame(upCoordSys, destination);
        upd = upd * upFrame.getFrame()->getOrientation(getTime()).toMatrix3();
    }
    
    jparams.initialOrientation = getOrientation();
    Vec3d vn = targetPosition - jparams.to;
    Point3d focus(vn.x, vn.y, vn.z);
    jparams.finalOrientation = lookAt(Point3d(0, 0, 0), focus, upd);
    jparams.startInterpolation = min(startInter, endInter);
    jparams.endInterpolation   = max(startInter, endInter);

    jparams.accelTime = 0.5;
    double distance = astro::microLightYearsToKilometers(jparams.from.distanceTo(jparams.to)) / 2.0;
    pair<double, double> sol = solve_bisection(TravelExpFunc(distance, jparams.accelTime),
                                               0.0001, 100.0,
                                               1e-10);
    jparams.expFactor = sol.first;

    // Convert to frame coordinates
    jparams.from = frame->convertFromUniversal(jparams.from, getTime());
    jparams.initialOrientation = frame->convertFromUniversal(jparams.initialOrientation, getTime());
    jparams.to = frame->convertFromUniversal(jparams.to, getTime());
    jparams.finalOrientation = frame->convertFromUniversal(jparams.finalOrientation, getTime());    
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

    Vec3d up = Vec3d(0, 1, 0) * getOrientation().toMatrix3();

    jparams.initialOrientation = getOrientation();
    Vec3d vn = targetPosition - jparams.to;
    Point3d focus(vn.x, vn.y, vn.z);
    jparams.finalOrientation = lookAt(Point3d(0, 0, 0), focus, up);
    jparams.startInterpolation = 0;
    jparams.endInterpolation   = 1;


    jparams.accelTime = 0.5;
    jparams.expFactor = 0;

    // Convert to frame coordinates
    jparams.from = frame->convertFromUniversal(jparams.from, getTime());
    jparams.initialOrientation = frame->convertFromUniversal(jparams.initialOrientation, getTime());
    jparams.to = frame->convertFromUniversal(jparams.to, getTime());
    jparams.finalOrientation = frame->convertFromUniversal(jparams.finalOrientation, getTime());    
}


void Observer::computeCenterCOParameters(const Selection& destination,
                                         JourneyParams& jparams,
                                         double centerTime)
{
    jparams.duration = centerTime;
    jparams.startTime = realTime;
    jparams.traj = CircularOrbit;

    jparams.centerObject = frame->getRefObject();
    jparams.expFactor = 0.5;

    Vec3d v = destination.getPosition(getTime()) - getPosition();
    Vec3d w = Vec3d(0.0, 0.0, -1.0) * getOrientation().toMatrix3();
    v.normalize();

    Selection centerObj = frame->getRefObject();
    UniversalCoord centerPos = centerObj.getPosition(getTime());
    UniversalCoord targetPosition = destination.getPosition(getTime());

    Quatd q = Quatd::vecToVecRotation(v, w);

    jparams.from = getPosition();
    jparams.to = centerPos + ((getPosition() - centerPos) * q.toMatrix3());
    jparams.initialOrientation = getOrientation();
    jparams.finalOrientation = getOrientation() * q;

    jparams.startInterpolation = 0.0;
    jparams.endInterpolation = 1.0;

    jparams.rotation1 = q;

    // Convert to frame coordinates
    jparams.from = frame->convertFromUniversal(jparams.from, getTime());
    jparams.initialOrientation = frame->convertFromUniversal(jparams.initialOrientation, getTime());
    jparams.to = frame->convertFromUniversal(jparams.to, getTime());
    jparams.finalOrientation = frame->convertFromUniversal(jparams.finalOrientation, getTime());
}


/*! Center the selection by moving on a circular orbit arround
* the primary body (refObject).
*/
void Observer::centerSelectionCO(const Selection& selection, double centerTime)
{
    if (!selection.empty() && !frame->getRefObject().empty())
    {
        computeCenterCOParameters(selection, journey, centerTime);
        observerMode = Travelling;
    }
}


Observer::ObserverMode Observer::getMode() const
{
    return observerMode;
}


void Observer::setMode(Observer::ObserverMode mode)
{
    observerMode = mode;
}


// Private method to convert coordinates when a new observer frame is set.
// Universal coordinates remain the same. All frame coordinates get updated, including
// the goto parameters.
void Observer::convertFrameCoordinates(const ObserverFrame* newFrame)
{
    double now = getTime();
    
    // Universal coordinates don't change.
    // Convert frame coordinates to the new frame.
    position = newFrame->convertFromUniversal(positionUniv, now);
    orientation = newFrame->convertFromUniversal(orientationUniv, now);
    
    // Convert goto parameters to the new frame
    journey.from               = ObserverFrame::convert(frame, newFrame, journey.from, now);
    journey.initialOrientation = ObserverFrame::convert(frame, newFrame, journey.initialOrientation, now);
    journey.to                 = ObserverFrame::convert(frame, newFrame, journey.to, now);
    journey.finalOrientation   = ObserverFrame::convert(frame, newFrame, journey.finalOrientation, now);
}


/*! Set the observer's reference frame. The position of the observer in
*   universal coordinates will not change.
*/
void Observer::setFrame(ObserverFrame::CoordinateSystem cs, const Selection& refObj, const Selection& targetObj)
{
    ObserverFrame* newFrame = new ObserverFrame(cs, refObj, targetObj);    
    if (newFrame != NULL)
    {
        convertFrameCoordinates(newFrame);
        delete frame;
        frame = newFrame;
    }
}


/*! Set the observer's reference frame. The position of the observer in
*  universal coordinates will not change.
*/
void Observer::setFrame(ObserverFrame::CoordinateSystem cs, const Selection& refObj)
{
    setFrame(cs, refObj, Selection());
}


/*! Set the observer's reference frame. The position of the observer in
 *  universal coordinates will not change.
 */
void Observer::setFrame(const ObserverFrame& f)
{
    if (frame != &f)
    {
        ObserverFrame* newFrame = new ObserverFrame(f);
        
        if (newFrame != NULL)
        {
			if (frame != NULL)
			{
				convertFrameCoordinates(newFrame);
				delete frame;
			}
            frame = newFrame;
        }
    }    
}


/*! Get the current reference frame for the observer.
 */
const ObserverFrame* Observer::getFrame() const
{
    return frame;
}


/*! Rotate the observer about its center.
 */
void Observer::rotate(Quatf q)
{
    Quatd qd(q.w, q.x, q.y, q.z);
    orientation = qd * orientation;
    updateUniversal();
}


/*! Orbit around the reference object (if there is one.)  This involves changing
 *  both the observer's position and orientation. If there is no current center
 *  object, the specified selection will be used as the center of rotation, and
 *  the observer reference frame will be modified.
 */
void Observer::orbit(const Selection& selection, Quatf q)
{
    Selection center = frame->getRefObject();
    if (center.empty() && !selection.empty())
    {
        // Automatically set the center of the reference frame
        center = selection;
        setFrame(frame->getCoordinateSystem(), center);
    }

    if (!center.empty())
    {
        // Get the focus position (center of rotation) in frame
        // coordinates; in order to make this function work in all
        // frames of reference, it's important to work in frame
        // coordinates.
        UniversalCoord focusPosition = center.getPosition(getTime());
        //focusPosition = frame.fromUniversal(RigidTransform(focusPosition), getTime()).translation;
        focusPosition = frame->convertFromUniversal(focusPosition, getTime());

        // v = the vector from the observer's position to the focus
        //Vec3d v = situation.translation - focusPosition;
        Vec3d v = position - focusPosition;

        // Get a double precision version of the rotation
        Quatd qd(q.w, q.x, q.y, q.z);

        // To give the right feel for rotation, we want to premultiply
        // the current orientation by q.  However, because of the order in
        // which we apply transformations later on, we can't pre-multiply.
        // To get around this, we compute a rotation q2 such
        // that q1 * r = r * q2.
        Quatd qd2 = ~orientation * qd * orientation;
        qd2.normalize();

        // Roundoff errors will accumulate and cause the distance between
        // viewer and focus to drift unless we take steps to keep the
        // length of v constant.
        double distance = v.length();
        v = v * qd2.toMatrix3();
        v.normalize();
        v *= distance;

        //situation.rotation = situation.rotation * qd2;
        //situation.translation = focusPosition + v;
        orientation = orientation * qd2;
        position = focusPosition + v;
        updateUniversal();
    }
}


/*! Exponential camera dolly--move toward or away from the selected object
 * at a rate dependent on the observer's distance from the object.
 */
void Observer::changeOrbitDistance(const Selection& selection, float d)
{
    Selection center = frame->getRefObject();
    if (center.empty() && !selection.empty())
    {
        center = selection;
        setFrame(frame->getCoordinateSystem(), center);
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

        if (currentDistance < minOrbitDistance)
            minOrbitDistance = currentDistance * 0.5;

        if (currentDistance >= minOrbitDistance && naturalOrbitDistance != 0)
        {
            double r = (currentDistance - minOrbitDistance) / naturalOrbitDistance;
            double newDistance = minOrbitDistance + naturalOrbitDistance * exp(log(r) + d);
            v = v * (newDistance / currentDistance);

            position = frame->convertFromUniversal(focusPosition + v, getTime());
            updateUniversal();
        }
    }
}


void Observer::setTargetSpeed(float s)
{
    targetSpeed = s;
    Vec3d v;
    if (reverseFlag)
        s = -s;
    if (trackObject.empty())
    {
        trackingOrientation = getOrientation();
        // Generate vector for velocity using current orientation
        // and specified speed.
        v = Vec3d(0, 0, -s) * getOrientation().toMatrix4();
    }
    else
    {
        // Use tracking orientation vector to generate target velocity
        v = Vec3d(0, 0, -s) * trackingOrientation.toMatrix4();
    }

    targetVelocity = v;
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
                             ObserverFrame::CoordinateSystem upFrame)
{
    gotoSelection(selection, gotoTime, 0.0, 0.5, up, upFrame);
}


// Return the preferred distance for viewing an object
static double getPreferredDistance(const Selection& selection)
{
    switch (selection.getType())
    {
    case Selection::Type_Body:
        // Handle reference points (i.e. invisible objects) specially, since the
        // actual radius of the point is meaningless. Instead, use the size of
        // bounding sphere of all child objects. This is useful for system
        // barycenters--the normal goto command will place the observer at 
        // a viewpoint in which the entire system can be seen.
        if (selection.body()->getClassification() == Body::Invisible)
        {
            double r = selection.body()->getRadius();
            if (selection.body()->getFrameTree() != NULL)
                r = selection.body()->getFrameTree()->boundingSphereRadius();
            return min(astro::lightYearsToKilometers(0.1), r * 5.0);
        }
        else
        {
            return 5.0 * selection.radius();
        }

    case Selection::Type_DeepSky:
        return 5.0 * selection.radius();

    case Selection::Type_Star:
        if (selection.star()->getVisibility())
        {
            return 100.0 * selection.radius();
        }
        else
        {
            // Handle star system barycenters specially, using the same approach as
            // for reference points in solar systems.
            double maxOrbitRadius = 0.0;
            const vector<Star*>* orbitingStars = selection.star()->getOrbitingStars();
            if (orbitingStars != NULL)
            {
                for (vector<Star*>::const_iterator iter = orbitingStars->begin();
                     iter != orbitingStars->end(); iter++)
                {
                    Orbit* orbit = (*iter)->getOrbit();
                    if (orbit != NULL)
                        maxOrbitRadius = max(orbit->getBoundingRadius(), maxOrbitRadius);
                }
            }

            if (maxOrbitRadius == 0.0)
                return astro::AUtoKilometers(1.0);
            else
                return maxOrbitRadius * 5.0;
        }

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
                             ObserverFrame::CoordinateSystem upFrame)
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
                              ObserverFrame::Universal,
                              up, upFrame);
        observerMode = Travelling;
    }
}


/*! Like normal goto, except we'll follow a great circle trajectory.  Useful
 *  for travelling between surface locations, where we'd rather not go straight
 *  through the middle of a planet.
 */
void Observer::gotoSelectionGC(const Selection& selection,
                               double gotoTime,
                               double /*startInter*/,       //TODO: remove parameter??
                               double /*endInter*/,         //TODO: remove parameter??
                               Vec3f up,
                               ObserverFrame::CoordinateSystem upFrame)
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
                                ObserverFrame::Universal,
                                up, upFrame,
                                centerObj);
        observerMode = Travelling;
    }
}


void Observer::gotoSelection(const Selection& selection,
                             double gotoTime,
                             double distance,
                             Vec3f up,
                             ObserverFrame::CoordinateSystem upFrame)
{
    if (!selection.empty())
    {
        UniversalCoord pos = selection.getPosition(getTime());
        // The destination position lies along the line between the current
        // position and the star
        Vec3d v = pos - getPosition();
        v.normalize();

        computeGotoParameters(selection, journey, gotoTime, 0.25, 0.75,
                              v * -distance * 1e6, ObserverFrame::Universal,
                              up, upFrame);
        observerMode = Travelling;
    }
}


void Observer::gotoSelectionGC(const Selection& selection,
                               double gotoTime,
                               double distance,
                               Vec3f up,
                               ObserverFrame::CoordinateSystem upFrame)
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
                                v * -distance * 1e6, ObserverFrame::Universal,
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
        double theta = longitude;
        double x = cos(theta) * sin(phi);
        double y = cos(phi);
        double z = -sin(theta) * sin(phi);
        computeGotoParameters(selection, journey, gotoTime, 0.25, 0.75,
                              Vec3d(x, y, z) * distance * 1e6, ObserverFrame::BodyFixed,
                              up, ObserverFrame::BodyFixed);
        observerMode = Travelling;
    }
}


void Observer::gotoLocation(const UniversalCoord& toPosition,
                            const Quatd& toOrientation,
                            double duration)
{
    journey.startTime = realTime;
    journey.duration = duration;

    journey.from = position;
    journey.initialOrientation = orientation;    
    journey.to = toPosition;
    journey.finalOrientation = toOrientation;
    
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
        ObserverFrame frame(ObserverFrame::BodyFixed, selection);
        Point3d bfPos = (Point3d) frame.convertFromUniversal(positionUniv, getTime());
        
        distance = bfPos.distanceFromOrigin();

        // Convert from Celestia's coordinate system
        double x = bfPos.x;
        double y = -bfPos.z;
        double z = bfPos.y;

        longitude = radToDeg(atan2(y, x));
        latitude = radToDeg(PI/2 - acos(z / distance));
        
        // Convert distance from light years to kilometers.
        distance = astro::microLightYearsToKilometers(distance);
    }
}


void Observer::gotoSurface(const Selection& sel, double duration)
{
    Vec3d v = getPosition() - sel.getPosition(getTime());
    v.normalize();
    
    Vec3d viewDir = Vec3d(0, 0, -1) * orientationUniv.toMatrix3();
    Vec3d up = Vec3d(0, 1, 0) * orientationUniv.toMatrix3();
    Quatd q = orientationUniv;
    if (v * viewDir < 0.0)
    {
        q = lookAt(Point3d(0.0, 0.0, 0.0), Point3d(0.0, 0.0, 0.0) + up, v);
    }
    
    ObserverFrame frame(ObserverFrame::BodyFixed, sel);
    UniversalCoord bfPos = frame.convertFromUniversal(positionUniv, getTime());
    q = frame.convertFromUniversal(q, getTime());
    
    double height = 1.0001 * astro::kilometersToMicroLightYears(sel.radius());
    Vec3d dir = bfPos - Point3d(0.0, 0.0, 0.0);
    dir.normalize();
    dir *= height;
    UniversalCoord nearSurfacePoint(dir.x, dir.y, dir.z);
    
    gotoLocation(nearSurfacePoint, q, duration);    
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
    setFrame(ObserverFrame::Ecliptical, selection);
}


void Observer::geosynchronousFollow(const Selection& selection)
{
    if (selection.body() != NULL ||
        selection.location() != NULL ||
        selection.star() != NULL)
    {
        setFrame(ObserverFrame::BodyFixed, selection);
    }
}


void Observer::phaseLock(const Selection& selection)
{
    Selection refObject = frame->getRefObject();
    
    if (selection != refObject)
    {
        if (refObject.body() != NULL || refObject.star() != NULL)
        {
            setFrame(ObserverFrame::PhaseLock, refObject, selection);
        }
    }
    else
    {
        // Selection and reference object are identical, so the frame is undefined.
        // We'll instead use the object's star as the target object.
        if (selection.body() != NULL)
        {
            setFrame(ObserverFrame::PhaseLock, selection, Selection(selection.body()->getSystem()->getStar()));
        }        
    }
}


void Observer::chase(const Selection& selection)
{
    if (selection.body() != NULL || selection.star() != NULL)
    {
        setFrame(ObserverFrame::Chase, selection);
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


// Internal method to update the position and orientation of the observer in
// universal coordinates.
void Observer::updateUniversal()
{
    positionUniv = frame->convertToUniversal(position, simTime);
    orientationUniv = frame->convertToUniversal(orientation, simTime);
}


/*! Create the default 'universal' observer frame, with a center at the
 *  Solar System barycenter and coordinate axes of the J200Ecliptic
 *  reference frame.
 */
ObserverFrame::ObserverFrame() :
    coordSys(Universal),
    frame(NULL)
{
    frame = createFrame(Universal, Selection(), Selection());
    frame->addRef();
}


/*! Create a new frame with the specified coordinate system and
 *  reference object. The targetObject is only needed for phase
 *  lock frames; the argument is ignored for other frames.
 */
ObserverFrame::ObserverFrame(CoordinateSystem _coordSys,
                             const Selection& _refObject,
                             const Selection& _targetObject) :
    coordSys(_coordSys),
    frame(NULL),
    targetObject(_targetObject)
{
    frame = createFrame(_coordSys, _refObject, _targetObject);
    frame->addRef();
}


/*! Create a new ObserverFrame with the specified reference frame.
 *  The coordinate system of this frame will be marked as unknown.
 */
ObserverFrame::ObserverFrame(const ReferenceFrame &f) :
    coordSys(Unknown),
    frame(&f)
{
    frame->addRef();
}


/*! Copy constructor. */
ObserverFrame::ObserverFrame(const ObserverFrame& f) :
    coordSys(f.coordSys),
    frame(f.frame),
    targetObject(f.targetObject)
{
    frame->addRef();
}


ObserverFrame& ObserverFrame::operator=(const ObserverFrame& f)
{
    coordSys = f.coordSys;
    targetObject = f.targetObject;
    
    // In case frames are the same, make sure we addref before releasing
    f.frame->addRef();
    frame->release();
    frame = f.frame;
    
    return *this;
}


ObserverFrame::~ObserverFrame()
{
    if (frame != NULL)
        frame->release();
}


ObserverFrame::CoordinateSystem
ObserverFrame::getCoordinateSystem() const
{
    return coordSys;
}


Selection
ObserverFrame::getRefObject() const
{
    return frame->getCenter();
}


Selection
ObserverFrame::getTargetObject() const
{
    return targetObject;
}


const ReferenceFrame*
ObserverFrame::getFrame() const
{
    return frame;
}


UniversalCoord 
ObserverFrame::convertFromUniversal(const UniversalCoord& uc, double tjd) const
{
    return frame->convertFromUniversal(uc, tjd);
}


UniversalCoord
ObserverFrame::convertToUniversal(const UniversalCoord& uc, double tjd) const
{
    return frame->convertToUniversal(uc, tjd);
}


Quatd 
ObserverFrame::convertFromUniversal(const Quatd& q, double tjd) const
{
    return frame->convertFromUniversal(q, tjd);
}


Quatd
ObserverFrame::convertToUniversal(const Quatd& q, double tjd) const
{
    return frame->convertToUniversal(q, tjd);
}


/*! Convert a position from one frame to another.
 */
UniversalCoord
ObserverFrame::convert(const ObserverFrame* fromFrame,
                       const ObserverFrame* toFrame,
                       const UniversalCoord& uc,
                       double t)
{
    // Perform the conversion fromFrame -> universal -> toFrame
    return toFrame->convertFromUniversal(fromFrame->convertToUniversal(uc, t), t);
}


/*! Convert an orientation from one frame to another.
*/
Quatd
ObserverFrame::convert(const ObserverFrame* fromFrame,
                       const ObserverFrame* toFrame,
                       const Quatd& q,
                       double t)
{
    // Perform the conversion fromFrame -> universal -> toFrame
    return toFrame->convertFromUniversal(fromFrame->convertToUniversal(q, t), t);
}


// Create the ReferenceFrame for the specified observer frame parameters.
ReferenceFrame*
ObserverFrame::createFrame(CoordinateSystem _coordSys,
                           const Selection& _refObject,
                           const Selection& _targetObject)
{
    switch (_coordSys)
    {
    case Universal:
        return new J2000EclipticFrame(Selection());
        
    case Ecliptical:
        return new J2000EclipticFrame(_refObject);
        
    case Equatorial:
        return new BodyMeanEquatorFrame(_refObject, _refObject);
        
    case BodyFixed:
        return new BodyFixedFrame(_refObject, _refObject);
        
    case PhaseLock:
    {
        return new TwoVectorFrame(_refObject,
                                  FrameVector::createRelativePositionVector(_refObject, _targetObject), 1,
                                  FrameVector::createRelativeVelocityVector(_refObject, _targetObject), 2);
    }
        
    case Chase:
    {
        return new TwoVectorFrame(_refObject,
                                  FrameVector::createRelativeVelocityVector(_refObject, _refObject.parent()), 1,
                                  FrameVector::createRelativePositionVector(_refObject, _refObject.parent()), 2);
    }
        
    case PhaseLock_Old:
    {
        FrameVector rotAxis(FrameVector::createConstantVector(Vec3d(0, 1, 0),
                                                              new BodyMeanEquatorFrame(_refObject, _refObject)));
        return new TwoVectorFrame(_refObject,
                                  FrameVector::createRelativePositionVector(_refObject, _targetObject), 3,
                                  rotAxis, 2);
    }
        
    case Chase_Old:
    {
        FrameVector rotAxis(FrameVector::createConstantVector(Vec3d(0, 1, 0),
                                                              new BodyMeanEquatorFrame(_refObject, _refObject)));
        
        return new TwoVectorFrame(_refObject,
                                  FrameVector::createRelativeVelocityVector(_refObject.parent(), _refObject), 3,
                                  rotAxis, 2);
    }

    case ObserverLocal:
        // TODO: This is only used for computing up vectors for orientation; it does
        // define a proper frame for the observer position orientation.
        return new J2000EclipticFrame(Selection());
        
    default:
        return new J2000EclipticFrame(_refObject);
    }
}


