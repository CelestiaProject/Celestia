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

#include <celmath/quaternion.h>
#include <celengine/astro.h>


class RotationElements
{
 public:
    inline RotationElements();

    inline Quatd eclipticalToEquatorial(double t) const;
    inline Quatd eclipticalToPlanetographic(double t) const;
    inline Quatd equatorialToPlanetographic(double t) const;

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


Quatd RotationElements::eclipticalToEquatorial(double t) const
{
    double Omega = (double) ascendingNode + precessionRate * (t - astro::J2000);

    return (Quatd::xrotation(-obliquity) * Quatd::yrotation(-Omega));
}


Quatd RotationElements::eclipticalToPlanetographic(double t) const
{
    return equatorialToPlanetographic(t) * eclipticalToEquatorial(t);
}


Quatd RotationElements::equatorialToPlanetographic(double t) const
{
    double rotations = (t - epoch) / (double) period;
    double wholeRotations = floor(rotations);
    double remainder = rotations - wholeRotations;

    // Add an extra half rotation because of the convention in all
    // planet texture maps where zero deg long. is in the middle of
    // the texture.
    remainder += 0.5;
    
    Quatd q(1);
    q.yrotate(-remainder * 2 * PI - offset);

    return q;
}

#endif // _CELENGINE_ROTATION_H_
