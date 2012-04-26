// observer.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "observer.h"
#include "simulation.h"
#include "frametree.h"
#include <celmath/mathlib.h>
#include <celmath/solve.h>

static const double maximumSimTime = 730486721060.00073; // 2000000000 Jan 01 12:00:00 UTC
static const double minimumSimTime = -730498278941.99951; // -2000000000 Jan 01 12:00:00 UTC

using namespace Eigen;
using namespace std;


#define VELOCITY_CHANGE_TIME      0.25f


static Vector3d slerp(double t, const Vector3d& v0, const Vector3d& v1)
{
    double r0 = v0.norm();
    double r1 = v1.norm();
    Vector3d u = v0 / r0;
    Vector3d n = u.cross(v1 / r1);
    n.normalize();
    Vector3d v = n.cross(u);
    if (v.dot(v1) < 0.0)
        v = -v;

    double cosTheta = u.dot(v1 / r1);
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
    orientation(Quaternionf::Identity()),
    velocity(0.0, 0.0, 0.0),
    angularVelocity(0.0, 0.0, 0.0),
    frame(NULL),
    realTime(0.0),
    targetSpeed(0.0),
    targetVelocity(0.0, 0.0, 0.0),
    initialVelocity(0.0, 0.0, 0.0),
    beginAccelTime(0.0),
    observerMode(Free),
    trackingOrientation(Quaternionf::Identity()),
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

#if 0
// TODO: Low-precision set position that should be removed
void Observer::setPosition(const Vector3d& p)
{
    setPosition(UniversalCoord(p));
}
#endif

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
Quaterniond Observer::getOrientation() const
{
    return orientationUniv;
}

/*! Reduced precision version of getOrientation()
 */
Quaternionf Observer::getOrientationf() const
{
    return getOrientation().cast<float>();
}


/* Set the orientation of the observer. The orientation is specified in
 * the universal coordinate system.
 */
void Observer::setOrientation(const Quaternionf& q)
{
    /*
    RigidTransform rt = frame.toUniversal(situation, getTime());
    rt.rotation = Quatd(q.w, q.x, q.y, q.z);
    situation = frame.fromUniversal(rt, getTime());
     */
    setOrientation(q.cast<double>());
}


/*! Set the orientation of the observer. The orientation is specified in
 *  the universal coordinate system.
 */
void Observer::setOrientation(const Quaterniond& q)
{
    orientationUniv = q;
    orientation = frame->convertFromUniversal(q, getTime());
}


/*! Get the velocity of the observer within the observer's reference frame.
 */
Vector3d Observer::getVelocity() const
{
    return velocity;
}


/*! Set the velocity of the observer within the observer's reference frame.
*/
void Observer::setVelocity(const Vector3d& v)
{
    velocity = v;
}


Vector3d Observer::getAngularVelocity() const
{
    return angularVelocity;
}


void Observer::setAngularVelocity(const Vector3d& v)
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
lookAt(Matrix<T, 3, 1> from, Matrix<T, 3, 1> to, Matrix<T, 3, 1> up)
{
    Matrix<T, 3, 1> n = to - from;
    n.normalize();
    Matrix<T, 3, 1> v = n.cross(up).normalized();
    Matrix<T, 3, 1> u = v.cross(n);

    Matrix<T, 3, 3> m;
    m.col(0) = v;
    m.col(1) = u;
    m.col(2) = -n;

    return Quaternion<T>(m).conjugate();
}

#ifdef CELVEC
template<class T> static Quat<T> 
lookAt(Point3<T> from, Point3<T> to, Vector3<T> up)
{
    Vector3<T> n = to - from;
    n.normalize();
    Vector3<T> v = n ^ up;
    v.normalize();
    Vector3<T> u = v ^ n;

    return Quat<T>::matrixToQuaternion(Matrix3<T>(v, u, -n));
}
#endif

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

        Vector3d jv = journey.to.offsetFromKm(journey.from);
#ifdef CELVEC
        Vector3d jv = journey.to - journey.from;
#endif
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
                Vector3d v = jv;
                if (v.norm() == 0.0)
                {
                    p = journey.from;
                }
                else
                {
                    v.normalize();
                    if (t < 0.5)
                        p = journey.from.offsetKm(v * x);
                    else
                        p = journey.to.offsetKm(-v * x);
#ifdef CELVEC
                    if (t < 0.5)
                        p = journey.from + v * astro::kilometersToMicroLightYears(x);
                    else
                        p = journey.to - v * astro::kilometersToMicroLightYears(x);
#endif
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
                Vector3d v0 = ufrom.offsetFromKm(origin);
                Vector3d v1 = uto.offsetFromKm(origin);

                if (jv.norm() == 0.0)
                {
                    p = journey.from;
                }
                else
                {
                    x /= jv.norm();
                    Vector3d v;

                    if (t < 0.5)
                        v = slerp(x, v0, v1);
                    else
                        v = slerp(x, v1, v0);

                    p = frame->convertFromUniversal(origin.offsetKm(v), simTime);
#ifdef CELVEC
                    x = astro::kilometersToMicroLightYears(x / jv.length());
                    Vector3d v;

                    if (t < 0.5)
                        v = slerp(x, v0, v1);
                    else
                        v = slerp(x, v1, v0);

                    p = frame->convertFromUniversal(origin + v, simTime);
#endif
                }
            }
            else if (journey.traj == CircularOrbit)
            {
                Selection centerObj = frame->getRefObject();

                UniversalCoord ufrom = frame->convertToUniversal(journey.from, simTime);
                UniversalCoord uto   = frame->convertToUniversal(journey.to, simTime);
                UniversalCoord origin = centerObj.getPosition(simTime);

                Vector3d v0 = ufrom.offsetFromKm(origin);
                Vector3d v1 = uto.offsetFromKm(origin);

                if (jv.norm() == 0.0)
                {
                    p = journey.from;
                }
                else
                {
                    Quaterniond q0(Quaterniond::Identity());
                    Quaterniond q1 = journey.rotation1;
                    p = origin.offsetKm(q0.slerp(t, q1).conjugate() * v0);
                    p = frame->convertFromUniversal(p, simTime);
                }
            }
        }

        // Spherically interpolate the orientation over the first half
        // of the journey.
        Quaterniond q;
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

            q = journey.initialOrientation.slerp(v, journey.finalOrientation);
#ifdef CELVEC
            // Be careful to choose the shortest path when interpolating
            if ((journey.initialOrientation.coeffs() - journey.finalOrientation.coeffs()).norm() <
                (journey.initialOrientation.coeffs() + journey.finalOrientation.coeffs()).norm())
            {
                q = journey.initialOrientation.slerp(v, journey.finalOrientation);
            }
            else
            {
                q = journey.initialOrientation.slerp(v, journey.finalOrientation * -1.0);
            }
#endif
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
            setVelocity(Vector3d::Zero());
        }
    }

    if (getVelocity() != targetVelocity)
    {
        double t = clamp((realTime - beginAccelTime) / VELOCITY_CHANGE_TIME);
        Vector3d v = getVelocity() * (1.0 - t) + targetVelocity * t;

        // At some threshold, we just set the velocity to zero; otherwise,
        // we'll end up with ridiculous velocities like 10^-40 m/s.
        if (v.norm() < 1.0e-12)
            v = Vector3d::Zero();
        setVelocity(v);
    }

    // Update the position
    position = position.offsetKm(getVelocity() * dt);

    if (observerMode == Free)
    {
        // Update the observer's orientation
        Vector3d halfAV = getAngularVelocity() * 0.5;
        Quaterniond dr = Quaterniond(0.0, halfAV.x(), halfAV.y(), halfAV.z()) * orientation;
        orientation = Quaterniond(orientation.coeffs() + dt * dr.coeffs());
        orientation.normalize();
#ifdef CELVEC
        Quaterniond dr = 0.5 * (AV * orientation);
        orientation += dt * dr;
        orientation.normalize();
#endif
    }
   
    updateUniversal();

    // Update orientation for tracking--must occur after updateUniversal(), as it
    // relies on the universal position and orientation of the observer.
    if (!trackObject.empty())
    {
        Vector3d up = getOrientation().conjugate() * Vector3d::UnitY();
        Vector3d viewDir = trackObject.getPosition(getTime()).offsetFromKm(getPosition()).normalized();
        
        setOrientation(lookAt<double>(Vector3d::Zero(), viewDir, up));
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
    setOrientation(getOrientation() * Quaterniond(AngleAxisd(PI, Vector3d::UnitY())));
#ifdef CELVEC
    Quatd q = getOrientation();
    q.yrotate(PI);
    setOrientation(q);
#endif
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
                                     const Vector3d& offset,
                                     ObserverFrame::CoordinateSystem offsetCoordSys,
                                     const Vector3f& up,
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
    Vector3d v = targetPosition.offsetFromKm(getPosition()).normalized();
    
    jparams.traj = Linear;
    jparams.duration = gotoTime;
    jparams.startTime = realTime;
    
    // Right where we are now . . .
    jparams.from = getPosition();
    
    if (offsetCoordSys == ObserverFrame::ObserverLocal)
    {
        jparams.to = targetPosition.offsetKm(orientationUniv.conjugate() * offset);
    }
    else
    {
        ObserverFrame offsetFrame(offsetCoordSys, destination);
        jparams.to = targetPosition.offsetKm(offsetFrame.getFrame()->getOrientation(getTime()).conjugate() * offset);
    }
    
    Vector3d upd = up.cast<double>();
    if (upCoordSys == ObserverFrame::ObserverLocal)
    {
        upd = orientationUniv.conjugate() * upd;
    }
    else
    {
        ObserverFrame upFrame(upCoordSys, destination);
        upd = upFrame.getFrame()->getOrientation(getTime()).conjugate() * upd;
    }
    
    jparams.initialOrientation = getOrientation();
    Vector3d focus = targetPosition.offsetFromKm(jparams.to);
    jparams.finalOrientation = lookAt<double>(Vector3d::Zero(), focus, upd);
    jparams.startInterpolation = min(startInter, endInter);
    jparams.endInterpolation   = max(startInter, endInter);
    
    jparams.accelTime = 0.5;
    double distance = jparams.from.offsetFromKm(jparams.to).norm() / 2.0;
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
                                       const Vector3d& offset,
                                       ObserverFrame::CoordinateSystem offsetCoordSys,
                                       const Vector3f& up,
                                       ObserverFrame::CoordinateSystem upCoordSys,
                                       const Selection& centerObj)
{
    setFrame(frame->getCoordinateSystem(), destination);

    UniversalCoord targetPosition = destination.getPosition(getTime());
    Vector3d v = targetPosition.offsetFromKm(getPosition()).normalized();

    jparams.traj = GreatCircle;
    jparams.duration = gotoTime;
    jparams.startTime = realTime;

    jparams.centerObject = centerObj;

    // Right where we are now . . .
    jparams.from = getPosition();

    ObserverFrame offsetFrame(offsetCoordSys, destination);
    Vector3d offsetTransformed = offsetFrame.getFrame()->getOrientation(getTime()).conjugate() * offset;
    
    jparams.to = targetPosition.offsetKm(offsetTransformed);

    Vector3d upd = up.cast<double>();
    if (upCoordSys == ObserverFrame::ObserverLocal)
    {
        upd = orientationUniv.conjugate() * upd;
    }
    else
    {
        ObserverFrame upFrame(upCoordSys, destination);
        upd = upFrame.getFrame()->getOrientation(getTime()).conjugate() * upd;
    }
    
    jparams.initialOrientation = getOrientation();
    Vector3d focus = targetPosition.offsetFromKm(jparams.to);
    jparams.finalOrientation = lookAt<double>(Vector3d::Zero(), focus, upd);
    jparams.startInterpolation = min(startInter, endInter);
    jparams.endInterpolation   = max(startInter, endInter);

    jparams.accelTime = 0.5;
    double distance = jparams.from.offsetFromKm(jparams.to).norm() / 2.0;
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

    Vector3d up = getOrientation().conjugate() * Vector3d::UnitY();

    jparams.initialOrientation = getOrientation();
    Vector3d focus = targetPosition.offsetFromKm(jparams.to);
    jparams.finalOrientation = lookAt<double>(Vector3d::Zero(), focus, up);
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

    Vector3d v = destination.getPosition(getTime()).offsetFromKm(getPosition()).normalized();
    Vector3d w = getOrientation().conjugate() * -Vector3d::UnitZ();

    Selection centerObj = frame->getRefObject();
    UniversalCoord centerPos = centerObj.getPosition(getTime());
    UniversalCoord targetPosition = destination.getPosition(getTime());

    Quaterniond q;
    q.setFromTwoVectors(v, w);

    jparams.from = getPosition();
    jparams.to = centerPos.offsetKm(q.conjugate() * getPosition().offsetFromKm(centerPos));
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
void Observer::rotate(const Quaternionf& q)
{
    orientation = q.cast<double>() * orientation;
    updateUniversal();
}


/*! Orbit around the reference object (if there is one.)  This involves changing
 *  both the observer's position and orientation. If there is no current center
 *  object, the specified selection will be used as the center of rotation, and
 *  the observer reference frame will be modified.
 */
void Observer::orbit(const Selection& selection, const Quaternionf& q)
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
        Vector3d v = position.offsetFromKm(focusPosition);

        Quaterniond qd = q.cast<double>();

        // To give the right feel for rotation, we want to premultiply
        // the current orientation by q.  However, because of the order in
        // which we apply transformations later on, we can't pre-multiply.
        // To get around this, we compute a rotation q2 such
        // that q1 * r = r * q2.
        Quaterniond qd2 = orientation.conjugate() * qd * orientation;
        qd2.normalize();

        // Roundoff errors will accumulate and cause the distance between
        // viewer and focus to drift unless we take steps to keep the
        // length of v constant.
        double distance = v.norm();
        v = qd2.conjugate() * v;
        v = v.normalized() * distance;

        orientation = orientation * qd2;
        position = focusPosition.offsetKm(v);
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
        double minOrbitDistance = size;
        double naturalOrbitDistance = 4.0 * size;

        // Determine distance and direction to the selected object
        Vector3d v = getPosition().offsetFromKm(focusPosition);
        double currentDistance = v.norm();

        if (currentDistance < minOrbitDistance)
            minOrbitDistance = currentDistance * 0.5;

        if (currentDistance >= minOrbitDistance && naturalOrbitDistance != 0)
        {
            double r = (currentDistance - minOrbitDistance) / naturalOrbitDistance;
            double newDistance = minOrbitDistance + naturalOrbitDistance * exp(log(r) + d);
            v = v * (newDistance / currentDistance);

            position = frame->convertFromUniversal(focusPosition.offsetKm(v), getTime());
            updateUniversal();
        }
    }
}


void Observer::setTargetSpeed(float s)
{
    targetSpeed = s;
    Vector3d v;

    if (reverseFlag)
        s = -s;
    if (trackObject.empty())
    {
        trackingOrientation = getOrientation();
        // Generate vector for velocity using current orientation
        // and specified speed.
        v = getOrientation().conjugate() * Vector3d(0, 0, -s);
    }
    else
    {
        // Use tracking orientation vector to generate target velocity
        v = trackingOrientation.conjugate() * Vector3d(0, 0, -s);
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
    double distance = journey.from.offsetFromKm(journey.to).norm() / 2.0;
    pair<double, double> sol = solve_bisection(TravelExpFunc(distance, journey.accelTime),
                                               0.0001, 100.0,
                                               1e-10);
    journey.expFactor = sol.first;
    journey.startTime = realTime;
    observerMode = Travelling;
}

void Observer::gotoSelection(const Selection& selection,
                             double gotoTime,
                             const Vector3f& up,
                             ObserverFrame::CoordinateSystem upFrame)
{
    gotoSelection(selection, gotoTime, 0.0, 0.5, up, upFrame);
}


// Return the preferred distance (in kilometers) for viewing an object
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
    double maxDist = getPreferredDistance(selection);
    double minDist = 1.01 * selection.radius();
    double dist = (currentDistance > maxDist * 10.0) ? maxDist : currentDistance * 0.1;

    return max(dist, minDist);
}


void Observer::gotoSelection(const Selection& selection,
                             double gotoTime,
                             double startInter,
                             double endInter,
                             const Vector3f& up,
                             ObserverFrame::CoordinateSystem upFrame)
{
    if (!selection.empty())
    {
        UniversalCoord pos = selection.getPosition(getTime());
        Vector3d v = pos.offsetFromKm(getPosition());
        double distance = v.norm();

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
                               const Vector3f& up,
                               ObserverFrame::CoordinateSystem upFrame)
{
    if (!selection.empty())
    {
        Selection centerObj = selection.parent();

        UniversalCoord pos = selection.getPosition(getTime());
        Vector3d v = pos.offsetFromKm(centerObj.getPosition(getTime()));
        double distanceToCenter = v.norm();
        Vector3d viewVec = pos.offsetFromKm(getPosition());
        double orbitDistance = getOrbitDistance(selection, viewVec.norm());

        if (selection.location() != NULL)
        {
            Selection parent = selection.parent();
            double maintainDist = getPreferredDistance(parent);
            Vector3d parentPos = parent.getPosition(getTime()).offsetFromKm(getPosition());
            double parentDist = parentPos.norm() - parent.radius();

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
                             const Vector3f& up,
                             ObserverFrame::CoordinateSystem upFrame)
{
    if (!selection.empty())
    {
        UniversalCoord pos = selection.getPosition(getTime());
        // The destination position lies along the line between the current
        // position and the star
        Vector3d v = pos.offsetFromKm(getPosition());
        v.normalize();

        computeGotoParameters(selection, journey, gotoTime, 0.25, 0.75,
                              v * -distance, ObserverFrame::Universal,
                              up, upFrame);
        observerMode = Travelling;
    }
}


void Observer::gotoSelectionGC(const Selection& selection,
                               double gotoTime,
                               double distance,
                               const Vector3f& up,
                               ObserverFrame::CoordinateSystem upFrame)
{
    if (!selection.empty())
    {
        Selection centerObj = selection.parent();

        UniversalCoord pos = selection.getPosition(getTime());
        Vector3d v = pos.offsetFromKm(centerObj.getPosition(getTime()));
        v.normalize();

        // The destination position lies along a line extended from the center
        // object to the target object
        computeGotoParametersGC(selection, journey, gotoTime, 0.25, 0.75,
                                v * -distance, ObserverFrame::Universal,
                                up, upFrame,
                                centerObj);
        observerMode = Travelling;
    }
}


/** Make the observer travel to the specified planetocentric coordinates.
 *  @param selection the central object
 *  @param gotoTime travel time in seconds of real time
 *  @param distance the distance from the center (in kilometers)
 *  @param longitude longitude in radians
 *  @param latitude latitude in radians
 */
void Observer::gotoSelectionLongLat(const Selection& selection,
                                    double gotoTime,
                                    double distance,
                                    float longitude,
                                    float latitude,
                                    const Vector3f& up)
{
    if (!selection.empty())
    {
        double phi = -latitude + PI / 2;
        double theta = longitude;
        double x = cos(theta) * sin(phi);
        double y = cos(phi);
        double z = -sin(theta) * sin(phi);
        computeGotoParameters(selection, journey, gotoTime, 0.25, 0.75,
                              Vector3d(x, y, z) * distance, ObserverFrame::BodyFixed,
                              up, ObserverFrame::BodyFixed);
        observerMode = Travelling;
    }
}


void Observer::gotoLocation(const UniversalCoord& toPosition,
                            const Quaterniond& toOrientation,
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
    double distance = journey.from.offsetFromKm(journey.to).norm() / 2.0;
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
        Vector3d bfPos = frame.convertFromUniversal(positionUniv, getTime()).offsetFromKm(UniversalCoord::Zero());
        
        // Convert from Celestia's coordinate system
        double x = bfPos.x();
        double y = -bfPos.z();
        double z = bfPos.y();

        distance = bfPos.norm();
        longitude = radToDeg(atan2(y, x));
        latitude = radToDeg(PI/2 - acos(z / distance));
    }
}


void Observer::gotoSurface(const Selection& sel, double duration)
{
    Vector3d v = getPosition().offsetFromKm(sel.getPosition(getTime()));
    v.normalize();
    
    Vector3d viewDir = orientationUniv.conjugate() * -Vector3d::UnitZ();
    Vector3d up      = orientationUniv.conjugate() * Vector3d::UnitY();
    Quaterniond q = orientationUniv;
    if (v.dot(viewDir) < 0.0)
    {
        q = lookAt<double>(Vector3d::Zero(), up, v);
    }
    
    ObserverFrame frame(ObserverFrame::BodyFixed, sel);
    UniversalCoord bfPos = frame.convertFromUniversal(positionUniv, getTime());
    q = frame.convertFromUniversal(q, getTime());
    
    double height = 1.0001 * sel.radius();
    Vector3d dir = bfPos.offsetFromKm(UniversalCoord::Zero()).normalized() * height;
    UniversalCoord nearSurfacePoint = UniversalCoord::Zero().offsetKm(dir);
    
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


Vector3f Observer::getPickRay(float x, float y) const
{
    float s = 2 * (float) tan(fov / 2.0);
    Vector3f pickDirection(x * s, y * s, -1.0f);

    return pickDirection.normalized();
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


Quaterniond 
ObserverFrame::convertFromUniversal(const Quaterniond& q, double tjd) const
{
    return frame->convertFromUniversal(q, tjd);
}


Quaterniond
ObserverFrame::convertToUniversal(const Quaterniond& q, double tjd) const
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
Quaterniond
ObserverFrame::convert(const ObserverFrame* fromFrame,
                       const ObserverFrame* toFrame,
                       const Quaterniond& q,
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
        FrameVector rotAxis(FrameVector::createConstantVector(Vector3d::UnitY(),
                                                              new BodyMeanEquatorFrame(_refObject, _refObject)));
        return new TwoVectorFrame(_refObject,
                                  FrameVector::createRelativePositionVector(_refObject, _targetObject), 3,
                                  rotAxis, 2);
    }
        
    case Chase_Old:
    {
        FrameVector rotAxis(FrameVector::createConstantVector(Vector3d::UnitY(),
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


