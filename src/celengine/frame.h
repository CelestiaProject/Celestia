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

    explicit ReferenceFrame(Selection center);
    virtual ~ReferenceFrame() = default;

    UniversalCoord convertFromUniversal(const UniversalCoord& uc, double tjd) const;
    UniversalCoord convertToUniversal(const UniversalCoord& uc, double tjd) const;
    Eigen::Quaterniond convertFromUniversal(const Eigen::Quaterniond& q, double tjd) const;
    Eigen::Quaterniond convertToUniversal(const Eigen::Quaterniond& q, double tjd) const;

    Eigen::Vector3d convertToAstrocentric(const Eigen::Vector3d& p, double tjd) const;

    Selection getCenter() const;

    virtual Eigen::Quaterniond getOrientation(double tjd) const = 0;
    virtual Eigen::Vector3d getAngularVelocity(double tdb) const;

    virtual bool isInertial() const = 0;

    unsigned int nestingDepth(unsigned int maxDepth) const;

protected:
    enum class FrameType
    {
        PositionFrame,
        OrientationFrame,
    };

    static unsigned int getFrameDepth(const Selection& sel,
                                      unsigned int depth,
                                      unsigned int maxDepth,
                                      FrameType frameType);

    virtual unsigned int nestingDepth(unsigned int depth,
                                      unsigned int maxDepth) const = 0;

private:
    Selection centerObject;

    friend class FrameVector;
};

/*! Base class for complex frames where there may be some benefit
 *  to caching the last calculated orientation.
 */
class CachingFrame : public ReferenceFrame
{
public:
    SHARED_TYPES(CachingFrame)

    explicit CachingFrame(Selection _center);
    ~CachingFrame() override = default;

    Eigen::Quaterniond getOrientation(double tjd) const final;
    Eigen::Vector3d getAngularVelocity(double tjd) const final;

protected:
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
class J2000EclipticFrame : public ReferenceFrame
{
public:
    SHARED_TYPES(J2000EclipticFrame)

    explicit J2000EclipticFrame(Selection center);
    ~J2000EclipticFrame() override = default;

    Eigen::Quaterniond getOrientation(double /* tjd */) const override
    {
        return Eigen::Quaterniond::Identity();
    }

    bool isInertial() const override;

protected:
    unsigned int nestingDepth(unsigned int depth,
                              unsigned int maxDepth) const override;
};

//! J2000.0 Earth Mean Equator
class J2000EquatorFrame : public ReferenceFrame
{
public:
    SHARED_TYPES(J2000EquatorFrame)

    explicit J2000EquatorFrame(Selection center);
    ~J2000EquatorFrame() override = default;
    Eigen::Quaterniond getOrientation(double tjd) const override;
    bool isInertial() const override;

protected:
    unsigned int nestingDepth(unsigned int depth,
                              unsigned int maxDepth) const override;
};

/*! A BodyFixed frame is a coordinate system with the x-axis pointing
 *  from the body center through the intersection of the prime meridian
 *  and the equator, and the z-axis aligned with the north pole. The
 *  y-axis is the cross product of x and z, and points toward the 90
 *  meridian.
 */
class BodyFixedFrame : public ReferenceFrame
{
public:
    SHARED_TYPES(BodyFixedFrame)

    BodyFixedFrame(Selection center, Selection obj);
    ~BodyFixedFrame() override = default;
    Eigen::Quaterniond getOrientation(double tjd) const override;
    Eigen::Vector3d getAngularVelocity(double tjd) const override;
    bool isInertial() const override;

protected:
    unsigned int nestingDepth(unsigned int depth,
                              unsigned int maxDepth) const override;

private:
    Selection fixObject;
};

class BodyMeanEquatorFrame : public ReferenceFrame
{
public:
    SHARED_TYPES(BodyMeanEquatorFrame)

    BodyMeanEquatorFrame(Selection center, Selection obj, double freeze);
    BodyMeanEquatorFrame(Selection center, Selection obj);
    ~BodyMeanEquatorFrame() override = default;
    Eigen::Quaterniond getOrientation(double tjd) const override;
    Eigen::Vector3d getAngularVelocity(double tjd) const override;
    bool isInertial() const override;

protected:
    unsigned int nestingDepth(unsigned int depth,
                              unsigned int maxDepth) const override;

private:
    Selection equatorObject;
    double freezeEpoch;
    bool isFrozen;
};

/*! FrameVectors are used to define the axes for TwoVector frames
 */
class FrameVector
{
public:
    Eigen::Vector3d direction(double tjd) const;

    /*! Frames can be defined in reference to other frames; this method
     *  counts the depth of such nesting, up to some specified maximum
     *  level. This method is used to test for circular references in
     *  frames.
     */
    unsigned int nestingDepth(unsigned int depth, unsigned int maxDepth) const;

    static FrameVector createRelativePositionVector(const Selection& _observer,
                                                    const Selection& _target);
    static FrameVector createRelativeVelocityVector(const Selection& _observer,
                                                    const Selection& _target);
    static FrameVector createConstantVector(const Eigen::Vector3d& _vec,
                                            const ReferenceFrame::SharedConstPtr& _frame);

private:
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

    std::variant<RelativePosition, RelativeVelocity, ConstVector> m_data;
};

/*! A two vector frame is a coordinate system defined by a primary and
 *  secondary vector. The primary axis points in the direction of the
 *  primary vector. The secondary axis points in the direction of the
 *  component of the secondary vector that is orthogonal to the primary
 *  vector. The third axis is the cross product of the primary and
 *  secondary axis.
 */
class TwoVectorFrame : public CachingFrame
{
public:
    /*! primAxis and secAxis are the labels of the axes defined by
     *  the primary and secondary vectors:
     *  1 = x, 2 = y, 3 = z, -1 = -x, -2 = -y, -3 = -z
     */
    TwoVectorFrame(Selection center,
                   const FrameVector& prim,
                   int primAxis,
                   const FrameVector& sec,
                   int secAxis);
    ~TwoVectorFrame() override = default;

    bool isInertial() const override;

protected:
    Eigen::Quaterniond computeOrientation(double tjd) const override;
    unsigned int nestingDepth(unsigned int depth,
                              unsigned int maxDepth) const override;

private:
    FrameVector primaryVector;
    int primaryAxis;
    FrameVector secondaryVector;
    int secondaryAxis;
    int tertiaryAxis;
};
