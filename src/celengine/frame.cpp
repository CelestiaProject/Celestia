// frame.cpp
//
// Reference frame base class.
//
// Copyright (C) 2003-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "frame.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include <celastro/date.h>
#include <celephem/rotation.h>
#include <celmath/geomutil.h>
#include <celutil/r128.h>
#include "body.h"
#include "location.h"
#include "selection.h"
#include "star.h"
#include "univcoord.h"

namespace astro = celestia::astro;
namespace math = celestia::math;

namespace
{

// Velocity for two-vector frames is computed by differentiation; units are
// Julian days.
constexpr double ANGULAR_VELOCITY_DIFF_DELTA = 1.0 / 1440.0;

// The sine of minimum angle between the primary and secondary vectors in a
// TwoVector frame
constexpr double Tolerance = 1.0e-6;

const Eigen::Quaterniond J2000Orientation{ Eigen::AngleAxis<double>(astro::J2000Obliquity, Eigen::Vector3d::UnitX()) };

// High-precision rotation using 64.64 fixed point path. Rotate uc by
// the rotation specified by unit quaternion q.
UniversalCoord
rotate(const UniversalCoord& uc, const Eigen::Quaterniond& q)
{
    Eigen::Matrix3d r = q.toRotationMatrix();
    UniversalCoord uc1;

    uc1.x = uc.x * R128(r(0, 0)) + uc.y * R128(r(1, 0)) + uc.z * R128(r(2, 0));
    uc1.y = uc.x * R128(r(0, 1)) + uc.y * R128(r(1, 1)) + uc.z * R128(r(2, 1));
    uc1.z = uc.x * R128(r(0, 2)) + uc.y * R128(r(1, 2)) + uc.z * R128(r(2, 2));

    return uc1;
}

} // end unnamed namespace

/*** ReferenceFrame ***/

ReferenceFrame::ReferenceFrame(Selection center) :
    centerObject(center)
{
}

/*! Convert from universal coordinates to frame coordinates. This method
 *  uses 64.64 fixed point arithmetic in conversion, and is thus /much/ slower
 *  than convertFromAstrocentric(), which works with double precision
 *  floating points values. For cases when the bodies are all in the same
 *  solar system, convertFromAstrocentric() should be used.
 */
UniversalCoord
ReferenceFrame::convertFromUniversal(const UniversalCoord& uc, double tjd) const
{
    UniversalCoord uc1 = uc - centerObject.getPosition(tjd);
    return rotate(uc1, getOrientation(tjd).conjugate());
}

Eigen::Quaterniond
ReferenceFrame::convertFromUniversal(const Eigen::Quaterniond& q, double tjd) const
{
    return q * getOrientation(tjd).conjugate();
}

/*! Convert from local coordinates to universal coordinates. This method
 *  uses 64.64 fixed point arithmetic in conversion, and is thus /much/ slower
 *  than convertFromAstrocentric(), which works with double precision
 *  floating points values. For cases when the bodies are all in the same
 *  solar system, convertFromAstrocentric() should be used.
 *
 *  To get the position of a solar system object in universal coordinates,
 *  it usually suffices to get the astrocentric position and then add that
 *  to the position of the star in universal coordinates. This avoids any
 *  expensive high-precision multiplication.
 */
UniversalCoord
ReferenceFrame::convertToUniversal(const UniversalCoord& uc, double tjd) const
{
    return centerObject.getPosition(tjd) + rotate(uc, getOrientation(tjd));
}

Eigen::Quaterniond
ReferenceFrame::convertToUniversal(const Eigen::Quaterniond& q, double tjd) const
{
    return q * getOrientation(tjd);
}

Eigen::Vector3d
ReferenceFrame::convertToAstrocentric(const Eigen::Vector3d& p, double tjd) const
{
    switch (centerObject.getType())
    {
    case SelectionType::Body:
        {
            Eigen::Vector3d center = centerObject.body()->getAstrocentricPosition(tjd);
            return center + getOrientation(tjd).conjugate() * p;
        }

    case SelectionType::Star:
        return getOrientation(tjd).conjugate() * p;

    default:
        // DSO/Locations not currently supported
        return Eigen::Vector3d::Zero();
    }
}

/*! Return the object that is the defined origin of the reference frame.
 */
Selection
ReferenceFrame::getCenter() const
{
    return centerObject;
}

Eigen::Vector3d
ReferenceFrame::getAngularVelocity(double tjd) const
{
    Eigen::Quaterniond q0 = getOrientation(tjd);
    Eigen::Quaterniond q1 = getOrientation(tjd + ANGULAR_VELOCITY_DIFF_DELTA);
    Eigen::Quaterniond dq = q0.conjugate() * q1;

    if (std::abs(dq.w()) > 0.99999999)
        return Eigen::Vector3d::Zero();
    return dq.vec().normalized() * (2.0 * std::acos(dq.w()) / ANGULAR_VELOCITY_DIFF_DELTA);
}

unsigned int
ReferenceFrame::nestingDepth(unsigned int maxDepth) const
{
    return nestingDepth(0, maxDepth);
}

unsigned int
ReferenceFrame::getFrameDepth(const Selection& sel,
                              unsigned int depth,
                              unsigned int maxDepth,
                              FrameType frameType)
{
    if (depth > maxDepth)
        return depth;

    const Body* body = sel.body();
    if (sel.location() != nullptr)
        body = sel.location()->getParentBody();

    if (body == nullptr)
        return depth;

    unsigned int frameDepth;
    // TODO: need to check /all/ orbit frames of body
    switch (frameType)
    {
    case FrameType::PositionFrame:
        if (const ReferenceFrame* orbitFrame = body->getOrbitFrame(0.0).get();
            orbitFrame != nullptr)
        {
            frameDepth = orbitFrame->nestingDepth(depth + 1, maxDepth);
            if (frameDepth > maxDepth)
                return frameDepth;
        }
        break;

    case FrameType::OrientationFrame:
        if (const ReferenceFrame* bodyFrame = body->getBodyFrame(0.0).get();
            bodyFrame != nullptr)
        {
            frameDepth = bodyFrame->nestingDepth(depth + 1, maxDepth);
        }
        break;

    default:
        assert(0);
        return depth;
    }

    return std::max(frameDepth, depth);
}

/*** J2000EclipticFrame ***/

J2000EclipticFrame::J2000EclipticFrame(Selection center) :
    ReferenceFrame(center)
{
}

bool
J2000EclipticFrame::isInertial() const
{
    return true;
}

unsigned int
J2000EclipticFrame::nestingDepth(unsigned int depth,
                                 unsigned int maxDepth) const
{
    return getFrameDepth(getCenter(), depth, maxDepth, FrameType::PositionFrame);
}

/*** J2000EquatorFrame ***/

J2000EquatorFrame::J2000EquatorFrame(Selection center) :
    ReferenceFrame(center)
{
}

Eigen::Quaterniond
J2000EquatorFrame::getOrientation(double /* tjd */) const
{
    return J2000Orientation;
}

bool
J2000EquatorFrame::isInertial() const
{
    return true;
}

unsigned int
J2000EquatorFrame::nestingDepth(unsigned int depth,
                                unsigned int maxDepth) const
{
    return getFrameDepth(getCenter(), depth, maxDepth, FrameType::PositionFrame);
}

/*** BodyFixedFrame ***/

BodyFixedFrame::BodyFixedFrame(Selection center, Selection obj) :
    ReferenceFrame(center),
    fixObject(obj)
{
}

Eigen::Quaterniond
BodyFixedFrame::getOrientation(double tjd) const
{
    switch (fixObject.getType())
    {
    case SelectionType::Body:
        return math::YRot180<double> * fixObject.body()->getEclipticToBodyFixed(tjd);
    case SelectionType::Star:
        return math::YRot180<double> * fixObject.star()->getRotationModel()->orientationAtTime(tjd);
    case SelectionType::Location:
        if (fixObject.location()->getParentBody())
            return math::YRot180<double> * fixObject.location()->getParentBody()->getEclipticToBodyFixed(tjd);
        else
            return math::YRot180<double>;
    default:
        return math::YRot180<double>;
    }
}

Eigen::Vector3d
BodyFixedFrame::getAngularVelocity(double tjd) const
{
    switch (fixObject.getType())
    {
    case SelectionType::Body:
        return fixObject.body()->getAngularVelocity(tjd);
    case SelectionType::Star:
        return fixObject.star()->getRotationModel()->angularVelocityAtTime(tjd);
    case SelectionType::Location:
        if (fixObject.location()->getParentBody())
            return fixObject.location()->getParentBody()->getAngularVelocity(tjd);
        else
            return Eigen::Vector3d::Zero();
    default:
        return Eigen::Vector3d::Zero();
    }
}

bool
BodyFixedFrame::isInertial() const
{
    return false;
}

unsigned int
BodyFixedFrame::nestingDepth(unsigned int depth,
                             unsigned int maxDepth) const
{
    unsigned int n = getFrameDepth(getCenter(), depth, maxDepth, FrameType::PositionFrame);
    if (n > maxDepth)
        return n;

    unsigned int m = getFrameDepth(fixObject, depth, maxDepth, FrameType::OrientationFrame);
    return std::max(m, n);
}

/*** BodyMeanEquatorFrame ***/

BodyMeanEquatorFrame::BodyMeanEquatorFrame(Selection center,
                                           Selection obj) :
    ReferenceFrame(center),
    equatorObject(obj),
    freezeEpoch(astro::J2000),
    isFrozen(false)
{
}

BodyMeanEquatorFrame::BodyMeanEquatorFrame(Selection center,
                                           Selection obj,
                                           double freeze) :
    ReferenceFrame(center),
    equatorObject(obj),
    freezeEpoch(freeze),
    isFrozen(true)
{
}

Eigen::Quaterniond
BodyMeanEquatorFrame::getOrientation(double tjd) const
{
    double t = isFrozen ? freezeEpoch : tjd;

    switch (equatorObject.getType())
    {
    case SelectionType::Body:
        return equatorObject.body()->getEclipticToEquatorial(t);
    case SelectionType::Star:
        return equatorObject.star()->getRotationModel()->equatorOrientationAtTime(t);
    default:
        return Eigen::Quaterniond::Identity();
    }
}

Eigen::Vector3d
BodyMeanEquatorFrame::getAngularVelocity(double tjd) const
{
    if (isFrozen)
        return Eigen::Vector3d::Zero();

    if (equatorObject.body() != nullptr)
        return equatorObject.body()->getBodyFrame(tjd)->getAngularVelocity(tjd);

    return Eigen::Vector3d::Zero();
}

bool
BodyMeanEquatorFrame::isInertial() const
{
    // Although the mean equator of an object may vary slightly due to precession,
    // treat it as an inertial frame as long as the body frame of the object is
    // also inertial.
    return isFrozen ||
           equatorObject.body() == nullptr ||
           equatorObject.body()->getBodyFrame(0.0)->isInertial();
}

unsigned int
BodyMeanEquatorFrame::nestingDepth(unsigned int depth,
                                   unsigned int maxDepth) const
{
    // Test origin and equator object (typically the same) frames
    unsigned int n =  getFrameDepth(getCenter(), depth, maxDepth, FrameType::PositionFrame);
    if (n > maxDepth)
        return n;

    unsigned int m = getFrameDepth(equatorObject, depth, maxDepth, FrameType::OrientationFrame);
    return std::max(m, n);
}

/*** CachingFrame ***/

CachingFrame::CachingFrame(Selection _center) : ReferenceFrame(_center)
{
}

Eigen::Quaterniond
CachingFrame::getOrientation(double tjd) const
{
    if (tjd != lastTime)
    {
        lastTime = tjd;
        lastOrientation = computeOrientation(tjd);
        orientationCacheValid = true;
        angularVelocityCacheValid = false;
    }
    else if (!orientationCacheValid)
    {
        lastOrientation = computeOrientation(tjd);
        orientationCacheValid = true;
    }

    return lastOrientation;
}

Eigen::Vector3d
CachingFrame::getAngularVelocity(double tjd) const
{
    if (tjd != lastTime)
    {
        lastTime = tjd;
        lastAngularVelocity = computeAngularVelocity(tjd);
        orientationCacheValid = false;
        angularVelocityCacheValid = true;
    }
    else if (!angularVelocityCacheValid)
    {
        lastAngularVelocity = computeAngularVelocity(tjd);
        angularVelocityCacheValid = true;
    }

    return lastAngularVelocity;
}

/*! Calculate the angular velocity at the specified time (units are
 *  radians / Julian day.) The default implementation just
 *  differentiates the orientation.
 */
Eigen::Vector3d
CachingFrame::computeAngularVelocity(double tjd) const
{
    Eigen::Quaterniond q0 = getOrientation(tjd);

    // Call computeOrientation() instead of getOrientation() so that we
    // don't affect the cached value.
    // TODO: check the valid ranges of the frame to make sure that
    // jd+dt is still in range.
    Eigen::Quaterniond q1 = computeOrientation(tjd + ANGULAR_VELOCITY_DIFF_DELTA);

    Eigen::Quaterniond dq = q0.conjugate() * q1;

    if (std::abs(dq.w()) > 0.99999999)
        return Eigen::Vector3d::Zero();

    return dq.vec().normalized() * (2.0 * std::acos(dq.w()) / ANGULAR_VELOCITY_DIFF_DELTA);
}

/*** TwoVectorFrame ***/

TwoVectorFrame::TwoVectorFrame(Selection center,
                               const FrameVector& prim,
                               int primAxis,
                               const FrameVector& sec,
                               int secAxis) :
    CachingFrame(center),
    primaryVector(prim),
    primaryAxis(primAxis),
    secondaryVector(sec),
    secondaryAxis(secAxis)
{
    // Verify that primary and secondary axes are valid
    assert(primaryAxis != 0 && secondaryAxis != 0);
    assert(std::abs(primaryAxis) <= 3 && std::abs(secondaryAxis) <= 3);
    // Verify that the primary and secondary axes aren't collinear
    assert(std::abs(primaryAxis) != std::abs(secondaryAxis));

    if (std::abs(primaryAxis) != 1 && std::abs(secondaryAxis) != 1)
        tertiaryAxis = 1;
    else if (std::abs(primaryAxis) != 2 && std::abs(secondaryAxis) != 2)
        tertiaryAxis = 2;
    else
        tertiaryAxis = 3;
}

Eigen::Quaterniond
TwoVectorFrame::computeOrientation(double tjd) const
{
    Eigen::Vector3d v0 = primaryVector.direction(tjd);
    Eigen::Vector3d v1 = secondaryVector.direction(tjd);

    // TODO: verify that v0 and v1 aren't zero length
    v0.normalize();
    v1.normalize();

    if (primaryAxis < 0)
        v0 = -v0;
    if (secondaryAxis < 0)
        v1 = -v1;

    Eigen::Vector3d v2 = v0.cross(v1);

    // Check for degenerate case when the primary and secondary vectors
    // are collinear. A well-chosen two vector frame should never have this
    // problem.
    double length = v2.norm();
    if (length < Tolerance)
    {
        // Just return identity . . .
        return Eigen::Quaterniond::Identity();
    }

    v2 = v2 / length;

    // Determine whether the primary and secondary axes are in
    // right hand order.
    int rhAxis = std::abs(primaryAxis) + 1;
    if (rhAxis > 3)
        rhAxis = 1;
    bool rhOrder = rhAxis == std::abs(secondaryAxis);

    // Set the rotation matrix axes
    Eigen::Matrix3d m;
    m.row(std::abs(primaryAxis) - 1) = v0;

    // Reverse the cross products if the axes are not in right
    // hand order.
    if (rhOrder)
    {
        m.row(std::abs(secondaryAxis) - 1) = v2.cross(v0);
        m.row(std::abs(tertiaryAxis) - 1)  = v2;
    }
    else
    {
        m.row(std::abs(secondaryAxis) - 1) = v0.cross(-v2);
        m.row(std::abs(tertiaryAxis) - 1)  = -v2;
    }

    // The axes are the rows of a rotation matrix. The getOrientation
    // method must return the quaternion representation of the
    // orientation, so convert the rotation matrix to a quaternion now.
    Eigen::Quaterniond q(m);

    // A rotation matrix will have a determinant of 1; if the matrix also
    // includes a reflection, the determinant will be -1, indicating that
    // there's a bug and there's a reversed cross-product or sign error
    // somewhere.
    // assert(Mat3d(v[0], v[1], v[2]).determinant() > 0);

    return q;
}

bool
TwoVectorFrame::isInertial() const
{
    // Although it's possible to specify an inertial two-vector frame, we won't
    // bother trying to distinguish these cases: all two-vector frames will be
    // treated as non-inertial.
    return true;
}

unsigned int
TwoVectorFrame::nestingDepth(unsigned int depth,
                             unsigned int maxDepth) const
{
    // Check nesting of the origin object as well as frames references by
    // the primary and secondary axes.
    unsigned int n = getFrameDepth(getCenter(), depth, maxDepth, FrameType::PositionFrame);
    if (n > maxDepth)
        return n;

    unsigned int m = primaryVector.nestingDepth(depth, maxDepth);
    n = std::max(m, n);
    if (n > maxDepth)
        return n;

    m = secondaryVector.nestingDepth(depth, maxDepth);
    return std::max(m, n);
}

FrameVector::FrameVector(const RelativePosition& pos) : m_data(pos)
{
}

FrameVector::FrameVector(const RelativeVelocity& vel) : m_data(vel)
{
}

FrameVector::FrameVector(ConstVector&& vec) : m_data(std::move(vec))
{
}

FrameVector
FrameVector::createRelativePositionVector(const Selection& _observer,
                                          const Selection& _target)
{
    return FrameVector(RelativePosition { _observer, _target });
}

FrameVector
FrameVector::createRelativeVelocityVector(const Selection& _observer,
                                          const Selection& _target)
{
    return FrameVector(RelativeVelocity { _observer, _target });
}

FrameVector
FrameVector::createConstantVector(const Eigen::Vector3d& _vec,
                                  const ReferenceFrame::SharedConstPtr& _frame)
{
    return FrameVector(ConstVector { _vec, _frame });
}

Eigen::Vector3d
FrameVector::direction(double tjd) const
{
    class DirectionVisitor
    {
    public:
        explicit DirectionVisitor(double t) : _tjd(t) {}

        Eigen::Vector3d operator()(const RelativePosition& pos) const
        {
            return pos.target.getPosition(_tjd).offsetFromKm(pos.observer.getPosition(_tjd));
        }

        Eigen::Vector3d operator()(const RelativeVelocity& vel) const
        {
            return vel.target.getVelocity(_tjd) - vel.observer.getVelocity(_tjd);
        }

        Eigen::Vector3d operator()(const ConstVector& vec) const
        {
            return vec.frame != nullptr
                ? vec.frame->getOrientation(_tjd).conjugate() * vec.vec
                : vec.vec;
        }

    private:
        double _tjd;
    };

    return std::visit(DirectionVisitor{ tjd }, m_data);
}

unsigned int
FrameVector::nestingDepth(unsigned int depth,
                          unsigned int maxDepth) const
{
    class NestingVisitor
    {
    public:
        NestingVisitor(unsigned int d, unsigned int md)
            : _depth(d), _maxDepth(md)
        {
        }

        unsigned int operator()(const RelativePosition& pos) const
        {
            return relativeDepth(pos.observer, pos.target);
        }

        unsigned int operator()(const RelativeVelocity& vel) const
        {
            return relativeDepth(vel.observer, vel.target);
        }

        unsigned int operator()(const ConstVector& vec) const
        {
            return _depth > _maxDepth
                ? _depth
                : vec.frame->nestingDepth(_depth + 1, _maxDepth);
        }

    private:
        unsigned int relativeDepth(const Selection& _observer, const Selection& _target) const
        {
            unsigned int n = ReferenceFrame::getFrameDepth(_observer,
                                                           _depth,
                                                           _maxDepth,
                                                           ReferenceFrame::FrameType::PositionFrame);
            if (n > _maxDepth)
                return n;

            unsigned int m = ReferenceFrame::getFrameDepth(_target,
                                                           _depth,
                                                           _maxDepth,
                                                           ReferenceFrame::FrameType::PositionFrame);
            return std::max(m, n);
        }

        unsigned int _depth;
        unsigned int _maxDepth;
    };

    return std::visit(NestingVisitor{ depth, maxDepth }, m_data);
}
