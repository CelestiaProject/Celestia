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


class OrbitSampleProc;

class Orbit
{
public:
    virtual Point3d positionAtTime(double) const = 0;
    virtual double getPeriod() const = 0;
    virtual double getBoundingRadius() const = 0;
    virtual void sample(double, double, int, OrbitSampleProc&) const = 0;
    virtual bool isPeriodic() const { return true; };

    // Return the time range over which the orbit is valid; if the orbit
    // is always valid, begin and end should be equal.
    virtual void getValidRange(double& begin, double& end) const
        { begin = 0.0; end = 0.0; };
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
    virtual void sample(double, double, int, OrbitSampleProc&) const;

private:
    double eccentricAnomaly(double) const;
    Point3d positionAtE(double) const;

    double pericenterDistance;
    double eccentricity;
    double inclination;
    double ascendingNode;
    double argOfPeriapsis;
    double meanAnomalyAtEpoch;
    double period;
    double epoch;
};


class OrbitSampleProc
{
 public:
    virtual void sample(double t, const Point3d&) = 0;
};



/*! Custom orbit classes should be derived from CachingOrbit.  The custom
 * orbits can be expensive to compute, with more than 50 periodic terms.
 * Celestia may need require position of a planet more than once per frame; in
 * order to avoid redundant calculation, the CachingOrbit class saves the
 * result of the last calculation and uses it if the time matches the cached
 * time.
 */
class CachingOrbit : public Orbit
{
public:
    CachingOrbit() : lastTime(1.0e-30) {};

    virtual Point3d computePosition(double jd) const = 0;
    virtual double getPeriod() const = 0;
    virtual double getBoundingRadius() const = 0;

    Point3d positionAtTime(double jd) const;

    virtual void sample(double, double, int, OrbitSampleProc& proc) const;

private:
    mutable Point3d lastPosition;
    mutable double lastTime;
};


/*! A mixed orbit is a composite orbit, typically used when you have a
 *  custom orbit calculation that is only valid over limited span of time.
 *  When a mixed orbit is constructed, it computes elliptical orbits
 *  to approximate the behavior of the primary orbit before and after the
 *  span over which it is valid.
 */
class MixedOrbit : public Orbit
{
 public:
    MixedOrbit(Orbit* orbit, double t0, double t1, double mass);
    virtual ~MixedOrbit();

    Point3d positionAtTime(double jd) const;
    virtual double getPeriod() const;
    virtual double getBoundingRadius() const;
    virtual void sample(double, double, int, OrbitSampleProc& proc) const;

 private:
    Orbit* primary;
    EllipticalOrbit* afterApprox;
    EllipticalOrbit* beforeApprox;
    double begin;
    double end;
    double boundingRadius;
};


class Body;

/*! An object in a synchronous orbit will always hover of the same spot on
 *  the surface of the body it orbits.  Only equatorial orbits of a certain
 *  radius are stable in the real world.  In Celestia, synchronous orbits are
 *  a convenient way to fix objects to a planet surface.
 */
class SynchronousOrbit : public Orbit
{
 public:
    SynchronousOrbit(const Body& _body, const Point3d& _position);
    virtual ~SynchronousOrbit();

    Point3d positionAtTime(double jd) const;
    virtual double getPeriod() const;
    virtual double getBoundingRadius() const;
    virtual void sample(double, double, int, OrbitSampleProc& proc) const;

 private:
    const Body& body;
    Point3d position;
};


#endif // _ORBIT_H_
