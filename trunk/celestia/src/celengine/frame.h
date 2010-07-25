// frame.h
// 
// Copyright (C) 2003-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_FRAME_H_
#define _CELENGINE_FRAME_H_

#include <celengine/astro.h>
#include <celengine/selection.h>
#include <Eigen/Core>
#include <Eigen/Geometry>


/*! A ReferenceFrame object has a center and set of orthogonal axes.
 *
 * Subclasses of ReferenceFrame must override the getOrientation method
 * (which specifies the coordinate axes at a given time) and the
 * nestingDepth() method (which is used to check for recursive frames.)
 */
class ReferenceFrame
{
 public:
    ReferenceFrame(Selection center);
    virtual ~ReferenceFrame() {};

    int addRef() const;
    int release() const;
    
    UniversalCoord convertFromUniversal(const UniversalCoord& uc, double tjd) const;
    UniversalCoord convertToUniversal(const UniversalCoord& uc, double tjd) const;
    Eigen::Quaterniond convertFromUniversal(const Eigen::Quaterniond& q, double tjd) const;
    Eigen::Quaterniond convertToUniversal(const Eigen::Quaterniond& q, double tjd) const;

    Eigen::Vector3d convertFromAstrocentric(const Eigen::Vector3d& p, double tjd) const;
    Eigen::Vector3d convertToAstrocentric(const Eigen::Vector3d& p, double tjd) const;
    
    Selection getCenter() const;

    virtual Eigen::Quaterniond getOrientation(double tjd) const = 0;
    virtual Eigen::Vector3d getAngularVelocity(double tdb) const;

	virtual bool isInertial() const = 0;

    enum FrameType
    {
        PositionFrame = 1,
        OrientationFrame = 2,
    };

    unsigned int nestingDepth(unsigned int maxDepth, FrameType) const;

    virtual unsigned int nestingDepth(unsigned int depth,
                                      unsigned int maxDepth,
                                      FrameType frameType) const = 0;

 private:
    Selection centerObject;
    mutable int refCount;
};


/*! Base class for complex frames where there may be some benefit
 *  to caching the last calculated orientation.
 */
class CachingFrame : public ReferenceFrame
{
 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    CachingFrame(Selection _center);
    virtual ~CachingFrame() {};

    Eigen::Quaterniond getOrientation(double tjd) const;
    Eigen::Vector3d getAngularVelocity(double tjd) const;
    virtual Eigen::Quaterniond computeOrientation(double tjd) const = 0;
    virtual Eigen::Vector3d computeAngularVelocity(double tjd) const;

 private:
    mutable double lastTime;
    mutable Eigen::Quaterniond lastOrientation;
    mutable Eigen::Vector3d lastAngularVelocity;
	mutable bool orientationCacheValid;
	mutable bool angularVelocityCacheValid;
};


//! J2000.0 Earth ecliptic frame
class J2000EclipticFrame : public ReferenceFrame
{
 public:
    J2000EclipticFrame(Selection center);
    virtual ~J2000EclipticFrame() {};

    Eigen::Quaterniond getOrientation(double /* tjd */) const
    {
        return Eigen::Quaterniond::Identity();
    }

	virtual bool isInertial() const;

    virtual unsigned int nestingDepth(unsigned int depth,
                                      unsigned int maxDepth,
                                      FrameType frameType) const;
};


//! J2000.0 Earth Mean Equator
class J2000EquatorFrame : public ReferenceFrame
{
 public:
    J2000EquatorFrame(Selection center);
    virtual ~J2000EquatorFrame() {};
    Eigen::Quaterniond getOrientation(double tjd) const;
	virtual bool isInertial() const;
    virtual unsigned int nestingDepth(unsigned int depth,
                                      unsigned int maxDepth,
                                      FrameType frameType) const;
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
    BodyFixedFrame(Selection center, Selection obj);
    virtual ~BodyFixedFrame() {};
    Eigen::Quaterniond getOrientation(double tjd) const;
    virtual Eigen::Vector3d getAngularVelocity(double tjd) const;
	virtual bool isInertial() const;
    virtual unsigned int nestingDepth(unsigned int depth,
                                      unsigned int maxDepth,
                                      FrameType frameType) const;

 private:
    Selection fixObject;
};


class BodyMeanEquatorFrame : public ReferenceFrame
{
 public:
    BodyMeanEquatorFrame(Selection center, Selection obj, double freeze);
    BodyMeanEquatorFrame(Selection center, Selection obj);
    virtual ~BodyMeanEquatorFrame() {};
    Eigen::Quaterniond getOrientation(double tjd) const;
    virtual Eigen::Vector3d getAngularVelocity(double tjd) const;
	virtual bool isInertial() const;
    virtual unsigned int nestingDepth(unsigned int depth,
                                      unsigned int maxDepth,
                                      FrameType frameType) const;

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
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    FrameVector(const FrameVector& fv);
    ~FrameVector();
    FrameVector& operator=(const FrameVector&);

    Eigen::Vector3d direction(double tjd) const;

    /*! Frames can be defined in reference to other frames; this method
     *  counts the depth of such nesting, up to some specified maximum
     *  level. This method is used to test for circular references in
     *  frames.
     */
    unsigned int nestingDepth(unsigned int depth, unsigned int maxDepth) const;

    enum FrameVectorType
    {
        RelativePosition,
        RelativeVelocity,
        ConstantVector,
    };
    
    static FrameVector createRelativePositionVector(const Selection& _observer,
                                                    const Selection& _target);
    static FrameVector createRelativeVelocityVector(const Selection& _observer,
                                                    const Selection& _target);
    static FrameVector createConstantVector(const Eigen::Vector3d& _vec,
                                            const ReferenceFrame* _frame);

 private:
    /*! Type-only constructor is private. Code outside the class should
     *  use create*Vector methods to create new FrameVectors.
     */
    FrameVector(FrameVectorType t);

    FrameVectorType vecType;
    Selection observer;
    Selection target;
    Eigen::Vector3d vec;                   // constant vector
    const ReferenceFrame* frame; // frame for constant vector
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
    virtual ~TwoVectorFrame() {};

    Eigen::Quaterniond computeOrientation(double tjd) const;
	virtual bool isInertial() const;
    virtual unsigned int nestingDepth(unsigned int depth,
                                      unsigned int maxDepth,
                                      FrameType frameType) const;

    //! The sine of minimum angle between the primary and secondary vectors
    static const double Tolerance;

 private:
    FrameVector primaryVector;
    int primaryAxis;
    FrameVector secondaryVector;
    int secondaryAxis;
    int tertiaryAxis;
};

#endif // _CELENGINE_FRAME_H_
