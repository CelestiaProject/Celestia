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

#endif // _ORBIT_H_
