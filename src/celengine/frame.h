// frame.h
//
// Copyright (C) 2003-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <celutil/classops.h>
#include "selection.h"
#include "shared.h"

class ReferenceFrame;

class FrameVisitor
{
public:
    virtual void visitPosition(const Selection& sel) = 0;
    virtual void visitBodyFrame(const Body* body, std::optional<double> epoch = std::nullopt) = 0;
    virtual void visitReferenceFrame(const ReferenceFrame*) = 0;

protected:
    FrameVisitor() = default;
    ~FrameVisitor() = default;
};

/*! A ReferenceFrame object has a center and set of orthogonal axes.
 *
 * Subclasses of ReferenceFrame must override the getOrientation method
 * (which specifies the coordinate axes at a given time) and the
 * nestingDepth() method (which is used to check for recursive frames.)
 */
class ReferenceFrame
{
public:
    SHARED_TYPES(ReferenceFrame)

    virtual ~ReferenceFrame() = default;

    virtual Eigen::Quaterniond getOrientation(double tjd) const = 0;
    virtual Eigen::Vector3d getAngularVelocity(double tdb) const;

    virtual bool isInertial() const = 0;

    virtual void visitChildren(FrameVisitor&) const = 0;

protected:
    explicit ReferenceFrame() = default;
};

/*! Base class for complex frames where there may be some benefit
 *  to caching the last calculated orientation.
 */
class CachingFrame : public ReferenceFrame
{
public:
    Eigen::Quaterniond getOrientation(double tjd) const final;
    Eigen::Vector3d getAngularVelocity(double tjd) const final;

protected:
    CachingFrame() = default;

    virtual Eigen::Quaterniond computeOrientation(double tjd) const = 0;
    virtual Eigen::Vector3d computeAngularVelocity(double tjd) const;

private:
    mutable double lastTime{ 1.0e-50 };
    mutable Eigen::Quaterniond lastOrientation{ Eigen::Quaterniond::Identity() };
    mutable Eigen::Vector3d lastAngularVelocity{ Eigen::Vector3d::Zero() };
    mutable bool orientationCacheValid{ false };
    mutable bool angularVelocityCacheValid{ false };
};

//! J2000.0 Earth ecliptic frame
class J2000EclipticFrame final : public ReferenceFrame
{
private:
    struct CreateToken { explicit CreateToken() = default; };

public:
    explicit J2000EclipticFrame(CreateToken) {}

    static SharedConstPtr getInstance();

    Eigen::Quaterniond getOrientation(double /* tjd */) const override
    {
        return Eigen::Quaterniond::Identity();
    }

    bool isInertial() const override { return true; }
    void visitChildren(FrameVisitor&) const override { /* no-op */ }
};

//! J2000.0 Earth Mean Equator
class J2000EquatorFrame final : public ReferenceFrame
{
private:
    struct CreateToken { explicit CreateToken() = default; };

public:
    explicit J2000EquatorFrame(CreateToken) {}

    static SharedConstPtr getInstance();

    Eigen::Quaterniond getOrientation(double tjd) const override;
    bool isInertial() const override { return true; }
    void visitChildren(FrameVisitor&) const override { /* no-op*/ }
};

/*! A BodyFixed frame is a coordinate system with the x-axis pointing
 *  from the body center through the intersection of the prime meridian
 *  and the equator, and the z-axis aligned with the north pole. The
 *  y-axis is the cross product of x and z, and points toward the 90
 *  meridian.
 */
class BodyFixedFrame final : public ReferenceFrame
{
public:
    explicit BodyFixedFrame(Selection obj);

    Eigen::Quaterniond getOrientation(double tjd) const override;
    Eigen::Vector3d getAngularVelocity(double tjd) const override;
    bool isInertial() const override { return false; }
    void visitChildren(FrameVisitor&) const override;

private:
    Selection m_fixObject;
};

class BodyMeanEquatorFrame final : public ReferenceFrame
{
public:
    explicit BodyMeanEquatorFrame(Selection obj, std::optional<double> freeze = std::nullopt);

    Eigen::Quaterniond getOrientation(double tjd) const override;
    Eigen::Vector3d getAngularVelocity(double tjd) const override;
    bool isInertial() const override;
    void visitChildren(FrameVisitor&) const override;

private:
    Selection m_equatorObject;
    std::optional<double> m_freezeEpoch;
};

/*! FrameVectors are used to define the axes for TwoVector frames
 */
class FrameVector
{
public:
    struct RelativePosition
    {
        RelativePosition(const Selection& obs, const Selection& tgt) :
            observer(obs), target(tgt)
        {}

        Selection observer;
        Selection target;
    };

    struct RelativeVelocity
    {
        RelativeVelocity(const Selection& obs, const Selection& tgt) :
            observer(obs), target(tgt)
        {}

        Selection observer;
        Selection target;
    };

    struct ConstVector
    {
        ConstVector(const Eigen::Vector3d& v, ReferenceFrame::SharedConstPtr f) :
            vec(v), frame(f)
        {}

        Eigen::Vector3d vec;
        ReferenceFrame::SharedConstPtr frame;
    };

    explicit FrameVector(const RelativePosition&);
    explicit FrameVector(const RelativeVelocity&);
    explicit FrameVector(ConstVector&&);

    Eigen::Vector3d direction(double tjd) const;
    void visitChildren(FrameVisitor&) const;

private:
    std::variant<RelativePosition, RelativeVelocity, ConstVector> m_data;
};

/*! A two vector frame is a coordinate system defined by a primary and
 *  secondary vector. The primary axis points in the direction of the
 *  primary vector. The secondary axis points in the direction of the
 *  component of the secondary vector that is orthogonal to the primary
 *  vector. The third axis is the cross product of the primary and
 *  secondary axis.
 */
class TwoVectorFrame final : public CachingFrame
{
public:
    /*! primAxis and secAxis are the labels of the axes defined by
     *  the primary and secondary vectors:
     *  1 = x, 2 = y, 3 = z, -1 = -x, -2 = -y, -3 = -z
     */
    TwoVectorFrame(const FrameVector& prim,
                   int primAxis,
                   const FrameVector& sec,
                   int secAxis);

    bool isInertial() const override;
    void visitChildren(FrameVisitor&) const override;

protected:
    Eigen::Quaterniond computeOrientation(double tjd) const override;

private:
    FrameVector m_primaryVector;
    int m_primaryAxis;
    FrameVector m_secondaryVector;
    int m_secondaryAxis;
    int m_tertiaryAxis;
};

enum class FrameId : std::size_t
{
};

enum class FrameVectorId : std::size_t
{
};

enum class SimpleFrameKey
{
    J2000Ecliptic,
    J2000Equator,
};

struct BodyFixedFrameKey
{
    explicit BodyFixedFrameKey(const Selection& tgt) : target(tgt) {}

    Selection target;
};

inline bool operator==(const BodyFixedFrameKey& lhs, const BodyFixedFrameKey& rhs)
{
    return lhs.target == rhs.target;
}

inline bool operator!=(const BodyFixedFrameKey& lhs, const BodyFixedFrameKey& rhs)
{
    return !(lhs == rhs);
}

template<>
struct std::hash<BodyFixedFrameKey>
{
    std::size_t operator()(const BodyFixedFrameKey& obj) const { return hash<Selection>{}(obj.target); }
};

struct BodyMeanEquatorFrameKey
{
    explicit BodyMeanEquatorFrameKey(Selection tgt, std::optional<double> freeze = std::nullopt) :
        target(tgt), freezeEpoch(freeze)
    {}

    Selection target;
    std::optional<double> freezeEpoch;
};

inline bool operator==(const BodyMeanEquatorFrameKey& lhs, const BodyMeanEquatorFrameKey& rhs)
{
    return lhs.target == rhs.target && lhs.freezeEpoch == rhs.freezeEpoch;
}

inline bool operator!=(const BodyMeanEquatorFrameKey& lhs, const BodyMeanEquatorFrameKey& rhs)
{
    return !(lhs == rhs);
}

template<>
struct std::hash<BodyMeanEquatorFrameKey>
{
    std::size_t operator()(const BodyMeanEquatorFrameKey&) const;
};

struct TwoVectorFrameKey
{
    TwoVectorFrameKey(FrameVectorId f1, int a1, FrameVectorId f2, int a2) :
        frameVectorId1(f1), axis1(a1), frameVectorId2(f2), axis2(a2)
    {}

    FrameVectorId frameVectorId1;
    int axis1;
    FrameVectorId frameVectorId2;
    int axis2;
};

inline bool operator==(const TwoVectorFrameKey& lhs, const TwoVectorFrameKey& rhs)
{
    return lhs.frameVectorId1 == rhs.frameVectorId1 &&
           lhs.axis1 == rhs.axis1 &&
           lhs.frameVectorId2 == rhs.frameVectorId2 &&
           lhs.axis2 == rhs.axis2;
}

inline bool operator!=(const TwoVectorFrameKey& lhs, const TwoVectorFrameKey& rhs)
{
    return !(lhs == rhs);
}

template<>
struct std::hash<TwoVectorFrameKey>
{
    std::size_t operator()(const TwoVectorFrameKey&) const;
};

using FrameKey = std::variant<SimpleFrameKey, BodyFixedFrameKey, BodyMeanEquatorFrameKey, TwoVectorFrameKey>;

struct RelativePositionKey
{
    RelativePositionKey(const Selection& obs, const Selection& tgt) :
        observer(obs), target(tgt)
    {}

    Selection observer;
    Selection target;
};

inline bool operator==(const RelativePositionKey& lhs, const RelativePositionKey& rhs)
{
    return lhs.observer == rhs.observer && lhs.target == rhs.target;
}

inline bool operator!=(const RelativePositionKey& lhs, const RelativePositionKey& rhs)
{
    return !(lhs == rhs);
}

template<>
struct std::hash<RelativePositionKey>
{
    std::size_t operator()(const RelativePositionKey&) const;
};

struct RelativeVelocityKey
{
    RelativeVelocityKey(const Selection& obs, const Selection& tgt) :
        observer(obs), target(tgt)
    {}

    Selection observer;
    Selection target;
};

inline bool operator==(const RelativeVelocityKey& lhs, const RelativeVelocityKey& rhs)
{
    return lhs.observer == rhs.observer && lhs.target == rhs.target;
}

inline bool operator!=(const RelativeVelocityKey& lhs, const RelativeVelocityKey& rhs)
{
    return !(lhs == rhs);
}

template<>
struct std::hash<RelativeVelocityKey>
{
    std::size_t operator()(const RelativeVelocityKey&) const;
};

struct ConstVectorKey
{
    ConstVectorKey(const Eigen::Vector3d& v, FrameId fid) :
        vec(v), frameId(fid)
    {}

    Eigen::Vector3d vec;
    FrameId frameId;
};

inline bool operator==(const ConstVectorKey& lhs, const ConstVectorKey& rhs)
{
    return lhs.vec == rhs.vec && lhs.frameId == rhs.frameId;
}

inline bool operator!=(const ConstVectorKey& lhs, const ConstVectorKey& rhs)
{
    return !(lhs == rhs);
}

template<>
struct std::hash<ConstVectorKey>
{
    std::size_t operator()(const ConstVectorKey&) const;
};

using FrameVectorKey = std::variant<RelativePositionKey, RelativeVelocityKey, ConstVectorKey>;

class FrameCache : private celestia::util::NoCopy
{
public:
    ReferenceFrame::SharedConstPtr getFrame(FrameId) const;

    FrameId getFrameId(const FrameKey&);
    FrameVectorId getFrameVectorId(const FrameVectorKey&);

    void commit();
    void rollback();

private:
    void createFrame(const FrameKey&);

    std::vector<ReferenceFrame::SharedConstPtr> m_frames;
    std::vector<FrameVector> m_frameVectors;
    std::unordered_map<FrameKey, FrameId> m_frameMap;
    std::unordered_map<FrameVectorKey, FrameVectorId> m_frameVectorMap;

    std::size_t m_uncommittedFrameIndex{ 0 };
    std::size_t m_uncommittedVectorIndex{ 0 };
    std::vector<FrameKey> m_uncommittedFrames;
    std::vector<FrameVectorKey> m_uncommittedVectors;
};
