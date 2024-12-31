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
#include "rotation.h"

namespace celestia::ephem
{

namespace
{

// Orbital velocity is computed by differentiation for orbits that don't
// override velocityAtTime().
constexpr double ORBITAL_VELOCITY_DIFF_DELTA = 1.0 / 1440.0;

// Follow hyperbolic orbit trajectories out to at least 1000 au
constexpr double HyperbolicMinBoundingRadius = 1000.0 * astro::KM_PER_AU<double>;

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
        math::sincos(x, s, c);
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
        math::sincos(x, s, c);
        s *= ecc;
        c *= ecc;
        double f = x - s - M;
        double f1 = 1.0 - c;
        double f2 = s;
        x += -5.0 * f / (f1 + math::sign(f1) * std::sqrt(std::abs(16.0 * f1 * f1 - 20.0 * f * f2)));

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
        x += -5.0 * f / (f1 + math::sign(f1) * std::sqrt(std::abs(16.0 * f1 * f1 - 20.0 * f * f2)));

        return x;
    }
};

std::unique_ptr<Orbit>
CreateKeplerOrbit(const astro::KeplerElements& elements, double epoch)
{
    if (elements.eccentricity < 1.0)
        return std::make_unique<EllipticalOrbit>(elements, epoch);

    return std::make_unique<HyperbolicOrbit>(elements, epoch);
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

                positionError = (pInterp - pTest).norm();
            }
        }

        t = t + dt;
        lastP = p1;
        lastV = v1;

        proc.sample(t, lastP, lastV);
    }
}


Eigen::Vector3d Orbit::velocityAtTime(double tdb) const
{
    Eigen::Vector3d p0 = positionAtTime(tdb);
    Eigen::Vector3d p1 = positionAtTime(tdb + ORBITAL_VELOCITY_DIFF_DELTA);
    return (p1 - p0) * (1.0 / ORBITAL_VELOCITY_DIFF_DELTA);
}


EllipticalOrbit::EllipticalOrbit(const astro::KeplerElements& _elements, double _epoch) :
    semiMajorAxis(_elements.semimajorAxis),
    eccentricity(_elements.eccentricity),
    meanAnomalyAtEpoch(_elements.meanAnomaly),
    period(_elements.period),
    epoch(_epoch),
    orbitPlaneRotation((math::ZRotation(_elements.longAscendingNode) *
                        math::XRotation(_elements.inclination) *
                        math::ZRotation(_elements.argPericenter)).toRotationMatrix())
{
    assert(eccentricity >= 0.0 && eccentricity < 1.0);
    assert(semiMajorAxis >= 0.0);
    assert(period != 0.0);
    semiMinorAxis = semiMajorAxis * std::sqrt(1.0 - math::square(eccentricity));
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
        return math::solve_iteration_fixed(SolveKeplerFunc1(eccentricity, M), M, 5).first;
    }
    if (eccentricity < 0.9)
    {
        // Higher eccentricity elliptical orbit; use a more complex but
        // much faster converging iteration.
        return math::solve_iteration_fixed(SolveKeplerFunc2(eccentricity, M), M, 6).first;
    }

    // Extremely stable Laguerre-Conway method for solving Kepler's
    // equation.  Only use this for high-eccentricity orbits, as it
    // requires more calcuation.
    double E = M + 0.85 * eccentricity * math::sign(std::sin(M));
    return math::solve_iteration_fixed(SolveKeplerLaguerreConway(eccentricity, M), E, 8).first;
}


// Compute the position at the specified eccentric
// anomaly E.
Eigen::Vector3d EllipticalOrbit::positionAtE(double E) const
{
    double x = semiMajorAxis * (std::cos(E) - eccentricity);
    double y = semiMinorAxis * std::sin(E);

    Eigen::Vector3d p = orbitPlaneRotation * Eigen::Vector3d(x, y, 0);

    // Convert to Celestia's internal coordinate system
    return Eigen::Vector3d(p.x(), p.z(), -p.y());
}


// Compute the velocity at the specified eccentric
// anomaly E.
Eigen::Vector3d EllipticalOrbit::velocityAtE(double E, double meanMotion) const
{
    double sinE;
    double cosE;
    math::sincos(E, sinE, cosE);

    double edot = meanMotion / (1.0 - eccentricity * cosE);

    double x = -semiMajorAxis * sinE * edot;
    double y =  semiMinorAxis * cosE * edot;

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

    return velocityAtE(E, meanMotion);
}


double EllipticalOrbit::getPeriod() const
{
    return period;
}


double EllipticalOrbit::getBoundingRadius() const
{
    // TODO: watch out for unbounded parabolic and hyperbolic orbits
    return semiMajorAxis * (1.0 + eccentricity);
}


HyperbolicOrbit::HyperbolicOrbit(const astro::KeplerElements& _elements, double _epoch) :
    semiMajorAxis(_elements.semimajorAxis),
    eccentricity(_elements.eccentricity),
    meanAnomalyAtEpoch(_elements.meanAnomaly),
    epoch(_epoch),
    orbitPlaneRotation((math::ZRotation(_elements.longAscendingNode) *
                        math::XRotation(_elements.inclination) *
                        math::ZRotation(_elements.argPericenter)).toRotationMatrix())
{
    assert(eccentricity > 1.0);
    assert(semiMajorAxis <= 0.0);
    assert(_elements.period != 0.0);
    semiMinorAxis = semiMajorAxis * std::sqrt(math::square(eccentricity) - 1.0);
    meanMotion = 2.0 * celestia::numbers::pi / _elements.period;

    // determine start and end epoch from when the object hits the bounding radius
    double pericenterDistance = semiMajorAxis * (1.0 - eccentricity);
    double boundingRadius = getBoundingRadius();
    double cosTrueAnomaly = (pericenterDistance / boundingRadius - 1.0) / eccentricity;
    double eccAnomaly = std::acosh((eccentricity + cosTrueAnomaly) / (1.0 + eccentricity * cosTrueAnomaly));
    double meanAnomaly = eccentricity * std::sinh(eccAnomaly) - eccAnomaly;
    double deltaT = std::abs(meanAnomaly / meanMotion);
    startEpoch = epoch - deltaT;
    endEpoch = epoch + deltaT;
}


double HyperbolicOrbit::eccentricAnomaly(double M) const
{
    // Laguerre-Conway method for hyperbolic (ecc > 1) orbits.
    if (M == 0.0)
        return 0.0;
    double E = std::log(2.0 * std::abs(M) / eccentricity + 1.85);
    return std::copysign(math::solve_iteration_fixed(SolveKeplerLaguerreConwayHyp(eccentricity, std::abs(M)), E, 30).first, M);
}


// Compute the position at the specified eccentric
// anomaly E.
Eigen::Vector3d HyperbolicOrbit::positionAtE(double E) const
{
    double x = -semiMajorAxis * (eccentricity - std::cosh(E));
    double y = -semiMinorAxis * std::sinh(E);

    Eigen::Vector3d p = orbitPlaneRotation * Eigen::Vector3d(x, y, 0);

    // Convert to Celestia's internal coordinate system
    return Eigen::Vector3d(p.x(), p.z(), -p.y());
}


// Compute the velocity at the specified eccentric
// anomaly E.
Eigen::Vector3d HyperbolicOrbit::velocityAtE(double E) const
{
    double coshE = std::cosh(E);
    double edot = meanMotion / (eccentricity * coshE - 1);

    double x =  semiMajorAxis * std::sinh(E) * edot;
    double y = -semiMinorAxis * coshE * edot;

    Eigen::Vector3d v = orbitPlaneRotation * Eigen::Vector3d(x, y, 0);

    // Convert to Celestia's coordinate system
    return Eigen::Vector3d(v.x(), v.z(), -v.y());
}


// Return the offset from the center
Eigen::Vector3d HyperbolicOrbit::positionAtTime(double t) const
{
    t = t - epoch;
    double meanAnomaly = meanAnomalyAtEpoch + t * meanMotion;
    double E = eccentricAnomaly(meanAnomaly);

    return positionAtE(E);
}


Eigen::Vector3d HyperbolicOrbit::velocityAtTime(double t) const
{
    t = t - epoch;
    double meanAnomaly = meanAnomalyAtEpoch + t * meanMotion;
    double E = eccentricAnomaly(meanAnomaly);

    return velocityAtE(E);
}


double HyperbolicOrbit::getPeriod() const
{
    // As this is a non-periodic orbit, we return the sample window here
    return endEpoch - startEpoch;
}


double HyperbolicOrbit::getBoundingRadius() const
{
    return std::max(2.0 * semiMajorAxis * (1.0 - eccentricity), HyperbolicMinBoundingRadius);
}


bool HyperbolicOrbit::isPeriodic() const
{
    return false;
}


void HyperbolicOrbit::getValidRange(double& begin, double& end) const
{
    begin = startEpoch;
    end = endEpoch;
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


MixedOrbit::MixedOrbit(const std::shared_ptr<const Orbit>& orbit, double t0, double t1, double mass) :
    primary(orbit),
    begin(t0),
    end(t1),
    boundingRadius(0.0)
{
    assert(t1 > t0);
    assert(primary != nullptr);

    double dt = 1.0 / 1440.0; // 1 minute
    Eigen::Vector3d p0 = primary->positionAtTime(t0);
    Eigen::Vector3d p1 = primary->positionAtTime(t1);
    Eigen::Vector3d v0 = (primary->positionAtTime(t0 + dt) - p0) / dt;
    Eigen::Vector3d v1 = (primary->positionAtTime(t1 + dt) - p1) / dt;

    constexpr double G = astro::G * math::square(86400.0) * 1e-9; // convert to km/day
    double Gmass = G * mass;

    auto keplerElements = astro::StateVectorToElements(p0, v0, Gmass);
    beforeApprox = CreateKeplerOrbit(keplerElements, t0);
    keplerElements = astro::StateVectorToElements(p1, v1, Gmass);
    afterApprox = CreateKeplerOrbit(keplerElements, t1);

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

} // end namespace celestia::ephem
