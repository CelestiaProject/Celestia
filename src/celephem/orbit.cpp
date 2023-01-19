// orbit.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "orbit.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <functional>
#include <utility>

#include <celcompat/numbers.h>
#include <celengine/body.h>
#include <celmath/mathlib.h>
#include <celmath/solve.h>
#include <celmath/geomutil.h>

namespace
{

// Orbital velocity is computed by differentiation for orbits that don't
// override velocityAtTime().
constexpr double ORBITAL_VELOCITY_DIFF_DELTA = 1.0 / 1440.0;

Eigen::Vector3d cubicInterpolate(const Eigen::Vector3d& p0, const Eigen::Vector3d& v0,
                                 const Eigen::Vector3d& p1, const Eigen::Vector3d& v1,
                                 double t)
{
    return p0 + (((2.0 * (p0 - p1) + v1 + v0) * (t * t * t)) +
                ((3.0 * (p1 - p0) - 2.0 * v0 - v1) * (t * t)) +
                (v0 * t));
}

// Standard iteration for solving Kepler's Equation
struct SolveKeplerFunc1
{
    double ecc;
    double M;

    SolveKeplerFunc1(double _ecc, double _M) : ecc(_ecc), M(_M) {};

    double operator()(double x) const
    {
        return M + ecc * std::sin(x);
    }
};


// Faster converging iteration for Kepler's Equation; more efficient
// than above for orbits with eccentricities greater than 0.3.  This
// is from Jean Meeus's _Astronomical Algorithms_ (2nd ed), p. 199
struct SolveKeplerFunc2
{
    double ecc;
    double M;

    SolveKeplerFunc2(double _ecc, double _M) : ecc(_ecc), M(_M) {};

    double operator()(double x) const
    {
        double s;
        double c;
        celmath::sincos(x, s, c);
        return x + (M + ecc * s - x) / (1.0 - ecc * c);
    }
};


struct SolveKeplerLaguerreConway
{
    double ecc;
    double M;

    SolveKeplerLaguerreConway(double _ecc, double _M) : ecc(_ecc), M(_M) {};

    double operator()(double x) const
    {
        double s;
        double c;
        celmath::sincos(x, s, c);
        s *= ecc;
        c *= ecc;
        double f = x - s - M;
        double f1 = 1.0 - c;
        double f2 = s;
        x += -5.0 * f / (f1 + celmath::sign(f1) * std::sqrt(std::abs(16.0 * f1 * f1 - 20.0 * f * f2)));

        return x;
    }
};

struct SolveKeplerLaguerreConwayHyp
{
    double ecc;
    double M;

    SolveKeplerLaguerreConwayHyp(double _ecc, double _M) : ecc(_ecc), M(_M) {};

    double operator()(double x) const
    {
        double s = ecc * std::sinh(x);
        double c = ecc * std::cosh(x);
        double f = s - x - M;
        double f1 = c - 1.0;
        double f2 = s;
        x += -5.0 * f / (f1 + celmath::sign(f1) * std::sqrt(std::abs(16.0 * f1 * f1 - 20.0 * f * f2)));

        return x;
    }
};


std::unique_ptr<EllipticalOrbit>
StateVectorToOrbit(const Eigen::Vector3d& position,
                   const Eigen::Vector3d& v,
                   double mass,
                   double t)
{
    Eigen::Vector3d R = position;
    Eigen::Vector3d L = R.cross(v);
    double magR = R.norm();
    double magL = L.norm();
    double magV = v.norm();
    L *= (1.0 / magL);

    Eigen::Vector3d W = L.cross(R / magR);

    double G = astro::G * 1e-9; // convert from meters to kilometers
    double GM = G * mass;

    // Compute the semimajor axis
    double a = 1.0 / (2.0 / magR - celmath::square(magV) / GM);

    // Compute the eccentricity
    double p = celmath::square(magL) / GM;
    double q = R.dot(v);
    double ex = 1.0 - magR / a;
    double ey = q / std::sqrt(a * GM);
    double e = std::sqrt(ex * ex + ey * ey);

    // Compute the mean anomaly
    double E = std::atan2(ey, ex);
    double M = E - e * std::sin(E);

    // Compute the inclination
    double cosi = L.dot(Eigen::Vector3d::UnitY());
    double i = 0.0;
    if (cosi < 1.0)
        i = std::acos(cosi);

    // Compute the longitude of ascending node
    double Om = std::atan2(L.x(), L.z());

    // Compute the argument of pericenter
    Eigen::Vector3d U = R / magR;
    double s_nu = (v.dot(U)) * std::sqrt(p / GM);
    double c_nu = (v.dot(W)) * std::sqrt(p / GM) - 1;
    s_nu /= e;
    c_nu /= e;
    Eigen::Vector3d P = U * c_nu - W * s_nu;
    Eigen::Vector3d Q = U * s_nu + W * c_nu;
    double om = std::atan2(P.y(), Q.y());

    // Compute the period
    double T = 2 * celestia::numbers::pi * std::sqrt(celmath::cube(a) / GM);
    T /= 86400.0; // Convert from seconds to days

    return std::make_unique<EllipticalOrbit>(a * (1.0 - e), e, i, Om, om, M, T, t);
}

} // end unnamed namespace

/** Sample the orbit over the time range [ startTime, endTime ] using the
  * default sampling parameters for the orbit type.
  *
  * Subclasses of orbit should override this method as necessary. The default
  * implementation uses an adaptive sampling scheme with the following defaults:
  *    tolerance: 1 km
  *    start step: T / 1e5
  *    min step: T / 1e7
  *    max step: T / 100
  *
  * Where T is either the mean orbital period for periodic orbits or the valid
  * time span for aperiodic trajectories.
  */
void Orbit::sample(double startTime, double endTime, OrbitSampleProc& proc) const
{
    double span = 0.0;
    if (isPeriodic())
    {
        span = getPeriod();
    }
    else
    {
        double startValidInterval = 0.0;
        double endValidInterval = 0.0;
        getValidRange(startValidInterval, endValidInterval);
        if (startValidInterval == endValidInterval)
        {
            span = endValidInterval - startValidInterval;
        }
        else
        {
            span = endTime - startTime;
        }
    }

    AdaptiveSamplingParameters samplingParams;
    samplingParams.tolerance = 1.0; // kilometers
    samplingParams.maxStep = span / 100.0;
    samplingParams.minStep = span / 1.0e7;
    samplingParams.startStep = span / 1.0e5;

    adaptiveSample(startTime, endTime, proc, samplingParams);
}


/** Adaptively sample the orbit over the range [ startTime, endTime ].
  */
void Orbit::adaptiveSample(double startTime, double endTime, OrbitSampleProc& proc, const AdaptiveSamplingParameters& samplingParams) const
{
    double startStepSize = samplingParams.startStep;
    double maxStepSize   = samplingParams.maxStep;
    double minStepSize   = samplingParams.minStep;
    double tolerance     = samplingParams.tolerance;
    double t = startTime;
    const double stepFactor = 1.25;

    Eigen::Vector3d lastP = positionAtTime(t);
    Eigen::Vector3d lastV = velocityAtTime(t);
    proc.sample(t, lastP, lastV);
    int sampCount = 0;
    int nTests = 0;

    while (t < endTime)
    {
        // Make sure that we don't go past the end of the sample interval
        maxStepSize = std::min(maxStepSize, endTime - t);
        double dt = std::min(maxStepSize, startStepSize * 2.0);

        Eigen::Vector3d p1 = positionAtTime(t + dt);
        Eigen::Vector3d v1 = velocityAtTime(t + dt);

        double tmid = t + dt / 2.0;
        Eigen::Vector3d pTest = positionAtTime(tmid);
        Eigen::Vector3d pInterp = cubicInterpolate(lastP, lastV * dt,
                                                   p1, v1 * dt,
                                                   0.5);
        ++nTests;

        double positionError = (pInterp - pTest).norm();

        // Error is greater than tolerance; decrease the step until the
        // error is within the tolerance.
        if (positionError > tolerance)
        {
            while (positionError > tolerance && dt > minStepSize)
            {
                dt /= stepFactor;

                p1 = positionAtTime(t + dt);
                v1 = velocityAtTime(t + dt);

                tmid = t + dt / 2.0;
                pTest = positionAtTime(tmid);
                pInterp = cubicInterpolate(lastP, lastV * dt,
                                           p1, v1 * dt,
                                           0.5);
                nTests++;

                positionError = (pInterp - pTest).norm();
            }
        }
        else
        {
            // Error is less than the tolerance; increase the step size until the
            // tolerance is just exceeded.
            while (positionError < tolerance && dt < maxStepSize)
            {
                dt *= stepFactor;

                p1 = positionAtTime(t + dt);
                v1 = velocityAtTime(t + dt);

                tmid = t + dt / 2.0;
                pTest = positionAtTime(tmid);
                pInterp = cubicInterpolate(lastP, lastV * dt,
                                           p1, v1 * dt,
                                           0.5);
                nTests++;

                positionError = (pInterp - pTest).norm();
            }
        }

        t = t + dt;
        lastP = p1;
        lastV = v1;

        proc.sample(t, lastP, lastV);
        sampCount++;
    }

    // Statistics for debugging
    // clog << "Orbit samples: " << sampCount << ", nTests: " << nTests << endl;
}


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
    meanAnomalyAtEpoch(_meanAnomalyAtEpoch),
    period(_period),
    epoch(_epoch),
    orbitPlaneRotation((celmath::ZRotation(_ascendingNode) *
                        celmath::XRotation(_inclination) *
                        celmath::ZRotation(_argOfPeriapsis)).toRotationMatrix())
{
}


Eigen::Vector3d Orbit::velocityAtTime(double tdb) const
{
    Eigen::Vector3d p0 = positionAtTime(tdb);
    Eigen::Vector3d p1 = positionAtTime(tdb + ORBITAL_VELOCITY_DIFF_DELTA);
    return (p1 - p0) * (1.0 / ORBITAL_VELOCITY_DIFF_DELTA);
}


double EllipticalOrbit::eccentricAnomaly(double M) const
{
    if (eccentricity == 0.0)
    {
        // Circular orbit
        return M;
    }
    if (eccentricity < 0.2)
    {
        // Low eccentricity, so use the standard iteration technique
        return celmath::solve_iteration_fixed(SolveKeplerFunc1(eccentricity, M), M, 5).first;
    }
    else if (eccentricity < 0.9)
    {
        // Higher eccentricity elliptical orbit; use a more complex but
        // much faster converging iteration.
        return celmath::solve_iteration_fixed(SolveKeplerFunc2(eccentricity, M), M, 6).first;
    }
    else if (eccentricity < 1.0)
    {
        // Extremely stable Laguerre-Conway method for solving Kepler's
        // equation.  Only use this for high-eccentricity orbits, as it
        // requires more calcuation.
        double E = M + 0.85 * eccentricity * celmath::sign(std::sin(M));
        return celmath::solve_iteration_fixed(SolveKeplerLaguerreConway(eccentricity, M), E, 8).first;
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
        double E = std::log(2.0 * M / eccentricity + 1.85);
        return celmath::solve_iteration_fixed(SolveKeplerLaguerreConwayHyp(eccentricity, M), E, 30).first;
    }
}


// Compute the position at the specified eccentric
// anomaly E.
Eigen::Vector3d EllipticalOrbit::positionAtE(double E) const
{
    double x, y;

    if (eccentricity < 1.0)
    {
        double a = pericenterDistance / (1.0 - eccentricity);
        x = a * (std::cos(E) - eccentricity);
        y = a * std::sqrt(1.0 - celmath::square(eccentricity)) * std::sin(E);
    }
    else if (eccentricity > 1.0)
    {
        double a = pericenterDistance / (1.0 - eccentricity);
        x = -a * (eccentricity - std::cosh(E));
        y = -a * std::sqrt(celmath::square(eccentricity) - 1.0) * std::sinh(E);
    }
    else
    {
        // TODO: Handle parabolic orbits
        x = 0.0;
        y = 0.0;
    }

    Eigen::Vector3d p = orbitPlaneRotation * Eigen::Vector3d(x, y, 0);

    // Convert to Celestia's internal coordinate system
    return Eigen::Vector3d(p.x(), p.z(), -p.y());
}


// Compute the velocity at the specified eccentric
// anomaly E.
Eigen::Vector3d EllipticalOrbit::velocityAtE(double E) const
{
    double x, y;

    if (eccentricity < 1.0)
    {
        double a = pericenterDistance / (1.0 - eccentricity);
        double b = a * std::sqrt(1.0 - celmath::square(eccentricity));
        double sinE;
        double cosE;
        celmath::sincos(E, sinE, cosE);

        double meanMotion = 2.0 * celestia::numbers::pi / period;
        double edot = meanMotion / (1.0 - eccentricity * cosE);

        x = -a * sinE * edot;
        y =  b * cosE * edot;
    }
    else if (eccentricity > 1.0)
    {
        double a = pericenterDistance / (1.0 - eccentricity);
        x = -a * (eccentricity - std::cosh(E));
        y = -a * std::sqrt(celmath::square(eccentricity) - 1.0) * std::sinh(E);
    }
    else
    {
        // TODO: Handle parabolic orbits
        x = 0.0;
        y = 0.0;
    }

    Eigen::Vector3d v = orbitPlaneRotation * Eigen::Vector3d(x, y, 0);

    // Convert to Celestia's coordinate system
    return Eigen::Vector3d(v.x(), v.z(), -v.y());
}


// Return the offset from the center
Eigen::Vector3d EllipticalOrbit::positionAtTime(double t) const
{
    t = t - epoch;
    double meanMotion = 2.0 * celestia::numbers::pi / period;
    double meanAnomaly = meanAnomalyAtEpoch + t * meanMotion;
    double E = eccentricAnomaly(meanAnomaly);

    return positionAtE(E);
}


Eigen::Vector3d EllipticalOrbit::velocityAtTime(double t) const
{
    t = t - epoch;
    double meanMotion = 2.0 * celestia::numbers::pi / period;
    double meanAnomaly = meanAnomalyAtEpoch + t * meanMotion;
    double E = eccentricAnomaly(meanAnomaly);

    return velocityAtE(E);
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


Eigen::Vector3d CachingOrbit::positionAtTime(double jd) const
{
    if (jd != lastTime)
    {
        lastTime = jd;
        lastPosition = computePosition(jd);
        positionCacheValid = true;
        velocityCacheValid = false;
    }
    else if (!positionCacheValid)
    {
        lastPosition = computePosition(jd);
        positionCacheValid = true;
    }

    return lastPosition;
}


Eigen::Vector3d CachingOrbit::velocityAtTime(double jd) const
{
    if (jd != lastTime)
    {
        lastVelocity = computeVelocity(jd);
        lastTime = jd;  // must be set *after* call to computeVelocity
        positionCacheValid = false;
        velocityCacheValid = true;
    }
    else if (!velocityCacheValid)
    {
        lastVelocity = computeVelocity(jd);
        velocityCacheValid = true;
    }

    return lastVelocity;
}


/*! Calculate the velocity at the specified time (units are
 *  kilometers / Julian day.) The default implementation just
 *  differentiates the position.
 */
Eigen::Vector3d CachingOrbit::computeVelocity(double jd) const
{
    // Compute the velocity by differentiating.
    Eigen::Vector3d p0 = positionAtTime(jd);

    // Call computePosition() instead of positionAtTime() so that we
    // don't affect the cached value.
    // TODO: check the valid ranges of the orbit to make sure that
    // jd+dt is still in range.
    Eigen::Vector3d p1 = computePosition(jd + ORBITAL_VELOCITY_DIFF_DELTA);

    return (p1 - p0) * (1.0 / ORBITAL_VELOCITY_DIFF_DELTA);
}


MixedOrbit::MixedOrbit(std::unique_ptr<Orbit>&& orbit, double t0, double t1, double mass) :
    primary(std::move(orbit)),
    afterApprox(nullptr),
    beforeApprox(nullptr),
    begin(t0),
    end(t1),
    boundingRadius(0.0)
{
    assert(t1 > t0);
    assert(primary != nullptr);

    double dt = 1.0 / 1440.0; // 1 minute
    Eigen::Vector3d p0 = primary->positionAtTime(t0);
    Eigen::Vector3d p1 = primary->positionAtTime(t1);
    Eigen::Vector3d v0 = (primary->positionAtTime(t0 + dt) - p0) / (86400.0 * dt);
    Eigen::Vector3d v1 = (primary->positionAtTime(t1 + dt) - p1) / (86400.0 * dt);
    beforeApprox = StateVectorToOrbit(p0, v0, mass, t0);
    afterApprox = StateVectorToOrbit(p1, v1, mass, t1);

    boundingRadius = beforeApprox->getBoundingRadius();
    if (primary->getBoundingRadius() > boundingRadius)
        boundingRadius = primary->getBoundingRadius();
    if (afterApprox->getBoundingRadius() > boundingRadius)
        boundingRadius = afterApprox->getBoundingRadius();
}


Eigen::Vector3d MixedOrbit::positionAtTime(double jd) const
{
    if (jd < begin)
        return beforeApprox->positionAtTime(jd);
    else if (jd < end)
        return primary->positionAtTime(jd);
    else
        return afterApprox->positionAtTime(jd);
}


Eigen::Vector3d MixedOrbit::velocityAtTime(double jd) const
{
    if (jd < begin)
        return beforeApprox->velocityAtTime(jd);
    else if (jd < end)
        return primary->velocityAtTime(jd);
    else
        return afterApprox->velocityAtTime(jd);
}


double MixedOrbit::getPeriod() const
{
    return primary->getPeriod();
}


double MixedOrbit::getBoundingRadius() const
{
    return boundingRadius;
}


void MixedOrbit::sample(double startTime, double endTime, OrbitSampleProc& proc) const
{
    const Orbit* o;
    if (startTime < begin)
        o = beforeApprox.get();
    else if (startTime < end)
        o = primary.get();
    else
        o = afterApprox.get();
    o->sample(startTime, endTime, proc);
}


/*** FixedOrbit ***/

FixedOrbit::FixedOrbit(const Eigen::Vector3d& pos) :
    position(pos)
{
}

Eigen::Vector3d
FixedOrbit::positionAtTime(double /*tjd*/) const
{
    return position;
}


bool
FixedOrbit::isPeriodic() const
{
    return false;
}


double
FixedOrbit::getPeriod() const
{
    return 1.0;
}


double
FixedOrbit::getBoundingRadius() const
{
    return position.norm() * 1.1;
}


void
FixedOrbit::sample(double /* startTime */, double /* endTime */, OrbitSampleProc& /*proc*/) const
{
    // Don't add any samples. This will prevent a fixed trajectory from
    // every being drawn when orbit visualization is enabled.
}


/*** SynchronousOrbit ***/
// TODO: eliminate this class once body-fixed reference frames are implemented
SynchronousOrbit::SynchronousOrbit(const Body& _body,
                                   const Eigen::Vector3d& _position) :
    body(_body),
    position(_position)
{
}


Eigen::Vector3d SynchronousOrbit::positionAtTime(double jd) const
{
    return body.getEquatorialToBodyFixed(jd).conjugate() * position;
}


double SynchronousOrbit::getPeriod() const
{
    return body.getRotationModel(0.0)->getPeriod();
}


double SynchronousOrbit::getBoundingRadius() const
{
    return position.norm();
}


void SynchronousOrbit::sample(double /* startTime */, double /* endTime */, OrbitSampleProc& /*proc*/) const
{
    // Empty method--we never want to show a synchronous orbit.
}
