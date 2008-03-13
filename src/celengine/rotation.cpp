// rotation.cpp
//
// Implementation of basic RotationModel class hierarchy for describing
// the orientation of objects over time.
//
// Copyright (C) 2006, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "rotation.h"

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
Vec3d
RotationModel::angularVelocityAtTime(double tdb) const
{
    double dt = chooseDiffTimeDelta(*this);
	Quatd q0 = orientationAtTime(tdb);
	Quatd q1 = orientationAtTime(tdb + dt);
	Quatd dq = ~q1 * q0;

	if (fabs(dq.w) > 0.99999999)
		return Vec3d(0.0, 0.0, 0.0);

	Vec3d v(dq.x, dq.y, dq.z);
	v.normalize();
	return v * (2.0 * acos(dq.w) / dt);
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


Quatd
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


Quatd
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


Vec3d
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


Vec3d
CachingRotationModel::computeAngularVelocity(double tjd) const
{
    double dt = chooseDiffTimeDelta(*this);
    Quatd q0 = orientationAtTime(tjd);
    
    // Call computeSpin/computeEquatorOrientation instead of orientationAtTime
    // in order to avoid affecting the cache.
    Quatd spin = computeSpin(tjd + dt);
    Quatd equator = computeEquatorOrientation(tjd + dt);
	Quatd q1 = spin * equator;
    Quatd dq = ~q1 * q0;
    
	if (fabs(dq.w) > 0.99999999)
		return Vec3d(0.0, 0.0, 0.0);
    
	Vec3d v(dq.x, dq.y, dq.z);
	v.normalize();
	return v * (2.0 * acos(dq.w) / dt);    
}


/***** ConstantOrientation implementation *****/

ConstantOrientation::ConstantOrientation(const Quatd& q) :
    orientation(q)
{
}


ConstantOrientation::~ConstantOrientation()
{
}


Quatd
ConstantOrientation::spin(double) const
{
    return orientation;
}


Vec3d
ConstantOrientation::angularVelocityAtTime(double /* tdb */) const
{
	return Vec3d(0.0, 0.0, 0.0);
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


Quatd
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
    
    return Quatd::yrotation(-remainder * 2 * PI - offset);
}


Quatd
UniformRotationModel::equatorOrientationAtTime(double) const
{
    return Quatd::xrotation(-inclination) * Quatd::yrotation(-ascendingNode);
}


Vec3d
UniformRotationModel::angularVelocityAtTime(double tdb) const
{
	Vec3d v(0.0, 1.0, 0.0);
	v = v * equatorOrientationAtTime(tdb).toMatrix3();
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


Quatd
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
    
    return Quatd::yrotation(-remainder * 2 * PI - offset);
}


Quatd
PrecessingRotationModel::equatorOrientationAtTime(double tjd) const
{
    double nodeOfDate;

    // A precession rate of zero indicates no precession
    if (precessionPeriod == 0.0)
        nodeOfDate = ascendingNode;
    else
        nodeOfDate = (double) ascendingNode -
            (2.0 * PI / precessionPeriod) * (tjd - epoch);

    return Quatd::xrotation(-inclination) * Quatd::yrotation(-nodeOfDate);
}

