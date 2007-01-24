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

