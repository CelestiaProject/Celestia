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

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numeric>

#include <celephem/orbit.h>
#include <celmath/geomutil.h>
#include <celmath/intersect.h>
#include <celmath/mathlib.h>
#include <celmath/solve.h>
#include <celmath/sphere.h>
#include "body.h"
#include "frame.h"
#include "frametree.h"
#include "location.h"
#include "star.h"

namespace astro = celestia::astro;
namespace math = celestia::math;

namespace
{

constexpr double maximumSimTime = 730486721060.00073; // 2000000000 Jan 01 12:00:00 UTC
constexpr double minimumSimTime = -730498278941.99951; // -2000000000 Jan 01 12:00:00 UTC

constexpr float VELOCITY_CHANGE_TIME = 0.25f;

Eigen::Vector3d
slerp(double t, const Eigen::Vector3d& v0, const Eigen::Vector3d& v1)
{
    double r0 = v0.norm();
    double r1 = v1.norm();
    Eigen::Vector3d u = v0 / r0;
    Eigen::Vector3d n = u.cross(v1 / r1);
    n.normalize();
    Eigen::Vector3d v = n.cross(u);
    if (v.dot(v1) < 0.0)
        v = -v;

    double cosTheta = u.dot(v1 / r1);
    double theta = std::acos(cosTheta);
    double sThetaT;
    double cThetaT;
    math::sincos(theta * t, sThetaT, cThetaT);

    return (cThetaT * u + sThetaT * v) * math::lerp(t, r0, r1);
}

std::optional<Eigen::Vector3d>
getNearIntersectionPoint(const Eigen::ParametrizedLine<double, 3>& ray,
                         const math::Sphered& sphere)
{
    double distance;
    bool intersects = math::testIntersection(ray, sphere, distance);
    if (intersects)
        return ray.origin() + distance * ray.direction();
    else
        return std::nullopt;
}

class TravelExpFunc
{
public:
    TravelExpFunc(double d, double _s) : dist(d), s(_s) {};

    double operator()(double x) const
    {
        // return (1.0 / x) * (exp(x / 2.0) - 1.0) - 0.5 - dist / 2.0;
        return std::exp(x * s) * (x * (1.0 - s) + 1.0) - 1.0 - dist;
    }

private:
    double dist;
    double s;
};

// Return the preferred distance (in kilometers) for viewing an object
double
getPreferredDistance(const Selection& selection)
{
    switch (selection.getType())
    {
    case SelectionType::Body:
        // Handle reference points (i.e. invisible objects) specially, since the
        // actual radius of the point is meaningless. Instead, use the size of
        // bounding sphere of all child objects. This is useful for system
        // barycenters--the normal goto command will place the observer at
        // a viewpoint in which the entire system can be seen.
        if (selection.body()->getClassification() == BodyClassification::Invisible)
        {
            double r = selection.body()->getRadius();
            if (selection.body()->getFrameTree() != nullptr)
                r = selection.body()->getFrameTree()->boundingSphereRadius();
            return std::min(astro::lightYearsToKilometers(0.1), r * 5.0);
        }
        else
        {
            return 5.0 * selection.radius();
        }

    case SelectionType::DeepSky:
        return 5.0 * selection.radius();

    case SelectionType::Star:
        if (selection.star()->getVisibility())
            return 100.0 * selection.radius();
        else
        {
            // Handle star system barycenters specially, using the same approach as
            // for reference points in solar systems.
            auto orbitingStars = selection.star()->getOrbitingStars();
            double maxOrbitRadius = std::accumulate(orbitingStars.begin(), orbitingStars.end(), 0.0,
                                    [](double r, const Star* s)
                                    {
                                        const celestia::ephem::Orbit* orbit = s->getOrbit();
                                        return orbit == nullptr
                                            ? r
                                            : std::max(r, orbit->getBoundingRadius());
                                    });

            return maxOrbitRadius == 0.0 ? astro::AUtoKilometers(1.0) : maxOrbitRadius * 5.0;
        }

    case SelectionType::Location:
        {
            double maxDist = getPreferredDistance(selection.location()->getParentBody());
            return maxDist < 1.0
                ? maxDist
                : std::clamp(selection.location()->getSize() * 50.0, 1.0, maxDist);
        }

    default:
        return 1.0;
    }
}

// Given an object and its current distance from the camera, determine how
// close we should go on the next goto.
double
getOrbitDistance(const Selection& selection, double currentDistance)
{
    // If further than 10 times the preferrred distance, goto the
    // preferred distance.  If closer, zoom in 10 times closer or to the
    // minimum distance.
    double maxDist = getPreferredDistance(selection);
    double minDist = 1.01 * selection.radius();
    double dist = (currentDistance > maxDist * 10.0) ? maxDist : currentDistance * 0.1;

    return std::max(dist, minDist);
}

void
setJourneyTimesInterpolation(Observer::JourneyParams& journey,
                             double duration,
                             double accelTime,
                             double startInter,
                             double endInter)
{
    journey.duration = duration;
    journey.accelTime = accelTime;
    if (startInter < endInter)
    {
        journey.startInterpolation = startInter;
        journey.endInterpolation = endInter;
    }
    else
    {
        journey.startInterpolation = endInter;
        journey.endInterpolation = startInter;
    }
}

void
convertJourneyToFrame(Observer::JourneyParams& jparams, const ObserverFrame& frame, double simTime)
{
    // Convert to frame coordinates
    jparams.from = frame.convertFromUniversal(jparams.from, simTime);
    jparams.initialOrientation = frame.convertFromUniversal(jparams.initialOrientation, simTime);
    jparams.to = frame.convertFromUniversal(jparams.to, simTime);
    jparams.finalOrientation = frame.convertFromUniversal(jparams.finalOrientation, simTime);
}

inline Eigen::Vector3d
computeJourneyVector(const Observer::JourneyParams& journey)
{
    return journey.to.offsetFromKm(journey.from);
}

UniversalCoord
interpolatePositionLinear(const Observer::JourneyParams& journey,
                          double t,
                          double x)
{
    Eigen::Vector3d v = computeJourneyVector(journey);
    if (v.norm() == 0.0)
        return journey.from;

    v.normalize();
    return t < 0.5
        ? journey.from.offsetKm(v * x)
        : journey.to.offsetKm(-v * x);
}

UniversalCoord
interpolatePositionGreatCircle(const Observer::JourneyParams& journey,
                               const ObserverFrame* frame,
                               double simTime,
                               double t,
                               double x)
{
    Selection centerObj = frame->getRefObject();
    if (const Body* body = centerObj.body(); body != nullptr)
    {
        if (const PlanetarySystem* system = body->getSystem(); system != nullptr)
        {
            if (Body* primaryBody = system->getPrimaryBody(); primaryBody != nullptr)
                centerObj = primaryBody;
            else
                centerObj = system->getStar();
        }
    }

    UniversalCoord ufrom  = frame->convertToUniversal(journey.from, simTime);
    UniversalCoord uto    = frame->convertToUniversal(journey.to, simTime);
    UniversalCoord origin = centerObj.getPosition(simTime);
    Eigen::Vector3d v0 = ufrom.offsetFromKm(origin);
    Eigen::Vector3d v1 = uto.offsetFromKm(origin);

    Eigen::Vector3d jv = computeJourneyVector(journey);
    if (jv.norm() == 0.0)
        return journey.from;

    x /= jv.norm();
    Eigen::Vector3d v = t < 0.5
        ? slerp(x, v0, v1)
        : slerp(x, v1, v0);

    return frame->convertFromUniversal(origin.offsetKm(v), simTime);
}

UniversalCoord
interpolatePositionCircularOrbit(const Observer::JourneyParams& journey,
                                 const ObserverFrame* frame,
                                 double simTime,
                                 double t)
{
    Selection centerObj = frame->getRefObject();

    UniversalCoord ufrom = frame->convertToUniversal(journey.from, simTime);
    UniversalCoord origin = centerObj.getPosition(simTime);

    Eigen::Vector3d v0 = ufrom.offsetFromKm(origin);

    if (computeJourneyVector(journey).norm() == 0.0)
        return journey.from;

    Eigen::Quaterniond q0(Eigen::Quaterniond::Identity());
    Eigen::Quaterniond q1 = journey.rotation1;
    UniversalCoord p = origin.offsetKm(q0.slerp(t, q1).conjugate() * v0);
    return frame->convertFromUniversal(p, simTime);
}

// Another interpolation method: accelerate exponentially, maintain a constant
// velocity for a period of time, then decelerate. The portion of the trip
// spent accelerating is controlled by the parameter journey.accelTime; a
// value of 1 means that the entire first half of the trip will be spent
// accelerating and there will be no coasting at constant velocity.
UniversalCoord
interpolatePosition(const Observer::JourneyParams& journey,
                    const ObserverFrame* frame,
                    double simTime,
                    double t)
{
    double u = t < 0.5 ? t * 2.0 : (1.0 - t) * 2.0;
    double x = u < journey.accelTime
        ? std::expm1(journey.expFactor * u) // expm1 == exp - 1
        : (std::exp(journey.expFactor * journey.accelTime) * (journey.expFactor * (u - journey.accelTime) + 1.0) - 1.0);

    switch (journey.traj)
    {
    case Observer::TrajectoryType::Linear:
        return interpolatePositionLinear(journey, t, x);

    case Observer::TrajectoryType::GreatCircle:
        return interpolatePositionGreatCircle(journey, frame, simTime, t, x);

    case Observer::TrajectoryType::CircularOrbit:
        return interpolatePositionCircularOrbit(journey, frame, simTime, t);

    default:
        assert(0);
        return journey.from;
    }
}

Eigen::Quaterniond
interpolateOrientation(const Observer::JourneyParams& journey, double t)
{
    if (t < journey.startInterpolation)
        return journey.initialOrientation;

    if (t >= journey.endInterpolation)
        return journey.finalOrientation;

    // Smooth out the interpolation to avoid jarring changes in
    // orientation
    double v;
    if (journey.traj == Observer::TrajectoryType::CircularOrbit)
    {
        // In circular orbit mode, interpolation of orientation must
        // match the interpolation of position.
        v = t;
    }
    else
    {
        v = math::square(std::sin((t - journey.startInterpolation) /
                                    (journey.endInterpolation - journey.startInterpolation) *
                                    celestia::numbers::pi * 0.5));
    }

    return journey.initialOrientation.slerp(v, journey.finalOrientation);
}

// Create the ReferenceFrame for the specified observer frame parameters.
ReferenceFrame::SharedConstPtr
createFrame(ObserverFrame::CoordinateSystem _coordSys,
            const Selection& _refObject,
            const Selection& _targetObject)
{
    switch (_coordSys)
    {
    case ObserverFrame::CoordinateSystem::Universal:
        return std::make_shared<J2000EclipticFrame>(Selection());

    case ObserverFrame::CoordinateSystem::Ecliptical:
        return std::make_shared<J2000EclipticFrame>(_refObject);

    case ObserverFrame::CoordinateSystem::Equatorial:
        return std::make_shared<BodyMeanEquatorFrame>(_refObject, _refObject);

    case ObserverFrame::CoordinateSystem::BodyFixed:
        return std::make_shared<BodyFixedFrame>(_refObject, _refObject);

    case ObserverFrame::CoordinateSystem::PhaseLock:
        return std::make_shared<TwoVectorFrame>(_refObject,
                                                FrameVector::createRelativePositionVector(_refObject, _targetObject), 1,
                                                FrameVector::createRelativeVelocityVector(_refObject, _targetObject), 2);

    case ObserverFrame::CoordinateSystem::Chase:
        return std::make_shared<TwoVectorFrame>(_refObject,
                                                FrameVector::createRelativeVelocityVector(_refObject, _refObject.parent()), 1,
                                                FrameVector::createRelativePositionVector(_refObject, _refObject.parent()), 2);

    case ObserverFrame::CoordinateSystem::PhaseLock_Old:
    {
        FrameVector rotAxis(FrameVector::createConstantVector(Eigen::Vector3d::UnitY(),
                                                              std::make_shared<BodyMeanEquatorFrame>(_refObject, _refObject)));
        return std::make_shared<TwoVectorFrame>(_refObject,
                                                FrameVector::createRelativePositionVector(_refObject, _targetObject), 3,
                                                rotAxis, 2);
    }

    case ObserverFrame::CoordinateSystem::Chase_Old:
    {
        FrameVector rotAxis(FrameVector::createConstantVector(Eigen::Vector3d::UnitY(),
                                                              std::make_shared<BodyMeanEquatorFrame>(_refObject, _refObject)));

        return std::make_shared<TwoVectorFrame>(_refObject,
                                                FrameVector::createRelativeVelocityVector(_refObject.parent(), _refObject), 3,
                                                rotAxis, 2);
    }

    case ObserverFrame::CoordinateSystem::ObserverLocal:
        // TODO: This is only used for computing up vectors for orientation; it does
        // define a proper frame for the observer position orientation.
        return std::make_shared<J2000EclipticFrame>(Selection());

    default:
        return std::make_shared<J2000EclipticFrame>(_refObject);
    }
}

} // end unnamed namespace

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
    frame(std::make_shared<ObserverFrame>())
{
    updateUniversal();
}

/*! Copy constructor. */
Observer::Observer(const Observer& o) :
    simTime(o.simTime),
    position(o.position),
    originalOrientation(o.originalOrientation),
    transformedOrientation(o.transformedOrientation),
    orientationTransform(o.orientationTransform),
    velocity(o.velocity),
    angularVelocity(o.angularVelocity),
    realTime(o.realTime),
    targetSpeed(o.targetSpeed),
    targetVelocity(o.targetVelocity),
    beginAccelTime(o.beginAccelTime),
    observerMode(o.observerMode),
    journey(o.journey),
    trackObject(o.trackObject),
    trackingOrientation(o.trackingOrientation),
    fov(o.fov),
    zoom(o.zoom),
    alternateZoom(o.alternateZoom),
    reverseFlag(o.reverseFlag),
    locationFilter(o.locationFilter),
    displayedSurface(o.displayedSurface)
{
    setFrame(o.frame);
    updateUniversal();
}

Observer& Observer::operator=(const Observer& o)
{
    simTime = o.simTime;
    position = o.position;
    originalOrientation = o.originalOrientation;
    orientationTransform = o.orientationTransform;
    transformedOrientation = o.transformedOrientation;
    velocity = o.velocity;
    angularVelocity = o.angularVelocity;
    frame = nullptr;
    realTime = o.realTime;
    targetSpeed = o.targetSpeed;
    targetVelocity = o.targetVelocity;
    beginAccelTime = o.beginAccelTime;
    observerMode = o.observerMode;
    journey = o.journey;
    trackObject = o.trackObject;
    trackingOrientation = o.trackingOrientation;
    fov = o.fov;
    zoom = o.zoom;
    alternateZoom = o.alternateZoom;
    reverseFlag = o.reverseFlag;
    locationFilter = o.locationFilter;
    displayedSurface = o.displayedSurface;

    setFrame(o.frame);
    updateUniversal();

    return *this;
}

/*! Get the current simulation time. The time returned is a Julian date,
 *  and the time standard is TDB.
 */
double
Observer::getTime() const
{
    return simTime;
}

/*! Get the current real time. The time returned is a Julian date,
 *  and the time standard is TDB.
 */
double
Observer::getRealTime() const
{
    return realTime;
}

/*! Set the simulation time (Julian date, TDB time standard)
*/
void
Observer::setTime(double jd)
{
    simTime = jd;
    updateUniversal();
}

/*! Return the position of the observer in universal coordinates. The origin
 *  The origin of this coordinate system is the Solar System Barycenter, and
 *  axes are defined by the J2000 ecliptic and equinox.
 */
UniversalCoord
Observer::getPosition() const
{
    return positionUniv;
}

/*! Set the position of the observer; position is specified in the universal
 *  coordinate system.
 */
void
Observer::setPosition(const UniversalCoord& p)
{
    positionUniv = p;
    position = frame->convertFromUniversal(p, getTime());
}

/*! Return the transformed orientation of the observer in the universal coordinate
 *  system.
 */
Eigen::Quaterniond
Observer::getOrientation() const
{
    return transformedOrientationUniv;
}

/*! Reduced precision version of `getOrientation()`
 */
Eigen::Quaternionf
Observer::getOrientationf() const
{
    return getOrientation().cast<float>();
}

/*! Reduced precision version of `setOrientation`
 */
void
Observer::setOrientation(const Eigen::Quaternionf& q)
{
    setOrientation(q.cast<double>());
}

/*! Set the transformed orientation of the observer. The orientation is specified in
 *  the universal coordinate system.
 */
void
Observer::setOrientation(const Eigen::Quaterniond& q)
{
    setOriginalOrientation(undoTransform(q));
}

/*! Reduced precision version of `setOriginalOrientation`
 */
void
Observer::setOriginalOrientation(const Eigen::Quaternionf& q)
{
    /*
    RigidTransform rt = frame.toUniversal(situation, getTime());
    rt.rotation = Quatd(q.w, q.x, q.y, q.z);
    situation = frame.fromUniversal(rt, getTime());
     */
    setOriginalOrientation(q.cast<double>());
}

/*! Set the non-transformed orientation of the observer. The orientation is specified in
 *  the universal coordinate system.
 */
void
Observer::setOriginalOrientation(const Eigen::Quaterniond& q)
{
    originalOrientationUniv = q;
    originalOrientation = frame->convertFromUniversal(q, getTime());
    updateOrientation();
}

Eigen::Matrix3d
Observer::getOrientationTransform() const
{
    return orientationTransform;
}

void
Observer::setOrientationTransform(const Eigen::Matrix3d& transform)
{
    orientationTransform = transform;
    updateOrientation();
}

/*! Get the velocity of the observer within the observer's reference frame.
 */
Eigen::Vector3d
Observer::getVelocity() const
{
    return velocity;
}

/*! Set the velocity of the observer within the observer's reference frame.
*/
void
Observer::setVelocity(const Eigen::Vector3d& v)
{
    velocity = v;
}

Eigen::Vector3d
Observer::getAngularVelocity() const
{
    return angularVelocity;
}

void
Observer::setAngularVelocity(const Eigen::Vector3d& v)
{
    angularVelocity = v;
}

double
Observer::getArrivalTime() const
{
    if (observerMode != ObserverMode::Travelling)
        return realTime;

    return journey.startTime + journey.duration;
}

/*! Tick the simulation by dt seconds. Update the observer position
 *  and orientation due to an active goto command or non-zero velocity
 *  or angular velocity.
 */
void
Observer::update(double dt, double timeScale)
{
    realTime += dt;
    simTime += (dt / 86400.0) * timeScale;

    simTime = std::clamp(simTime, minimumSimTime, maximumSimTime);

    if (observerMode == ObserverMode::Travelling)
    {
        // Compute the fraction of the trip that has elapsed; handle zero
        // durations correctly by skipping directly to the destination.
        double t = 1.0;
        if (journey.duration > 0)
            t = std::clamp((realTime - journey.startTime) / journey.duration, 0.0, 1.0);

        position = interpolatePosition(journey, frame.get(), simTime, t);

        // Spherically interpolate the orientation over the first half
        // of the journey.
        originalOrientation = undoTransform(interpolateOrientation(journey, t));

        // If the journey's complete, reset to manual control
        if (t == 1.0f)
        {
            if (journey.traj != TrajectoryType::CircularOrbit)
            {
                //situation = RigidTransform(journey.to, journey.finalOrientation);
                position = journey.to;
                originalOrientation = undoTransform(journey.finalOrientation);
            }

            observerMode = ObserverMode::Free;
            setVelocity(Eigen::Vector3d::Zero());
        }
    }

    if (getVelocity() != targetVelocity)
    {
        double t = std::clamp((realTime - beginAccelTime) / VELOCITY_CHANGE_TIME, 0.0, 1.0);
        Eigen::Vector3d v = getVelocity() * (1.0 - t) + targetVelocity * t;

        // At some threshold, we just set the velocity to zero; otherwise,
        // we'll end up with ridiculous velocities like 10^-40 m/s.
        if (v.norm() < 1.0e-12)
            v = Eigen::Vector3d::Zero();
        setVelocity(v);
    }

    // Update the position
    position = position.offsetKm(getVelocity() * dt);

    if (observerMode == ObserverMode::Free)
    {
        // Update the observer's orientation
        Eigen::Vector3d halfAV = getAngularVelocity() * 0.5;
        Eigen::Quaterniond dr = Eigen::Quaterniond(0.0, halfAV.x(), halfAV.y(), halfAV.z()) * transformedOrientation;
        Eigen::Quaterniond expectedOrientation = Eigen::Quaterniond(transformedOrientation.coeffs() + dt * dr.coeffs()).normalized();
        originalOrientation = undoTransform(expectedOrientation);
    }

    updateUniversal();

    // Update orientation for tracking--must occur after updateUniversal(), as it
    // relies on the universal position and orientation of the observer.
    if (!trackObject.empty())
    {
        Eigen::Vector3d up = getOrientation().conjugate() * Eigen::Vector3d::UnitY();
        Eigen::Vector3d viewDir = trackObject.getPosition(getTime()).offsetFromKm(getPosition()).normalized();

        setOrientation(math::LookAt<double>(Eigen::Vector3d::Zero(), viewDir, up));
    }
}

void
Observer::updateOrientation()
{
    transformedOrientationUniv = Eigen::Quaterniond(orientationTransform) * originalOrientationUniv;
    transformedOrientation = frame->convertFromUniversal(transformedOrientationUniv, getTime());
}

Eigen::Quaterniond
Observer::undoTransform(const Eigen::Quaterniond& transformed) const
{
    return Eigen::Quaterniond(orientationTransform).inverse() * transformed;
}

Selection
Observer::getTrackedObject() const
{
    return trackObject;
}

void
Observer::setTrackedObject(const Selection& sel)
{
    trackObject = sel;
}

const std::string&
Observer::getDisplayedSurface() const
{
    return displayedSurface;
}

void
Observer::setDisplayedSurface(std::string_view surf)
{
    displayedSurface = surf;
}

std::uint64_t
Observer::getLocationFilter() const
{
    return locationFilter;
}

void
Observer::setLocationFilter(std::uint64_t _locationFilter)
{
    locationFilter = _locationFilter;
}

void
Observer::reverseOrientation()
{
    setOrientation(getOrientation() * math::YRot180<double>);
    reverseFlag = !reverseFlag;
}

void
Observer::computeGotoParameters(const Selection& destination,
                                JourneyParams& jparams,
                                const Eigen::Vector3d& offset,
                                ObserverFrame::CoordinateSystem offsetCoordSys,
                                const Eigen::Vector3f& up,
                                ObserverFrame::CoordinateSystem upCoordSys)
{
    if (frame->getCoordinateSystem() == ObserverFrame::CoordinateSystem::PhaseLock)
        setFrame(ObserverFrame::CoordinateSystem::Ecliptical, destination);
    else
        setFrame(frame->getCoordinateSystem(), destination);

    UniversalCoord targetPosition = destination.getPosition(getTime());
    //Vector3d v = targetPosition.offsetFromKm(getPosition()).normalized();

    jparams.traj = TrajectoryType::Linear;
    jparams.startTime = realTime;

    // Right where we are now . . .
    jparams.from = getPosition();

    if (offsetCoordSys == ObserverFrame::CoordinateSystem::ObserverLocal)
    {
        jparams.to = targetPosition.offsetKm(transformedOrientationUniv.conjugate() * offset);
    }
    else
    {
        ObserverFrame offsetFrame(offsetCoordSys, destination);
        jparams.to = targetPosition.offsetKm(offsetFrame.getFrame()->getOrientation(getTime()).conjugate() * offset);
    }

    Eigen::Vector3d upd = up.cast<double>();
    if (upCoordSys == ObserverFrame::CoordinateSystem::ObserverLocal)
    {
        upd = transformedOrientationUniv.conjugate() * upd;
    }
    else
    {
        ObserverFrame upFrame(upCoordSys, destination);
        upd = upFrame.getFrame()->getOrientation(getTime()).conjugate() * upd;
    }

    jparams.initialOrientation = getOrientation();
    Eigen::Vector3d focus = targetPosition.offsetFromKm(jparams.to);
    jparams.finalOrientation = math::LookAt<double>(Eigen::Vector3d::Zero(), focus, upd);

    double distance = jparams.from.offsetFromKm(jparams.to).norm() / 2.0;
    jparams.expFactor = math::solve_bisection(TravelExpFunc(distance, jparams.accelTime),
                                              0.0001, 100.0,
                                              1e-10).first;

    convertJourneyToFrame(jparams, *frame, getTime());
}

void
Observer::computeGotoParametersGC(const Selection& destination,
                                  JourneyParams& jparams,
                                  const Eigen::Vector3d& offset,
                                  ObserverFrame::CoordinateSystem offsetCoordSys,
                                  const Eigen::Vector3f& up,
                                  ObserverFrame::CoordinateSystem upCoordSys,
                                  const Selection& centerObj)
{
    setFrame(frame->getCoordinateSystem(), destination);

    UniversalCoord targetPosition = destination.getPosition(getTime());
    //Vector3d v = targetPosition.offsetFromKm(getPosition()).normalized();

    jparams.traj = TrajectoryType::GreatCircle;
    jparams.startTime = realTime;

    jparams.centerObject = centerObj;

    // Right where we are now . . .
    jparams.from = getPosition();

    ObserverFrame offsetFrame(offsetCoordSys, destination);
    Eigen::Vector3d offsetTransformed = offsetFrame.getFrame()->getOrientation(getTime()).conjugate() * offset;

    jparams.to = targetPosition.offsetKm(offsetTransformed);

    Eigen::Vector3d upd = up.cast<double>();
    if (upCoordSys == ObserverFrame::CoordinateSystem::ObserverLocal)
    {
        upd = transformedOrientationUniv.conjugate() * upd;
    }
    else
    {
        ObserverFrame upFrame(upCoordSys, destination);
        upd = upFrame.getFrame()->getOrientation(getTime()).conjugate() * upd;
    }

    jparams.initialOrientation = getOrientation();
    Eigen::Vector3d focus = targetPosition.offsetFromKm(jparams.to);
    jparams.finalOrientation = math::LookAt<double>(Eigen::Vector3d::Zero(), focus, upd);

    double distance = jparams.from.offsetFromKm(jparams.to).norm() / 2.0;
    jparams.expFactor = math::solve_bisection(TravelExpFunc(distance, jparams.accelTime),
                                              0.0001, 100.0,
                                              1e-10).first;

    convertJourneyToFrame(jparams, *frame, getTime());
}

void
Observer::computeCenterParameters(const Selection& destination,
                                  JourneyParams& jparams,
                                  double centerTime) const
{
    UniversalCoord targetPosition = destination.getPosition(getTime());

    jparams.duration = centerTime;
    jparams.startTime = realTime;
    jparams.traj = TrajectoryType::Linear;

    // Don't move through space, just rotate the camera
    jparams.from = getPosition();
    jparams.to = jparams.from;

    Eigen::Vector3d up = getOrientation().conjugate() * Eigen::Vector3d::UnitY();

    jparams.initialOrientation = getOrientation();
    Eigen::Vector3d focus = targetPosition.offsetFromKm(jparams.to);
    jparams.finalOrientation = math::LookAt<double>(Eigen::Vector3d::Zero(), focus, up);
    jparams.startInterpolation = 0;
    jparams.endInterpolation   = 1;

    jparams.accelTime = 0.5;
    jparams.expFactor = 0;

    convertJourneyToFrame(jparams, *frame, getTime());
}

void
Observer::computeCenterCOParameters(const Selection& destination,
                                    JourneyParams& jparams,
                                    double centerTime) const
{
    jparams.duration = centerTime;
    jparams.startTime = realTime;
    jparams.traj = TrajectoryType::CircularOrbit;

    jparams.centerObject = frame->getRefObject();
    jparams.expFactor = 0.5;

    Eigen::Vector3d v = destination.getPosition(getTime()).offsetFromKm(getPosition()).normalized();
    Eigen::Vector3d w = getOrientation().conjugate() * -Eigen::Vector3d::UnitZ();

    Selection centerObj = frame->getRefObject();
    UniversalCoord centerPos = centerObj.getPosition(getTime());
    //UniversalCoord targetPosition = destination.getPosition(getTime());

    auto q = Eigen::Quaterniond::FromTwoVectors(v, w);

    jparams.from = getPosition();
    jparams.to = centerPos.offsetKm(q.conjugate() * getPosition().offsetFromKm(centerPos));
    jparams.initialOrientation = getOrientation();
    jparams.finalOrientation = getOrientation() * q;

    jparams.startInterpolation = 0.0;
    jparams.endInterpolation = 1.0;

    jparams.rotation1 = q;

    convertJourneyToFrame(jparams, *frame, getTime());
}

/*! Center the selection by moving on a circular orbit arround
* the primary body (refObject).
*/
void
Observer::centerSelectionCO(const Selection& selection, double centerTime)
{
    if (!selection.empty() && !frame->getRefObject().empty())
    {
        computeCenterCOParameters(selection, journey, centerTime);
        observerMode = ObserverMode::Travelling;
    }
}

Observer::ObserverMode
Observer::getMode() const
{
    return observerMode;
}

void
Observer::setMode(Observer::ObserverMode mode)
{
    observerMode = mode;
}

// Private method to convert coordinates when a new observer frame is set.
// Universal coordinates remain the same. All frame coordinates get updated, including
// the goto parameters.
void
Observer::convertFrameCoordinates(const ObserverFrame::SharedConstPtr &newFrame)
{
    double now = getTime();

    // Universal coordinates don't change.
    // Convert frame coordinates to the new frame.
    position = newFrame->convertFromUniversal(positionUniv, now);
    originalOrientation = newFrame->convertFromUniversal(originalOrientationUniv, now);
    transformedOrientation = newFrame->convertFromUniversal(transformedOrientationUniv, now);

    // Convert goto parameters to the new frame
    journey.from               = ObserverFrame::convert(frame, newFrame, journey.from, now);
    journey.initialOrientation = ObserverFrame::convert(frame, newFrame, journey.initialOrientation, now);
    journey.to                 = ObserverFrame::convert(frame, newFrame, journey.to, now);
    journey.finalOrientation   = ObserverFrame::convert(frame, newFrame, journey.finalOrientation, now);
}

/*! Set the observer's reference frame. The position of the observer in
*   universal coordinates will not change.
*/
void
Observer::setFrame(ObserverFrame::CoordinateSystem cs, const Selection& refObj, const Selection& targetObj)
{
    auto newFrame = std::make_shared<ObserverFrame>(cs, refObj, targetObj);
    convertFrameCoordinates(newFrame);
    frame = newFrame;
}

/*! Set the observer's reference frame. The position of the observer in
*  universal coordinates will not change.
*/
void
Observer::setFrame(ObserverFrame::CoordinateSystem cs, const Selection& refObj)
{
    setFrame(cs, refObj, Selection());
}

/*! Set the observer's reference frame. The position of the observer in
 *  universal coordinates will not change.
 */
void
Observer::setFrame(const ObserverFrame::SharedConstPtr& f)
{
    if (frame == f)
        return;

    if (frame)
        convertFrameCoordinates(f);
    frame = f;
}

/*! Get the current reference frame for the observer.
 */
const ObserverFrame::SharedConstPtr&
Observer::getFrame() const
{
    return frame;
}

/*! Rotate the observer about its center.
 */
void
Observer::rotate(const Eigen::Quaternionf& q)
{
    originalOrientation = undoTransform(q.cast<double>() * transformedOrientation);
    updateUniversal();
}

/*! Orbit around the reference object (if there is one.)  This involves changing
 *  both the observer's position and orientation. If there is no current center
 *  object, the specified selection will be used as the center of rotation, and
 *  the observer reference frame will be modified.
 */
void
Observer::orbit(const Selection& selection, const Eigen::Quaternionf& q)
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
        Eigen::Vector3d v = position.offsetFromKm(focusPosition);

        Eigen::Quaterniond qd = q.cast<double>();

        // To give the right feel for rotation, we want to premultiply
        // the current orientation by q.  However, because of the order in
        // which we apply transformations later on, we can't pre-multiply.
        // To get around this, we compute a rotation q2 such
        // that q1 * r = r * q2.
        Eigen::Quaterniond qd2 = transformedOrientation.conjugate() * qd * transformedOrientation;
        qd2.normalize();

        // Roundoff errors will accumulate and cause the distance between
        // viewer and focus to drift unless we take steps to keep the
        // length of v constant.
        double distance = v.norm();
        v = qd2.conjugate() * v;
        v = v.normalized() * distance;

        originalOrientation = undoTransform(transformedOrientation * qd2);
        position = focusPosition.offsetKm(v);
        updateUniversal();
    }
}

/*! Orbit around the reference object (if there is one.)  rotating the object with intersection point from
 *  a direction to another. If there is no intersection point, return false.
 */
bool
Observer::orbit(const Selection& selection, const Eigen::Vector3f &from, const Eigen::Vector3f &to)
{
    Selection center = frame->getRefObject();
    if (center.empty())
    {
        if (selection.empty())
            return false;
        center = selection;
    }

    double radius = center.radius();
    if (radius <= 0.0)
        return false;

    // Get the focus position (center of rotation) in frame
    // coordinates; in order to make this function work in all
    // frames of reference, it's important to work in frame
    // coordinates.
    UniversalCoord focusPosition = center.getPosition(getTime());
    focusPosition = frame->convertFromUniversal(focusPosition, getTime());

    Eigen::Vector3d objectCenter = focusPosition.offsetFromKm(position);

    // Get the rays adjusted to orientation
    Eigen::Vector3d transformedFrom = getOrientation().conjugate() * from.cast<double>();
    Eigen::Vector3d transformedTo = getOrientation().conjugate() * to.cast<double>();

    // Find intersections for the rays, if no intersection, return false
    math::Sphered sphere { objectCenter, radius };
    auto orbitStartPosition = getNearIntersectionPoint(Eigen::ParametrizedLine<double, 3>(Eigen::Vector3d::Zero(), transformedFrom), sphere);
    if (!orbitStartPosition.has_value())
        return false;

    auto orbitEndPosition = getNearIntersectionPoint(Eigen::ParametrizedLine<double, 3>(Eigen::Vector3d::Zero(), transformedTo), sphere);
    if (!orbitEndPosition.has_value())
        return false;

    orbit(selection,
          Eigen::Quaterniond::FromTwoVectors(transformedOrientation * (orbitStartPosition.value() - objectCenter),
                                             transformedOrientation * (orbitEndPosition.value() - objectCenter)).cast<float>());
    return true;
}

/*! Exponential camera dolly--move toward or away from the selected object
 * at a rate dependent on the observer's distance from the object.
 */
void
Observer::changeOrbitDistance(const Selection& selection, float d)
{
    scaleOrbitDistance(selection, std::exp(-d), std::nullopt);
}

void
Observer::scaleOrbitDistance(const Selection& selection, float scale, const std::optional<Eigen::Vector3f> &focus)
{
    Selection center = frame->getRefObject();
    if (center.empty())
    {
        if (selection.empty())
            return;

        center = selection;
        setFrame(frame->getCoordinateSystem(), center);
    }

    UniversalCoord centerPosition = center.getPosition(getTime());

    // Determine distance and direction to the selected object
    auto currentPosition = getPosition();
    Eigen::Vector3d positionFromCenter = currentPosition.offsetFromKm(centerPosition);
    double currentDistance = positionFromCenter.norm();

    double minOrbitDistance = center.radius();
    if (currentDistance < minOrbitDistance)
        minOrbitDistance = currentDistance * 0.5;

    double span = currentDistance - minOrbitDistance;
    double newDistance = minOrbitDistance + span / static_cast<double>(scale);
    positionFromCenter *= (newDistance / currentDistance);

    std::optional<UniversalCoord> controlPoint1 = std::nullopt;
    std::optional<Eigen::Vector3d> focusRay = std::nullopt;
    if (focus.has_value())
    {
        focusRay = getOrientation().conjugate() * focus.value().cast<double>();
        // The control points are the intersection points of the original focus ray and
        // the sphere with radius = span (distance to center - min distance) before and
        // after the distance change
        controlPoint1 = currentPosition.offsetKm(focusRay.value() * span);
    }

    auto newPosition = centerPosition.offsetKm(positionFromCenter);
    position = frame->convertFromUniversal(newPosition, getTime());
    updateUniversal();

    if (controlPoint1.has_value())
    {
        auto controlPoint2 = newPosition.offsetKm(focusRay.value() * (newDistance - minOrbitDistance));
        orbit(selection,
              Eigen::Quaterniond::FromTwoVectors(transformedOrientation * controlPoint1.value().offsetFromKm(centerPosition),
                                                 transformedOrientation * controlPoint2.offsetFromKm(centerPosition)).cast<float>());
    }
}

void
Observer::setTargetSpeed(float s)
{
    targetSpeed = s;
    Eigen::Vector3d v;

    if (reverseFlag)
        s = -s;
    if (trackObject.empty())
    {
        trackingOrientation = getOrientation();
        // Generate vector for velocity using current orientation
        // and specified speed.
        v = getOrientation().conjugate() * Eigen::Vector3d(0, 0, -s);
    }
    else
    {
        // Use tracking orientation vector to generate target velocity
        v = trackingOrientation.conjugate() * Eigen::Vector3d(0, 0, -s);
    }

    targetVelocity = v;
    initialVelocity = getVelocity();
    beginAccelTime = realTime;
}

float
Observer::getTargetSpeed() const
{
    return static_cast<float>(targetSpeed);
}

void
Observer::gotoJourney(const JourneyParams& params)
{
    journey = params;
    double distance = journey.from.offsetFromKm(journey.to).norm() / 2.0;
    journey.expFactor = math::solve_bisection(TravelExpFunc(distance, journey.accelTime),
                                              0.0001, 100.0,
                                              1e-10).first;
    journey.startTime = realTime;
    observerMode = ObserverMode::Travelling;
}

void
Observer::gotoSelection(const Selection& selection,
                        double gotoTime,
                        const Eigen::Vector3f& up,
                        ObserverFrame::CoordinateSystem upFrame)
{
    gotoSelection(selection, gotoTime, 0.0, 0.5, AccelerationTime, up, upFrame);
}

void
Observer::gotoSelection(const Selection& selection,
                        double gotoTime,
                        double startInter,
                        double endInter,
                        double accelTime,
                        const Eigen::Vector3f& up,
                        ObserverFrame::CoordinateSystem upFrame)
{
    if (selection.empty())
        return;

    UniversalCoord pos = selection.getPosition(getTime());
    Eigen::Vector3d v = pos.offsetFromKm(getPosition());
    double distance = v.norm();

    double orbitDistance = getOrbitDistance(selection, distance);

    setJourneyTimesInterpolation(journey, gotoTime, accelTime, startInter, endInter);
    computeGotoParameters(selection, journey,
                          v * -(orbitDistance / distance),
                          ObserverFrame::CoordinateSystem::Universal,
                          up, upFrame);

    observerMode = ObserverMode::Travelling;
}

/*! Like normal goto, except we'll follow a great circle trajectory.  Useful
 *  for travelling between surface locations, where we'd rather not go straight
 *  through the middle of a planet.
 */
void
Observer::gotoSelectionGC(const Selection& selection,
                          double gotoTime,
                          const Eigen::Vector3f& up,
                          ObserverFrame::CoordinateSystem upFrame)
{
    if (selection.empty())
        return;

    Selection centerObj = selection.parent();

    UniversalCoord pos = selection.getPosition(getTime());
    Eigen::Vector3d v = pos.offsetFromKm(centerObj.getPosition(getTime()));
    double distanceToCenter = v.norm();
    Eigen::Vector3d viewVec = pos.offsetFromKm(getPosition());
    double orbitDistance = getOrbitDistance(selection, viewVec.norm());

    if (selection.location() != nullptr)
    {
        Selection parent = selection.parent();
        double maintainDist = getPreferredDistance(parent);
        Eigen::Vector3d parentPos = parent.getPosition(getTime()).offsetFromKm(getPosition());
        double parentDist = parentPos.norm() - parent.radius();

        if (parentDist <= maintainDist && parentDist > orbitDistance)
            orbitDistance = parentDist;
    }

    setJourneyTimesInterpolation(journey, gotoTime, AccelerationTime, StartInterpolation, EndInterpolation);
    computeGotoParametersGC(selection, journey,
                            v * (orbitDistance / distanceToCenter),
                            ObserverFrame::CoordinateSystem::Universal,
                            up, upFrame,
                            centerObj);

    observerMode = ObserverMode::Travelling;
}

void
Observer::gotoSelection(const Selection& selection,
                        double gotoTime,
                        double distance,
                        const Eigen::Vector3f& up,
                        ObserverFrame::CoordinateSystem upFrame)
{
    if (selection.empty())
        return;

    UniversalCoord pos = selection.getPosition(getTime());
    // The destination position lies along the line between the current
    // position and the star
    Eigen::Vector3d v = pos.offsetFromKm(getPosition());
    v.normalize();

    setJourneyTimesInterpolation(journey, gotoTime, AccelerationTime, StartInterpolation, EndInterpolation);
    computeGotoParameters(selection, journey,
                          v * -distance, ObserverFrame::CoordinateSystem::Universal,
                          up, upFrame);
    observerMode = ObserverMode::Travelling;
}

void
Observer::gotoSelectionGC(const Selection& selection,
                          double gotoTime,
                          double distance,
                          const Eigen::Vector3f& up,
                          ObserverFrame::CoordinateSystem upFrame)
{
    if (selection.empty())
        return;

    Selection centerObj = selection.parent();

    UniversalCoord pos = selection.getPosition(getTime());
    Eigen::Vector3d v = pos.offsetFromKm(centerObj.getPosition(getTime()));
    v.normalize();

    // The destination position lies along a line extended from the center
    // object to the target object
    setJourneyTimesInterpolation(journey, gotoTime, AccelerationTime, StartInterpolation, EndInterpolation);
    computeGotoParametersGC(selection, journey,
                            v * -distance, ObserverFrame::CoordinateSystem::Universal,
                            up, upFrame,
                            centerObj);

    observerMode = ObserverMode::Travelling;
}

/** Make the observer travel to the specified planetocentric coordinates.
 *  @param selection the central object
 *  @param gotoTime travel time in seconds of real time
 *  @param distance the distance from the center (in kilometers)
 *  @param longitude longitude in radians
 *  @param latitude latitude in radians
 */
void
Observer::gotoSelectionLongLat(const Selection& selection,
                               double gotoTime,
                               double distance,
                               float longitude,
                               float latitude,
                               const Eigen::Vector3f& up)
{
    if (selection.empty())
        return;

    double sphi;
    double cphi;
    math::sincos(-latitude + celestia::numbers::pi * 0.5, sphi, cphi);
    double stheta;
    double ctheta;
    math::sincos(static_cast<double>(longitude), stheta, ctheta);
    double x = ctheta * sphi;
    double y = cphi;
    double z = -stheta * sphi;
    setJourneyTimesInterpolation(journey, gotoTime, AccelerationTime, StartInterpolation, EndInterpolation);
    computeGotoParameters(selection, journey,
                          Eigen::Vector3d(x, y, z) * distance,
                          ObserverFrame::CoordinateSystem::BodyFixed,
                          up, ObserverFrame::CoordinateSystem::BodyFixed);

    observerMode = ObserverMode::Travelling;
}

void
Observer::gotoLocation(const UniversalCoord& toPosition,
                       const Eigen::Quaterniond& toOrientation,
                       double duration)
{
    journey.startTime = realTime;
    journey.duration = duration;

    journey.from = position;
    journey.initialOrientation = transformedOrientation;
    journey.to = toPosition;
    journey.finalOrientation = toOrientation;

    journey.startInterpolation = StartInterpolation;
    journey.endInterpolation   = EndInterpolation;

    journey.accelTime = AccelerationTime;
    double distance = journey.from.offsetFromKm(journey.to).norm() / 2.0;
    journey.expFactor = math::solve_bisection(TravelExpFunc(distance, journey.accelTime),
                                              0.0001, 100.0,
                                              1e-10).first;

    observerMode = ObserverMode::Travelling;
}

void
Observer::getSelectionLongLat(const Selection& selection,
                              double& distance,
                              double& longitude,
                              double& latitude) const
{
    if (selection.empty())
        return;

    // Compute distance (km) and lat/long (degrees) of observer with
    // respect to currently selected object.
    ObserverFrame selFrame(ObserverFrame::CoordinateSystem::BodyFixed, selection);
    Eigen::Vector3d bfPos = selFrame.convertFromUniversal(positionUniv, getTime()).offsetFromKm(UniversalCoord::Zero());

    // Convert from Celestia's coordinate system
    double x = bfPos.x();
    double y = -bfPos.z();
    double z = bfPos.y();

    distance = bfPos.norm();
    longitude = math::radToDeg(std::atan2(y, x));
    latitude = math::radToDeg(celestia::numbers::pi * 0.5 - std::acos(std::clamp(z / distance, -1.0, 1.0)));
}

void
Observer::gotoSurface(const Selection& sel, double duration)
{
    Eigen::Vector3d v = getPosition().offsetFromKm(sel.getPosition(getTime()));
    v.normalize();

    Eigen::Vector3d viewDir = transformedOrientationUniv.conjugate() * -Eigen::Vector3d::UnitZ();
    Eigen::Vector3d up      = transformedOrientationUniv.conjugate() * Eigen::Vector3d::UnitY();
    Eigen::Quaterniond q    = transformedOrientationUniv;
    if (v.dot(viewDir) < 0.0)
    {
        q = math::LookAt<double>(Eigen::Vector3d::Zero(), up, v);
    }

    ObserverFrame selFrame(ObserverFrame::CoordinateSystem::BodyFixed, sel);
    UniversalCoord bfPos = selFrame.convertFromUniversal(positionUniv, getTime());
    q = selFrame.convertFromUniversal(q, getTime());

    double height = 1.0001 * sel.radius();
    Eigen::Vector3d dir = bfPos.offsetFromKm(UniversalCoord::Zero()).normalized() * height;
    UniversalCoord nearSurfacePoint = UniversalCoord::Zero().offsetKm(dir);

    gotoLocation(nearSurfacePoint, q, duration);
}

void
Observer::cancelMotion()
{
    observerMode = ObserverMode::Free;
}

void
Observer::centerSelection(const Selection& selection, double centerTime)
{
    if (selection.empty())
        return;

    computeCenterParameters(selection, journey, centerTime);
    observerMode = ObserverMode::Travelling;
}

void
Observer::follow(const Selection& selection)
{
    setFrame(ObserverFrame::CoordinateSystem::Ecliptical, selection);
}

void
Observer::geosynchronousFollow(const Selection& selection)
{
    if (selection.body() != nullptr ||
        selection.location() != nullptr ||
        selection.star() != nullptr)
    {
        setFrame(ObserverFrame::CoordinateSystem::BodyFixed, selection);
    }
}

void
Observer::phaseLock(const Selection& selection)
{
    Selection refObject = frame->getRefObject();

    if (selection != refObject)
    {
        if (refObject.body() != nullptr || refObject.star() != nullptr)
            setFrame(ObserverFrame::CoordinateSystem::PhaseLock, refObject, selection);
    }
    else if (selection.body() != nullptr)
    {
        // Selection and reference object are identical, so the frame is undefined.
        // We'll instead use the object's star as the target object.
        setFrame(ObserverFrame::CoordinateSystem::PhaseLock,
                 selection,
                 selection.body()->getSystem()->getStar());
    }
}

void
Observer::chase(const Selection& selection)
{
    if (selection.body() != nullptr || selection.star() != nullptr)
    {
        setFrame(ObserverFrame::CoordinateSystem::Chase, selection);
    }
}

float
Observer::getFOV() const
{
    return fov;
}

void
Observer::setFOV(float _fov)
{
    fov = _fov;
}

float
Observer::getZoom() const
{
    return zoom;
}

void
Observer::setZoom(float _zoom)
{
    zoom = _zoom;
}

float
Observer::getAlternateZoom() const
{
    return alternateZoom;
}

void
Observer::setAlternateZoom(float _alternateZoom)
{
    alternateZoom = _alternateZoom;
}

// Internal method to update the position and orientation of the observer in
// universal coordinates.
void
Observer::updateUniversal()
{
    UniversalCoord newPositionUniv = frame->convertToUniversal(position, simTime);
    if (newPositionUniv.isOutOfBounds())
    {
        // New position would take us out of range of the simulation. At this
        // point the positionUniv has not been updated, so will contain a position
        // within the bounds of the simulation. To make the coordinates consistent,
        // we recompute the frame-local position from positionUniv.
        position = frame->convertFromUniversal(positionUniv, simTime);
    }
    else
    {
        // We're in bounds of the simulation, so update the universal coordinate
        // to match the frame-local position.
        positionUniv = newPositionUniv;
    }

    originalOrientationUniv = frame->convertToUniversal(originalOrientation, simTime);
    updateOrientation();
}

/*! Create the default 'universal' observer frame, with a center at the
 *  Solar System barycenter and coordinate axes of the J200Ecliptic
 *  reference frame.
 */
ObserverFrame::ObserverFrame() :
    coordSys(CoordinateSystem::Universal),
    frame(createFrame(CoordinateSystem::Universal, Selection(), Selection()))
{
}


/*! Create a new frame with the specified coordinate system and
 *  reference object. The targetObject is only needed for phase
 *  lock frames; the argument is ignored for other frames.
 */
ObserverFrame::ObserverFrame(CoordinateSystem _coordSys,
                             const Selection& _refObject,
                             const Selection& _targetObject) :
    coordSys(_coordSys),
    frame(nullptr),
    targetObject(_targetObject)
{
    frame = createFrame(_coordSys, _refObject, _targetObject);
}


/*! Create a new ObserverFrame with the specified reference frame.
 *  The coordinate system of this frame will be marked as unknown.
 */
ObserverFrame::ObserverFrame(const ReferenceFrame::SharedConstPtr &f) :
    coordSys(CoordinateSystem::Unknown),
    frame(f)
{
}

ObserverFrame::~ObserverFrame() = default;

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

const ReferenceFrame::SharedConstPtr&
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

Eigen::Quaterniond
ObserverFrame::convertFromUniversal(const Eigen::Quaterniond& q, double tjd) const
{
    return frame->convertFromUniversal(q, tjd);
}

Eigen::Quaterniond
ObserverFrame::convertToUniversal(const Eigen::Quaterniond& q, double tjd) const
{
    return frame->convertToUniversal(q, tjd);
}

/*! Convert a position from one frame to another.
 */
UniversalCoord
ObserverFrame::convert(const ObserverFrame::SharedConstPtr& fromFrame,
                       const ObserverFrame::SharedConstPtr& toFrame,
                       const UniversalCoord& uc,
                       double t)
{
    // Perform the conversion fromFrame -> universal -> toFrame
    return toFrame->convertFromUniversal(fromFrame->convertToUniversal(uc, t), t);
}

/*! Convert an orientation from one frame to another.
*/
Eigen::Quaterniond
ObserverFrame::convert(const ObserverFrame::SharedConstPtr& fromFrame,
                       const ObserverFrame::SharedConstPtr& toFrame,
                       const Eigen::Quaterniond& q,
                       double t)
{
    // Perform the conversion fromFrame -> universal -> toFrame
    return toFrame->convertFromUniversal(fromFrame->convertToUniversal(q, t), t);
}
