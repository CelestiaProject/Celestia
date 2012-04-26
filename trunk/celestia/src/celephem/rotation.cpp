// rotation.cpp
//
// Implementation of basic RotationModel class hierarchy for describing
// the orientation of objects over time.
//
// Copyright (C) 2006-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "rotation.h"
#include <celmath/geomutil.h>
#include <celmath/mathlib.h>
#include <cmath>

using namespace Eigen;
using namespace std;


static const double ANGULAR_VELOCITY_DIFF_DELTA = 1.0 / 1440.0;

// Choose a time interval for numerically differentiating orientation
// to get the angular velocity for a rotation model.
static double chooseDiffTimeDelta(const RotationModel& rm)
{
    if (rm.isPeriodic())
        return rm.getPeriod() / 10000.0;
    else
        return ANGULAR_VELOCITY_DIFF_DELTA;
}

/***** RotationModel *****/

/*! Return the angular velocity at the specified time (TDB). The default
 *  implementation computes the angular velocity via differentiation.
 */
Vector3d
RotationModel::angularVelocityAtTime(double tdb) const
{
    double dt = chooseDiffTimeDelta(*this);
    Quaterniond q0 = orientationAtTime(tdb);
    Quaterniond q1 = orientationAtTime(tdb + dt);
    Quaterniond dq = q1.conjugate() * q0;

    if (std::abs(dq.w()) > 0.99999999)
        return Vector3d::Zero();

    return dq.vec().normalized() * (2.0 * acos(dq.w()) / dt);
#ifdef CELVEC
    Vector3d v(dq.x, dq.y, dq.z);
	v.normalize();
	return v * (2.0 * acos(dq.w) / dt);
#endif
}


/***** CachingRotationModel *****/

CachingRotationModel::CachingRotationModel() :
    lastTime(365.0),
    spinCacheValid(false),
    equatorCacheValid(false),
    angularVelocityCacheValid(false)
{
}


CachingRotationModel::~CachingRotationModel()
{
}


Quaterniond
CachingRotationModel::spin(double tjd) const
{
    if (tjd != lastTime)
    {
        lastTime = tjd;
        lastSpin = computeSpin(tjd);
        spinCacheValid = true;
        equatorCacheValid = false;
        angularVelocityCacheValid = false;
    }
    else if (!spinCacheValid)
    {
        lastSpin = computeSpin(tjd);
        spinCacheValid = true;
    }
    
    return lastSpin;
}


Quaterniond
CachingRotationModel::equatorOrientationAtTime(double tjd) const
{
    if (tjd != lastTime)
    {
        lastTime = tjd;
        lastEquator = computeEquatorOrientation(tjd);
        spinCacheValid = false;
        equatorCacheValid = true;
        angularVelocityCacheValid = false;
    }
    else if (!equatorCacheValid)
    {
        lastEquator = computeEquatorOrientation(tjd);
        equatorCacheValid = true;
    }
    
    return lastEquator;    
}


Vector3d
CachingRotationModel::angularVelocityAtTime(double tjd) const
{
    if (tjd != lastTime)
    {
        lastAngularVelocity = computeAngularVelocity(tjd);
        lastTime = tjd;
        spinCacheValid = false;
        equatorCacheValid = false;
        angularVelocityCacheValid = true;
    }
    else if (!angularVelocityCacheValid)
    {
        lastAngularVelocity = computeAngularVelocity(tjd);
        angularVelocityCacheValid = true;
    }
    
    return lastAngularVelocity;    
}


Vector3d
CachingRotationModel::computeAngularVelocity(double tjd) const
{
    double dt = chooseDiffTimeDelta(*this);
    Quaterniond q0 = orientationAtTime(tjd);
    
    // Call computeSpin/computeEquatorOrientation instead of orientationAtTime
    // in order to avoid affecting the cache.
    Quaterniond spin = computeSpin(tjd + dt);
    Quaterniond equator = computeEquatorOrientation(tjd + dt);
    Quaterniond q1 = spin * equator;
    Quaterniond dq = q1.conjugate() * q0;
    
    if (std::abs(dq.w()) > 0.99999999)
        return Vector3d::Zero();
    
    return dq.vec().normalized() * (2.0 * acos(dq.w()) / dt);
#ifdef CELVEC
	Vec3d v(dq.x, dq.y, dq.z);
	v.normalize();
    return v * (2.0 * acos(dq.w) / dt);
#endif
}


/***** ConstantOrientation implementation *****/

ConstantOrientation::ConstantOrientation(const Quaterniond& q) :
    orientation(q)
{
}


ConstantOrientation::~ConstantOrientation()
{
}


Quaterniond
ConstantOrientation::spin(double) const
{
    return orientation;
}


Vector3d
ConstantOrientation::angularVelocityAtTime(double /* tdb */) const
{
    return Vector3d::Zero();
}


/***** UniformRotationModel implementation *****/

UniformRotationModel::UniformRotationModel(double _period,
                                           float _offset,
                                           double _epoch,
                                           float _inclination,
                                           float _ascendingNode) :
    period(_period),
    offset(_offset),
    epoch(_epoch),
    inclination(_inclination),
    ascendingNode(_ascendingNode)
{
}


UniformRotationModel::~UniformRotationModel()
{
}


bool
UniformRotationModel::isPeriodic() const
{
    return true;
}


double
UniformRotationModel::getPeriod() const
{
    return period;
}


Quaterniond
UniformRotationModel::spin(double tjd) const
{
    double rotations = (tjd - epoch) / period;
    double wholeRotations = floor(rotations);
    double remainder = rotations - wholeRotations;

    // TODO: This is the wrong place for this offset
    // Add an extra half rotation because of the convention in all
    // planet texture maps where zero deg long. is in the middle of
    // the texture.
    remainder += 0.5;
    
    return YRotation(-remainder * 2 * PI - offset);
}


Quaterniond
UniformRotationModel::equatorOrientationAtTime(double) const
{
    return XRotation((double) -inclination) * YRotation((double) -ascendingNode);
}


Vector3d
UniformRotationModel::angularVelocityAtTime(double tdb) const
{
    Vector3d v = equatorOrientationAtTime(tdb).conjugate() * Vector3d::UnitY();;
	return v * (2.0 * PI / period);
}


/***** PrecessingRotationModel implementation *****/

PrecessingRotationModel::PrecessingRotationModel(double _period,
                                                 float _offset,
                                                 double _epoch,
                                                 float _inclination,
                                                 float _ascendingNode,
                                                 double _precPeriod) :
    period(_period),
    offset(_offset),
    epoch(_epoch),
    inclination(_inclination),
    ascendingNode(_ascendingNode),
    precessionPeriod(_precPeriod)
{
}


PrecessingRotationModel::~PrecessingRotationModel()
{
}


bool
PrecessingRotationModel::isPeriodic() const
{
    return true;
}


double
PrecessingRotationModel::getPeriod() const
{
    return period;
}


Quaterniond
PrecessingRotationModel::spin(double tjd) const
{
    double rotations = (tjd - epoch) / period;
    double wholeRotations = floor(rotations);
    double remainder = rotations - wholeRotations;

    // TODO: This is the wrong place for this offset
    // Add an extra half rotation because of the convention in all
    // planet texture maps where zero deg long. is in the middle of
    // the texture.
    remainder += 0.5;
    
    return YRotation(-remainder * 2 * PI - offset);
}


Quaterniond
PrecessingRotationModel::equatorOrientationAtTime(double tjd) const
{
    double nodeOfDate;

    // A precession rate of zero indicates no precession
    if (precessionPeriod == 0.0)
    {
        nodeOfDate = ascendingNode;
    }
    else
    {
        nodeOfDate = (double) ascendingNode -
            (2.0 * PI / precessionPeriod) * (tjd - epoch);
    }

    return XRotation((double) -inclination) * YRotation(-nodeOfDate);
}

