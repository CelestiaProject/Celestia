// orbit.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <functional>
#include <cmath>
#include <celmath/mathlib.h>
#include <celmath/solve.h>
#include "orbit.h"

using namespace std;


EllipticalOrbit::EllipticalOrbit(double _pericenterDistance,
                                 double _eccentricity,
                                 double _inclination,
                                 double _ascendingNode,
                                 double _argOfPeriapsis,
                                 double _meanAnomalyAtEpoch,
                                 double _period,
                                 double _epoch) :
    pericenterDistance(_pericenterDistance),
    eccentricity(_eccentricity),
    inclination(_inclination),
    ascendingNode(_ascendingNode),
    argOfPeriapsis(_argOfPeriapsis),
    meanAnomalyAtEpoch(_meanAnomalyAtEpoch),
    period(_period),
    epoch(_epoch)
{
}


// Standard iteration for solving Kepler's Equation
struct SolveKeplerFunc1 : public unary_function<double, double>
{
    double ecc;
    double M;

    SolveKeplerFunc1(double _ecc, double _M) : ecc(_ecc), M(_M) {};

    double operator()(double x) const
    {
        return M + ecc * sin(x);
    }
};


// Faster converging iteration for Kepler's Equation; more efficient
// than above for orbits with eccentricities greater than 0.3.  This
// is from Jean Meeus's _Astronomical Algorithms_ (2nd ed), p. 199
struct SolveKeplerFunc2 : public unary_function<double, double>
{
    double ecc;
    double M;

    SolveKeplerFunc2(double _ecc, double _M) : ecc(_ecc), M(_M) {};

    double operator()(double x) const
    {
        return x + (M + ecc * sin(x) - x) / (1 - ecc * cos(x));
    }
};


struct SolveKeplerLaguerreConway : public unary_function<double, double>
{
    double ecc;
    double M;

    SolveKeplerLaguerreConway(double _ecc, double _M) : ecc(_ecc), M(_M) {};

    double operator()(double x) const
    {
        double s = ecc * sin(x);
        double c = ecc * cos(x);
        double f = x - s - M;
        double f1 = 1 - c;
        double f2 = s;
        x += -5 * f / (f1 + sign(f1) * sqrt(abs(16 * f1 * f1 - 20 * f * f2)));

        return x;
    }
};

struct SolveKeplerLaguerreConwayHyp : public unary_function<double, double>
{
    double ecc;
    double M;

    SolveKeplerLaguerreConwayHyp(double _ecc, double _M) : ecc(_ecc), M(_M) {};

    double operator()(double x) const
    {
        double s = ecc * sinh(x);
        double c = ecc * cosh(x);
        double f = s - x - M;
        double f1 = c - 1;
        double f2 = s;
        x += -5 * f / (f1 + sign(f1) * sqrt(abs(16 * f1 * f1 - 20 * f * f2)));

        return x;
    }
};

typedef pair<double, double> Solution;


double EllipticalOrbit::eccentricAnomaly(double M) const
{
    if (eccentricity == 0.0)
    {
        // Circular orbit
        return M;
    }
    else if (eccentricity < 0.2)
    {
        // Low eccentricity, so use the standard iteration technique
        Solution sol = solve_iteration_fixed(SolveKeplerFunc1(eccentricity, M), M, 5);
        return sol.first;
    }
    else if (eccentricity < 0.9)
    {
        // Higher eccentricity elliptical orbit; use a more complex but
        // much faster converging iteration.
        Solution sol = solve_iteration_fixed(SolveKeplerFunc2(eccentricity, M), M, 6);
        // Debugging
        // printf("ecc: %f, error: %f mas\n",
        //        eccentricity, radToDeg(sol.second) * 3600000);
        return sol.first;
    }
    else if (eccentricity < 1.0)
    {
        // Extremely stable Laguerre-Conway method for solving Kepler's
        // equation.  Only use this for high-eccentricity orbits, as it
        // requires more calcuation.
        double E = M + 0.85 * eccentricity * sign(sin(M));
        Solution sol = solve_iteration_fixed(SolveKeplerLaguerreConway(eccentricity, M), E, 8);
        return sol.first;
    }
    else if (eccentricity == 1.0)
    {
        // Nearly parabolic orbit; very common for comets
        // TODO: handle this
        return M;
    }
    else
    {
        // Laguerre-Conway method for hyperbolic (ecc > 1) orbits.
        double E = log(2 * M / eccentricity + 1.85);
        Solution sol = solve_iteration_fixed(SolveKeplerLaguerreConwayHyp(eccentricity, M), E, 30);
        return sol.first;
    }
}


Point3d EllipticalOrbit::positionAtE(double E) const
{
    double x, z;

    if (eccentricity < 1.0)
    {
        double a = pericenterDistance / (1.0 - eccentricity);
        x = a * (cos(E) - eccentricity);
        z = a * sqrt(1 - square(eccentricity)) * -sin(E);
    }
    else if (eccentricity > 1.0)
    {
        double a = pericenterDistance / (1.0 - eccentricity);
        x = -a * (eccentricity - cosh(E));
        z = -a * sqrt(square(eccentricity) - 1) * -sinh(E);
    }
    else
    {
        // TODO: Handle parabolic orbits
        x = 0.0;
        z = 0.0;
    }

    Mat3d R = (Mat3d::yrotation(ascendingNode) *
               Mat3d::xrotation(inclination) *
               Mat3d::yrotation(argOfPeriapsis));

    return R * Point3d(x, 0, z);
}


// Return the offset from the center
Point3d EllipticalOrbit::positionAtTime(double t) const
{
    t = t - epoch;
    double meanMotion = 2.0 * PI / period;
    double meanAnomaly = meanAnomalyAtEpoch + t * meanMotion;
    double E = eccentricAnomaly(meanAnomaly);
    
    return positionAtE(E);
#if 0
    double x, z;

    // TODO: It's probably not a good idea to use this calculation for orbits
    // with eccentricities greater than 0.98.
    if (eccentricity < 1.0)
    {
        double eccAnomaly;

        if (eccentricity == 0.0)
        {
            // Circular orbit
            eccAnomaly = meanAnomaly;
        }
        else if (eccentricity < 0.2)
        {
            // Low eccentricity, so use the standard iteration technique
            Solution sol = solve_iteration_fixed(SolveKeplerFunc1(eccentricity,
                                                                  meanAnomaly),
                                                 meanAnomaly, 5);
            eccAnomaly = sol.first;
        }
        else if (eccentricity < 0.9)
        {
            // Higher eccentricity elliptical orbit; use a more complex but
            // much faster converging iteration.
            Solution sol = solve_iteration_fixed(SolveKeplerFunc2(eccentricity,
                                                                  meanAnomaly),
                                                 meanAnomaly, 6);
            eccAnomaly = sol.first;
            // Debugging
            // printf("ecc: %f, error: %f mas\n",
            //        eccentricity, radToDeg(sol.second) * 3600000);
        }
        else
        {
            // Extremely stable Laguerre-Conway method for solving Kepler's
            // equation.  Only use this for high-eccentricity orbits, as it
            // requires more calcuation.
            double E = meanAnomaly +
                0.85 * eccentricity * sign(sin(meanAnomaly));
            Solution sol = solve_iteration_fixed(SolveKeplerLaguerreConway(eccentricity, meanAnomaly), E, 8);
            eccAnomaly = sol.first;
        }

        double semiMajorAxis = pericenterDistance / (1.0 - eccentricity);
        x = semiMajorAxis * (cos(eccAnomaly) - eccentricity);
        z = semiMajorAxis * sqrt(1 - square(eccentricity)) * -sin(eccAnomaly);
    }
    else if (eccentricity == 1.0)
    {
        // Nearly parabolic orbit; very common for comets
        // double b = sqrt(1 + a * a); 
        x = 0.0;
        z = 0.0;
    }
    else
    {
        // Laguerre-Conway method for hyperbolic (ecc > 1) orbits.
        double E = log(2 * meanAnomaly / eccentricity + 1.85);
        Solution sol = solve_iteration_fixed(SolveKeplerLaguerreConwayHyp(eccentricity, meanAnomaly), E, 30);
        double eccAnomaly = sol.first;

        double a = pericenterDistance / (1.0 - eccentricity);
        x = -a * (eccentricity - cosh(eccAnomaly));
        z = -a * sqrt(square(eccentricity) - 1) * -sinh(eccAnomaly);
    }

    Mat3d R = (Mat3d::yrotation(ascendingNode) *
               Mat3d::xrotation(inclination) *
               Mat3d::yrotation(argOfPeriapsis));

    return R * Point3d(x, 0, z);
#endif
}


double EllipticalOrbit::getPeriod() const
{
    return period;
}


double EllipticalOrbit::getBoundingRadius() const
{
    // TODO: watch out for unbounded parabolic and hyperbolic orbits
    return pericenterDistance * ((1.0 + eccentricity) / (1.0 - eccentricity));
}


void EllipticalOrbit::sample(double start, double t, int nSamples,
                             OrbitSampleProc& proc) const
{
    double dE = 2 * PI / (double) nSamples;
    for (int i = 0; i < nSamples; i++)
        proc.sample(positionAtE(dE * i));
}



Point3d CachingOrbit::positionAtTime(double jd) const
{
    if (jd != lastTime)
    {
        lastTime = jd;
        lastPosition = computePosition(jd);
    }
    return lastPosition;
}


void CachingOrbit::sample(double start, double t, int nSamples,
                          OrbitSampleProc& proc) const
{
    double dt = t / (double) nSamples;
    for (int i = 0; i < nSamples; i++)
        proc.sample(positionAtTime(start + dt * i));
}
