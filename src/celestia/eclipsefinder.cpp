// eclipsefinder.cpp by Christophe Teyssier <chris@teyssier.org>
// adapted form wineclipses.cpp by Kendrix <kendrix@wanadoo.fr>
//
// Copyright (C) 2001-2009, the Celestia Development Team
//
// Compute Solar Eclipses for our Solar System planets
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstring>
#include <cassert>
#include "eclipsefinder.h"
#include "celmath/ray.h"
#include "celmath/distance.h"

using namespace Eigen;
using namespace std;
using namespace celmath;


constexpr const double dT = 1.0 / (24.0 * 60.0);
constexpr const int EclipseObjectMask = Body::Planet      |
                                        Body::Moon        |
                                        Body::MinorMoon   |
                                        Body::DwarfPlanet |
                                        Body::Asteroid;

// TODO: share this constant and function with render.cpp
static const float MinRelativeOccluderRadius = 0.005f;

EclipseFinder::EclipseFinder(Body* _body,
                             EclipseFinderWatcher* _watcher) :
    body(_body),
    watcher(_watcher)
{
};


bool testEclipse(const Body& receiver, const Body& caster, double now)
{
    // Ignore situations where the shadow casting body is much smaller than
    // the receiver, as these shadows aren't likely to be relevant.  Also,
    // ignore eclipses where the caster is not an ellipsoid, since we can't
    // generate correct shadows in this case.
    if (caster.getRadius() >= receiver.getRadius() * MinRelativeOccluderRadius &&
        caster.isEllipsoid())
    {
        // All of the eclipse related code assumes that both the caster
        // and receiver are spherical.  Irregular receivers will work more
        // or less correctly, but casters that are sufficiently non-spherical
        // will produce obviously incorrect shadows.  Another assumption we
        // make is that the distance between the caster and receiver is much
        // less than the distance between the sun and the receiver.  This
        // approximation works everywhere in the solar system, and likely
        // works for any orbitally stable pair of objects orbiting a star.
        Vector3d posReceiver = receiver.getAstrocentricPosition(now);
        Vector3d posCaster = caster.getAstrocentricPosition(now);

        const Star* sun = receiver.getSystem()->getStar();
        assert(sun != nullptr);
        double distToSun = posReceiver.norm();
        float appSunRadius = (float) (sun->getRadius() / distToSun);

        Vector3d dir = posCaster - posReceiver;
        double distToCaster = dir.norm() - receiver.getRadius();
        float appOccluderRadius = (float) (caster.getRadius() / distToCaster);

        // The shadow radius is the radius of the occluder plus some additional
        // amount that depends upon the apparent radius of the sun.  For
        // a sun that's distant/small and effectively a point, the shadow
        // radius will be the same as the radius of the occluder.
        float shadowRadius = (1 + appSunRadius / appOccluderRadius) *
            caster.getRadius();

        // Test whether a shadow is cast on the receiver.  We want to know
        // if the receiver lies within the shadow volume of the caster.  Since
        // we're assuming that everything is a sphere and the sun is far
        // away relative to the caster, the shadow volume is a
        // cylinder capped at one end.  Testing for the intersection of a
        // singly capped cylinder is as simple as checking the distance
        // from the center of the receiver to the axis of the shadow cylinder.
        // If the distance is less than the sum of the caster's and receiver's
        // radii, then we have an eclipse.
        float R = receiver.getRadius() + shadowRadius;
        double dist = distance(posReceiver, Ray3d(posCaster, posCaster));
        if (dist < R)
        {
            // Ignore "eclipses" where the caster and receiver have
            // intersecting bounding spheres.
            if (distToCaster > caster.getRadius())
                return true;
        }
    }

    return false;
}

#if 1
double findEclipseSpan(const Body& receiver, const Body& caster,
                       double now, double dt)
{
    double t = now;
    while (testEclipse(receiver, caster, t))
        t += dt;

    return t;
}
#else
// Given a time during an eclipse, find the start of the eclipse to
// a precision of minStep.
double findEclipseStart(const Body& receiver, const Body& occulter,
                        double now,
                        double startStep,
                        double minStep)
{
    double step = startStep / 2;
    double t = now - step;
    bool eclipsed = true;

    // Perform a binary search to find the end of the eclipse
    while (step > minStep)
    {
        eclipsed = testEclipse(receiver, occulter, t);
        step *= 0.5;
        if (eclipsed)
            t -= step;
        else
            t += step;
    }

    // Always return a time when the receiver is /not/ in eclipse
    if (eclipsed)
        t -= step;

    return t;
}

// Given a time during an eclipse, find the end of the eclipse to
// a precision of minStep.
double findEclipseEnd(const Body& receiver, const Body& occulter,
                      double now,
                      double startStep,
                      double minStep)
{
    // First do a coarse search to find the eclipse end to within the precision
    // of startStep.
    while (testEclipse(receiver, occulter, now + startStep))
        now += startStep;

    double step = startStep / 2;
    double t = now + step;
    bool eclipsed = true;

    // Perform a binary search to find the end of the eclipse
    while (step > minStep)
    {
        eclipsed = testEclipse(receiver, occulter, t);
        step *= 0.5;
        if (eclipsed)
            t += step;
        else
            t -= step;
    }

    // Always return a time when the receiver is /not/ in eclipse
    if (eclipsed)
        t += step;

    return t;
}
#endif

void addEclipse(const Body& receiver, const Body& occulter,
                double now,
                double /*startStep*/, double /*minStep*/,
                vector<Eclipse>& eclipses,
                vector<double>& previousEclipseEndTimes, int i)
{
    if (testEclipse(receiver, occulter, now))
    {
        Eclipse eclipse;
#if 1
        eclipse.startTime = findEclipseSpan(receiver, occulter, now, -dT);
        eclipse.endTime = findEclipseSpan(receiver, occulter, now, dT);
#else
        eclipse.startTime = findEclipseStart(receiver, occulter, now, searchStep, minStep);
        eclipse.endTime = findEclipseEnd(receiver, occulter, now, searchStep, minStep);
#endif
        eclipse.receiver = const_cast<Body*>(&receiver);
        eclipse.occulter = const_cast<Body*>(&occulter);
        eclipses.emplace_back(eclipse);

        previousEclipseEndTimes[i] = eclipse.endTime;
    }
}

void EclipseFinder::findEclipses(double startDate,
                                 double endDate,
                                 int eclipseTypeMask,
                                 vector<Eclipse>& eclipses)
{
    PlanetarySystem* satellites = body->getSatellites();

    // See if there's anything that could test
    if (satellites == nullptr)
        return;

    // For each body, we'll need to store the time when the last eclipse ended
    vector<double> previousEclipseEndTimes;

    // Make a list of satellites that we'll actually test for eclipses; ignore
    // spacecraft and very small objects.
    vector<Body*> testBodies;
    for (int i = 0; i < satellites->getSystemSize(); i++)
    {
        Body* obj = satellites->getBody(i);
        if ((obj->getClassification() & EclipseObjectMask) != 0 &&
            obj->getRadius() >= body->getRadius() * MinRelativeOccluderRadius)
        {
            testBodies.push_back(obj);
            previousEclipseEndTimes.push_back(startDate - 1.0);
        }
    }

    if (testBodies.empty())
        return;

    // TODO: Use a fixed step of one hour for now; we should use a binary
    // search instead.
    double searchStep = 1.0 / 24.0; // one hour

    // Precision of eclipse duration calculation
    double durationPrecision = 1.0 / (24.0 * 360.0); // ten seconds

    for (double t = startDate; t <= endDate; t += searchStep)
    {
        if (watcher != nullptr)
        {
            if (watcher->eclipseFinderProgressUpdate(t) == EclipseFinderWatcher::AbortOperation)
                return;
        }

        for (unsigned int i = 0; i < testBodies.size(); i++)
        {
            // Only test for an eclipse if we're not in the middle of
            // of previous one.
            if (t <= previousEclipseEndTimes[i])
                continue;

            if (eclipseTypeMask & Eclipse::Solar)
                addEclipse(*body, *testBodies[i], t, searchStep, durationPrecision, eclipses, previousEclipseEndTimes, i);

            if (eclipseTypeMask & Eclipse::Lunar)
                addEclipse(*testBodies[i], *body, t, searchStep, durationPrecision, eclipses, previousEclipseEndTimes, i);
        }
    }
}
