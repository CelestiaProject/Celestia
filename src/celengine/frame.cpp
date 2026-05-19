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

#include <boost/container_hash/hash.hpp>

#include <celastro/date.h>
#include <celephem/rotation.h>
#include <celmath/geomutil.h>
#include "body.h"
#include "location.h"
#include "selection.h"
#include "star.h"
#include "timeline.h"
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

} // end unnamed namespace

/*** ReferenceFrame ***/

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

/*** J2000EclipticFrame ***/

J2000EclipticFrame::SharedConstPtr
J2000EclipticFrame::getInstance()
{
    static std::weak_ptr<const J2000EclipticFrame> instance;
    auto ptr = instance.lock();
    if (ptr)
        return ptr;

    ptr = std::make_shared<J2000EclipticFrame>(CreateToken{});
    instance = ptr;
    return ptr;
}

/*** J2000EquatorFrame ***/

J2000EquatorFrame::SharedConstPtr
J2000EquatorFrame::getInstance()
{
    static std::weak_ptr<const J2000EquatorFrame> instance;
    auto ptr = instance.lock();
    if (ptr)
        return ptr;

    ptr = std::make_shared<J2000EquatorFrame>(CreateToken{});
    instance = ptr;
    return ptr;
}

Eigen::Quaterniond
J2000EquatorFrame::getOrientation(double /* tjd */) const
{
    return J2000Orientation;
}

/*** BodyFixedFrame ***/

BodyFixedFrame::BodyFixedFrame(Selection obj) :
    m_fixObject(obj)
{
}

Eigen::Quaterniond
BodyFixedFrame::getOrientation(double tjd) const
{
    switch (m_fixObject.getType())
    {
    case SelectionType::Body:
        return math::YRot180<double> * m_fixObject.body()->getEclipticToBodyFixed(tjd);
    case SelectionType::Star:
        return math::YRot180<double> * m_fixObject.star()->getRotationModel()->orientationAtTime(tjd);
    case SelectionType::Location:
        if (m_fixObject.location()->getParentBody())
            return math::YRot180<double> * m_fixObject.location()->getParentBody()->getEclipticToBodyFixed(tjd);
        else
            return math::YRot180<double>;
    default:
        return math::YRot180<double>;
    }
}

Eigen::Vector3d
BodyFixedFrame::getAngularVelocity(double tjd) const
{
    switch (m_fixObject.getType())
    {
    case SelectionType::Body:
        return m_fixObject.body()->getAngularVelocity(tjd);
    case SelectionType::Star:
        return m_fixObject.star()->getRotationModel()->angularVelocityAtTime(tjd);
    case SelectionType::Location:
        if (const Body* parentBody = m_fixObject.location()->getParentBody(); parentBody)
            return parentBody->getAngularVelocity(tjd);
        else
            return Eigen::Vector3d::Zero();
    default:
        return Eigen::Vector3d::Zero();
    }
}

void
BodyFixedFrame::visitChildren(FrameVisitor& visitor) const
{
    switch (m_fixObject.getType())
    {
    case SelectionType::Body:
        visitor.visitBodyFrame(m_fixObject.body());
        break;

    case SelectionType::Location:
        if (const Body* parentBody = m_fixObject.location()->getParentBody(); parentBody)
            visitor.visitBodyFrame(parentBody);
        break;

    default:
        break;
    }
}

/*** BodyMeanEquatorFrame ***/

BodyMeanEquatorFrame::BodyMeanEquatorFrame(Selection obj,
                                           std::optional<double> freeze) :
    m_equatorObject(obj),
    m_freezeEpoch(freeze)
{
}

Eigen::Quaterniond
BodyMeanEquatorFrame::getOrientation(double tjd) const
{
    double t = m_freezeEpoch.value_or(tjd);
    switch (m_equatorObject.getType())
    {
    case SelectionType::Body:
        return m_equatorObject.body()->getEclipticToEquatorial(t);
    case SelectionType::Star:
        return m_equatorObject.star()->getRotationModel()->equatorOrientationAtTime(t);
    default:
        return Eigen::Quaterniond::Identity();
    }
}

Eigen::Vector3d
BodyMeanEquatorFrame::getAngularVelocity(double tjd) const
{
    if (m_freezeEpoch.has_value())
        return Eigen::Vector3d::Zero();

    if (const Body* body = m_equatorObject.body(); body)
        return body->getBodyFrame(tjd)->getAngularVelocity(tjd);

    return Eigen::Vector3d::Zero();
}

bool
BodyMeanEquatorFrame::isInertial() const
{
    // Although the mean equator of an object may vary slightly due to precession,
    // treat it as an inertial frame as long as the body frame of the object is
    // also inertial.
    if (m_freezeEpoch.has_value())
        return true;

    if (const Body* body = m_equatorObject.body(); body)
    {
        return body->getTimeline()->phaseCount() == 1
            && body->getTimeline()->findPhase(0).bodyFrame()->isInertial();
    }

    return true;
}

void
BodyMeanEquatorFrame::visitChildren(FrameVisitor& visitor) const
{
    if (const Body* body = m_equatorObject.body(); body)
        visitor.visitBodyFrame(body, m_freezeEpoch);
}

/*** CachingFrame ***/

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

TwoVectorFrame::TwoVectorFrame(const FrameVector& prim,
                               int primAxis,
                               const FrameVector& sec,
                               int secAxis) :
    m_primaryVector(prim),
    m_primaryAxis(primAxis),
    m_secondaryVector(sec),
    m_secondaryAxis(secAxis)
{
    // Verify that primary and secondary axes are valid
    assert(m_primaryAxis != 0 && m_secondaryAxis != 0);
    assert(std::abs(m_primaryAxis) <= 3 && std::abs(m_secondaryAxis) <= 3);
    // Verify that the primary and secondary axes aren't collinear
    assert(std::abs(m_primaryAxis) != std::abs(m_secondaryAxis));

    if (std::abs(m_primaryAxis) != 1 && std::abs(m_secondaryAxis) != 1)
        m_tertiaryAxis = 1;
    else if (std::abs(m_primaryAxis) != 2 && std::abs(m_secondaryAxis) != 2)
        m_tertiaryAxis = 2;
    else
        m_tertiaryAxis = 3;
}

Eigen::Quaterniond
TwoVectorFrame::computeOrientation(double tjd) const
{
    Eigen::Vector3d v0 = m_primaryVector.direction(tjd);
    Eigen::Vector3d v1 = m_secondaryVector.direction(tjd);

    // TODO: verify that v0 and v1 aren't zero length
    v0.normalize();
    v1.normalize();

    if (m_primaryAxis < 0)
        v0 = -v0;
    if (m_secondaryAxis < 0)
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
    int rhAxis = std::abs(m_primaryAxis) + 1;
    if (rhAxis > 3)
        rhAxis = 1;
    bool rhOrder = rhAxis == std::abs(m_secondaryAxis);

    // Set the rotation matrix axes
    Eigen::Matrix3d m;
    m.row(std::abs(m_primaryAxis) - 1) = v0;

    // Reverse the cross products if the axes are not in right
    // hand order.
    if (rhOrder)
    {
        m.row(std::abs(m_secondaryAxis) - 1) = v2.cross(v0);
        m.row(std::abs(m_tertiaryAxis) - 1)  = v2;
    }
    else
    {
        m.row(std::abs(m_secondaryAxis) - 1) = v0.cross(-v2);
        m.row(std::abs(m_tertiaryAxis) - 1)  = -v2;
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
    return false;
}

void
TwoVectorFrame::visitChildren(FrameVisitor& visitor) const
{
    m_primaryVector.visitChildren(visitor);
    m_secondaryVector.visitChildren(visitor);
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

void
FrameVector::visitChildren(FrameVisitor& visitor) const
{
    class ReferenceVisitor
    {
    public:
        explicit ReferenceVisitor(FrameVisitor& v) : m_visitor(v) {}

        void operator()(const RelativePosition& pos) const
        {
            m_visitor.visitPosition(pos.observer);
            m_visitor.visitPosition(pos.target);
        }

        void operator()(const RelativeVelocity& vel) const
        {
            m_visitor.visitPosition(vel.observer);
            m_visitor.visitPosition(vel.target);
        }

        void operator()(const ConstVector& vec) const
        {
            m_visitor.visitReferenceFrame(vec.frame.get());
        }

    private:
        FrameVisitor& m_visitor;
    };

    std::visit(ReferenceVisitor(visitor), m_data);
}

std::size_t
std::hash<BodyMeanEquatorFrameKey>::operator()(const BodyMeanEquatorFrameKey& obj) const
{
    std::size_t seed = 0;
    boost::hash_combine(seed, obj.target);
    boost::hash_combine(seed, obj.freezeEpoch);
    return seed;
}

std::size_t
std::hash<TwoVectorFrameKey>::operator()(const TwoVectorFrameKey& obj) const
{
    std::size_t seed = 0;
    boost::hash_combine(seed, obj.frameVectorId1);
    boost::hash_combine(seed, obj.axis1);
    boost::hash_combine(seed, obj.frameVectorId2);
    boost::hash_combine(seed, obj.axis2);
    return seed;
}

std::size_t
std::hash<RelativePositionKey>::operator()(const RelativePositionKey& obj) const
{
    std::size_t seed = 0;
    boost::hash_combine(seed, obj.observer);
    boost::hash_combine(seed, obj.target);
    return seed;
}

std::size_t
std::hash<RelativeVelocityKey>::operator()(const RelativeVelocityKey& obj) const
{
    std::size_t seed = 0;
    boost::hash_combine(seed, obj.observer);
    boost::hash_combine(seed, obj.target);
    return seed;
}

std::size_t
std::hash<ConstVectorKey>::operator()(const ConstVectorKey& obj) const
{
    std::size_t seed = 0;
    boost::hash_combine(seed, obj.vec.x());
    boost::hash_combine(seed, obj.vec.y());
    boost::hash_combine(seed, obj.vec.z());
    boost::hash_combine(seed, obj.frameId);
    return seed;
}

ReferenceFrame::SharedConstPtr
FrameCache::getFrame(FrameId frameId) const
{
    assert(static_cast<std::size_t>(frameId) < m_frames.size());
    return m_frames[static_cast<std::size_t>(frameId)];
}

FrameId
FrameCache::getFrameId(const FrameKey& key)
{
    auto [it, inserted] = m_frameMap.try_emplace(key, static_cast<FrameId>(m_frames.size()));
    if (inserted)
        createFrame(key);

    return it->second;
}

void
FrameCache::createFrame(const FrameKey& key)
{
    class FrameKeyVisitor
    {
    public:
        explicit FrameKeyVisitor(const FrameCache& cache) : m_cache(cache) {}

        ReferenceFrame::SharedConstPtr operator()(SimpleFrameKey k) const
        {
            switch (k)
            {
            case SimpleFrameKey::J2000Ecliptic:
                return J2000EclipticFrame::getInstance();

            case SimpleFrameKey::J2000Equator:
                return J2000EquatorFrame::getInstance();

            default:
                assert(0);
                return nullptr;
            }
        }

        ReferenceFrame::SharedConstPtr operator()(const BodyFixedFrameKey& k) const
        {
            return std::make_shared<BodyFixedFrame>(k.target);
        }

        ReferenceFrame::SharedConstPtr operator()(const BodyMeanEquatorFrameKey& k) const
        {
            return std::make_shared<BodyMeanEquatorFrame>(k.target, k.freezeEpoch);
        }

        ReferenceFrame::SharedConstPtr operator()(const TwoVectorFrameKey& k) const
        {
            assert(static_cast<std::size_t>(k.frameVectorId1) < m_cache.m_frameVectors.size());
            assert(static_cast<std::size_t>(k.frameVectorId2) < m_cache.m_frameVectors.size());
            const FrameVector& vec1 = m_cache.m_frameVectors[static_cast<std::size_t>(k.frameVectorId1)];
            const FrameVector& vec2 = m_cache.m_frameVectors[static_cast<std::size_t>(k.frameVectorId2)];
            return std::make_shared<TwoVectorFrame>(vec1, k.axis1, vec2, k.axis2);
        }

    private:
        const FrameCache& m_cache;
    };

    m_frames.emplace_back(std::visit(FrameKeyVisitor(*this), key));
    m_uncommittedFrames.emplace_back(key);
}

FrameVectorId
FrameCache::getFrameVectorId(const FrameVectorKey& key)
{
    auto [it, inserted] = m_frameVectorMap.try_emplace(key, static_cast<FrameVectorId>(m_frameVectorMap.size()));
    if (!inserted)
        return it->second;

    class FrameVectorVisitor
    {
    public:
        explicit FrameVectorVisitor(const FrameCache& cache) : m_cache(cache) {}

        FrameVector operator()(const RelativePositionKey& k) const
        {
            return FrameVector(FrameVector::RelativePosition(k.observer, k.target));
        }

        FrameVector operator()(const RelativeVelocityKey& k) const
        {
            return FrameVector(FrameVector::RelativeVelocity(k.observer, k.target));
        }

        FrameVector operator()(const ConstVectorKey& k) const
        {
            assert(static_cast<std::size_t>(k.frameId) < m_cache.m_frames.size());
            const ReferenceFrame::SharedConstPtr& frame = m_cache.m_frames[static_cast<std::size_t>(k.frameId)];
            return FrameVector(FrameVector::ConstVector(k.vec, frame));
        }

    private:
        const FrameCache& m_cache;
    };

    m_frameVectors.emplace_back(std::visit(FrameVectorVisitor(*this), key));
    m_uncommittedVectors.emplace_back(key);
    return it->second;
}

// Marks all generated frames and frame vectors up to this point as committed.
void
FrameCache::commit()
{
    m_uncommittedFrameIndex = m_frames.size();
    m_uncommittedVectorIndex = m_frameVectors.size();
    m_uncommittedFrames.clear();
    m_uncommittedVectors.clear();
}

// Removes all generated frames and frame vectors created since the last commit
// This allows us to remove any dangling pointers if the validation fails and
// the Body object is deleted.
void
FrameCache::rollback()
{
    m_frames.erase(m_frames.begin() + m_uncommittedFrameIndex, m_frames.end());
    m_frameVectors.erase(m_frameVectors.begin() + m_uncommittedVectorIndex, m_frameVectors.end());

    for (const FrameKey& key : m_uncommittedFrames)
        m_frameMap.erase(key);

    for (const FrameVectorKey& key : m_uncommittedVectors)
        m_frameVectorMap.erase(key);

    m_uncommittedFrames.clear();
    m_uncommittedVectors.clear();
}
