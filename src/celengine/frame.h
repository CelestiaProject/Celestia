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

#include <optional>
#include <variant>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "selection.h"
#include "shared.h"

class FrameVector;

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

protected:
    explicit ReferenceFrame() = default;

    friend class FrameVector;
};

/*! Base class for complex frames where there may be some benefit
 *  to caching the last calculated orientation.
 */
class CachingFrame : public ReferenceFrame
{
public:
    SHARED_TYPES(CachingFrame)

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
public:
    SHARED_TYPES(J2000EclipticFrame)

    J2000EclipticFrame() = default;

    Eigen::Quaterniond getOrientation(double /* tjd */) const override
    {
        return Eigen::Quaterniond::Identity();
    }

    bool isInertial() const override { return true; }
};

//! J2000.0 Earth Mean Equator
class J2000EquatorFrame final : public ReferenceFrame
{
public:
    SHARED_TYPES(J2000EquatorFrame)

    J2000EquatorFrame() = default;

    Eigen::Quaterniond getOrientation(double tjd) const override;
    bool isInertial() const override { return true; }
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
    SHARED_TYPES(BodyFixedFrame)

    explicit BodyFixedFrame(Selection obj);
    ~BodyFixedFrame() override = default;
    Eigen::Quaterniond getOrientation(double tjd) const override;
    Eigen::Vector3d getAngularVelocity(double tjd) const override;
    bool isInertial() const override { return false; }

private:
    Selection m_fixObject;
};

class BodyMeanEquatorFrame final : public ReferenceFrame
{
public:
    SHARED_TYPES(BodyMeanEquatorFrame)

    BodyMeanEquatorFrame(Selection obj, std::optional<double> freeze = std::nullopt);
    ~BodyMeanEquatorFrame() override = default;
    Eigen::Quaterniond getOrientation(double tjd) const override;
    Eigen::Vector3d getAngularVelocity(double tjd) const override;
    bool isInertial() const override;

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
        Selection observer;
        Selection target;
    };

    struct RelativeVelocity
    {
        Selection observer;
        Selection target;
    };

    struct ConstVector
    {
        Eigen::Vector3d vec;
        ReferenceFrame::SharedConstPtr frame;
    };

    explicit FrameVector(const RelativePosition&);
    explicit FrameVector(const RelativeVelocity&);
    explicit FrameVector(ConstVector&&);

    Eigen::Vector3d direction(double tjd) const;

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
    ~TwoVectorFrame() override = default;

    bool isInertial() const override;

protected:
    Eigen::Quaterniond computeOrientation(double tjd) const override;

private:
    FrameVector m_primaryVector;
    int m_primaryAxis;
    FrameVector m_secondaryVector;
    int m_secondaryAxis;
    int m_tertiaryAxis;
};
