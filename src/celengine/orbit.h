// orbit.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _ORBIT_H_
#define _ORBIT_H_

#include <celmath/vecmath.h>


class Orbit
{
public:
    virtual Point3d positionAtTime(double) const = 0;
    virtual double getPeriod() const = 0;
    virtual double getBoundingRadius() const = 0;
};


class EllipticalOrbit : public Orbit
{
public:
    EllipticalOrbit(double, double, double, double, double, double, double,
                    double _epoch = 2451545.0);

    // Compute the orbit for a specified Julian date
    Point3d positionAtTime(double) const;
    double getPeriod() const;
    double getBoundingRadius() const;

private:
    double pericenterDistance;
    double eccentricity;
    double inclination;
    double ascendingNode;
    double argOfPeriapsis;
    double meanAnomalyAtEpoch;
    double period;
    double epoch;
};



// Custom orbit classes should be derived from CachingOrbit.  The custom
// orbits can be expensive to compute, with more than 50 periodic terms.
// Celestia may need require position of a planet more than once per frame; in
// order to avoid redundant calculation, the CachingOrbit class saves the
// result of the last calculation and uses it if the time matches the cached
// time.
class CachingOrbit : public Orbit
{
public:
    CachingOrbit() : lastTime(1.0e-30) {};

    virtual Point3d computePosition(double jd) const = 0;
    virtual double getPeriod() const = 0;
    virtual double getBoundingRadius() const = 0;

    Point3d positionAtTime(double jd) const
    {
        if (jd != lastTime)
        {
            lastTime = jd;
            lastPosition = computePosition(jd);
        }
        return lastPosition;
    };

private:
    mutable Point3d lastPosition;
    mutable double lastTime;
};


#endif // _ORBIT_H_
