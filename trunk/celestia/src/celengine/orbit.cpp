// orbit.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <functional>
#include <algorithm>
#include <cmath>
#include <cassert>
#include <celmath/mathlib.h>
#include <celmath/solve.h>
#include "astro.h"
#include "orbit.h"
#include "body.h"

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
        proc.sample(t, positionAtE(dE * i));
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
        proc.sample(start + dt * i, positionAtTime(start + dt * i));
}


static EllipticalOrbit* StateVectorToOrbit(const Point3d& position,
                                           const Vec3d& v,
                                           double mass,
                                           double t)
{
    Vec3d R = position - Point3d(0.0, 0.0, 0.0);
    Vec3d L = R ^ v;
    double magR = R.length();
    double magL = L.length();
    double magV = v.length();
    L *= (1.0 / magL);

    Vec3d W = L ^ (R / magR);

    double G = astro::G * 1e-9; // convert from meters to kilometers
    double GM = G * mass;

    // Compute the semimajor axis
    double a = 1.0 / (2.0 / magR - square(magV) / GM);

    // Compute the eccentricity
    double p = square(magL) / GM;
    double q = R * v;
    double ex = 1.0 - magR / a;
    double ey = q / sqrt(a * GM);
    double e = sqrt(ex * ex + ey * ey);

    // Compute the mean anomaly
    double E = atan2(ey, ex);
    double M = E - e * sin(E);
    
    // Compute the inclination
    double cosi = L * Vec3d(0, 1.0, 0);
    double i = 0.0;
    if (cosi < 1.0)
        i = acos(cosi);

    // Compute the longitude of ascending node
    double Om = atan2(L.x, L.z);

    // Compute the argument of pericenter
    Vec3d U = R / magR;
    double s_nu = (v * U) * sqrt(p / GM);
    double c_nu = (v * W) * sqrt(p / GM) - 1;
    s_nu /= e;
    c_nu /= e;
    Vec3d P = U * c_nu - W * s_nu;
    Vec3d Q = U * s_nu + W * c_nu;
    double om = atan2(P.y, Q.y);

    // Compute the period
    double T = 2 * PI * sqrt(cube(a) / GM);
    T = T / 86400.0; // Convert from seconds to days

#if 0
    cout << "a: " << astro::kilometersToAU(a) << '\n';
    cout << "e: " << e << '\n';
    cout << "i: " << radToDeg(i) << '\n';
    cout << "Om: " << radToDeg(Om) << '\n';
    cout << "om: " << radToDeg(om) << '\n';
    cout << "M: " << radToDeg(M) << '\n';
    cout << "T: " << T << '\n';
#endif

    return new EllipticalOrbit(a * (1 - e), e, i, Om, om, M, T, t);
}


MixedOrbit::MixedOrbit(Orbit* orbit, double t0, double t1, double mass) :
    primary(orbit),
    afterApprox(NULL),
    beforeApprox(NULL),
    begin(t0),
    end(t1),
    boundingRadius(0.0)
{
    assert(t1 > t0);
    assert(orbit != NULL);
    
    double dt = 1.0 / 1440.0; // 1 minute
    Point3d p0 = orbit->positionAtTime(t0);
    Point3d p1 = orbit->positionAtTime(t1);
    Vec3d v0 = (orbit->positionAtTime(t0 + dt) - p0) / (86400 * dt);
    Vec3d v1 = (orbit->positionAtTime(t1 + dt) - p1) / (86400 * dt);
    beforeApprox = StateVectorToOrbit(p0, v0, mass, t0);
    afterApprox = StateVectorToOrbit(p1, v1, mass, t1);

    boundingRadius = beforeApprox->getBoundingRadius();
    if (primary->getBoundingRadius() > boundingRadius)
        boundingRadius = primary->getBoundingRadius();
    if (afterApprox->getBoundingRadius() > boundingRadius)
        boundingRadius = afterApprox->getBoundingRadius();
}

MixedOrbit::~MixedOrbit()
{
    if (primary != NULL)
        delete primary;
    if (beforeApprox != NULL)
        delete beforeApprox;
    if (afterApprox != NULL)
        delete afterApprox;
}


Point3d MixedOrbit::positionAtTime(double jd) const
{
    if (jd < begin)
        return beforeApprox->positionAtTime(jd);
    else if (jd < end)
        return primary->positionAtTime(jd);
    else
        return afterApprox->positionAtTime(jd);
}


double MixedOrbit::getPeriod() const
{
    return primary->getPeriod();
}


double MixedOrbit::getBoundingRadius() const
{
    return boundingRadius;
}


void MixedOrbit::sample(double t0, double t1, int nSamples,
                        OrbitSampleProc& proc) const
{
    Orbit* o;
    if (t0 < begin)
        o = beforeApprox;
    else if (t0 < end)
        o = primary;
    else
        o = afterApprox;
    o->sample(t0, t1, nSamples, proc);
}



SynchronousOrbit::SynchronousOrbit(const Body& _body,
                                   const Point3d& _position) :
    body(_body),
    position(_position)
{
}


SynchronousOrbit::~SynchronousOrbit()
{
}


Point3d SynchronousOrbit::positionAtTime(double jd) const
{
    //Quatd q = body.getEclipticalToGeographic(jd);
    Quatd q = body.getEquatorialToGeographic(jd);
    return position * q.toMatrix3();
}


double SynchronousOrbit::getPeriod() const
{
    return body.getRotationElements().period;
}


double SynchronousOrbit::getBoundingRadius() const
{
    return position.distanceFromOrigin();
}


void SynchronousOrbit::sample(double, double, int, OrbitSampleProc&) const
{
    // Empty method--we never want to show a synchronous orbit.
}
