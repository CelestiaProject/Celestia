// rotation.h
//
// Copyright (C) 2004 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_ROTATION_H_
#define _CELENGINE_ROTATION_H_

#include <celengine/astro.h>


class RotationElements
{
 public:
    inline RotationElements();

    float period;        // sidereal rotation period
    float offset;        // rotation at epoch
    double epoch;
    float obliquity;     // tilt of rotation axis w.r.t. ecliptic
    float ascendingNode; // long. of ascending node of equator on the ecliptic
    float precessionRate; // rate of precession of rotation axis in rads/day
};


RotationElements::RotationElements() :
    period(1.0f),
    offset(0.0f),
    epoch(astro::J2000),
    obliquity(0.0f),
    ascendingNode(0.0f),
    precessionRate(0.0f)
{
}


inline bool operator==(const RotationElements& re0, const RotationElements& re1)
{
    return (re0.period         == re1.period        &&
            re0.offset         == re1.offset        &&
            re0.epoch          == re1.epoch         &&
            re0.obliquity      == re1.obliquity     &&
            re0.ascendingNode  == re1.ascendingNode &&
            re0.precessionRate == re1.precessionRate);
}

#endif // _CELENGINE_ROTATION_H_
