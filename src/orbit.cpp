// orbit.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cmath>
#include "mathlib.h"
#include "orbit.h"


EllipticalOrbit::EllipticalOrbit(double _semiMajorAxis,
                                 double _eccentricity,
                                 double _inclination,
                                 double _ascendingNode,
                                 double _argOfPeriapsis,
                                 double _meanAnomalyAtEpoch,
                                 double _period,
                                 double _epoch) :
    semiMajorAxis(_semiMajorAxis),
    eccentricity(_eccentricity),
    inclination(_inclination),
    ascendingNode(_ascendingNode),
    argOfPeriapsis(_argOfPeriapsis),
    meanAnomalyAtEpoch(_meanAnomalyAtEpoch),
    period(_period),
    epoch(_epoch)
{
}


// First four terms of a series solution to Kepler's equation
// for orbital motion.  This only works for small eccentricities,
// and in fact the series diverges for e > 0.6627434
static double solveKeplerSeries(double M, double e)
{
#if 0
    return M + e * (sin(M) +
                    e * ((0.5 * sin(2 * M)) +
                         e * ((0.325 * sin(3 * M) - 0.125 * sin(M)) +
                              e * ((1.0 / 3.0 * sin(4 * M) - 1.0 / 6.0 * sin(2 * M))))));
#endif    
    return (M +
            e * sin(M) +
            e * e * 0.5 * sin(2 * M) +
            e * e * e * (0.325 * sin(3 * M) - 0.125 * sin(M)) +
            e * e * e * e * (1.0 / 3.0 * sin(4 * M) - 1.0 / 6.0 * sin(2 * M)));

}


// Return the offset from the barycenter
Point3d EllipticalOrbit::positionAtTime(double t) const
{
    t = t - epoch;
    double meanMotion = 2.0 * PI / period;
    double meanAnomaly = meanAnomalyAtEpoch + t * meanMotion;

    // TODO: calculate the *real* eccentric anomaly.  This is a reasonable
    // approximation only for nearly circular orbits
    double eccAnomaly = meanAnomaly;
    // eccAnomaly = solveKeplerSeries(meanAnomaly, eccentricity);

    double x = semiMajorAxis * (-cos(eccAnomaly) - eccentricity);
    double z = semiMajorAxis * sqrt(1 - square(eccentricity)) * -sin(eccAnomaly);

    Mat3d R = (Mat3d::yrotation(ascendingNode) *
               Mat3d::xrotation(inclination) *
               Mat3d::yrotation(argOfPeriapsis - ascendingNode));

    return Point3d(x, 0, z) * R;
}


double EllipticalOrbit::getPeriod() const
{
    return period;
}
