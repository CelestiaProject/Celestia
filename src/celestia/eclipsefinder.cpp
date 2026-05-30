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

#include "eclipsefinder.h"

#include <cassert>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <celengine/body.h>
#include <celengine/frametree.h>
#include <celengine/star.h>
#include <celengine/timeline.h>
#include <celengine/timelinephase.h>
#include <celmath/distance.h>

namespace math = celestia::math;
namespace util = celestia::util;

namespace
{

constexpr double dT = 1.0 / (24.0 * 60.0);
constexpr auto EclipseObjectMask = BodyClassification::Planet      |
                                   BodyClassification::Moon        |
                                   BodyClassification::MinorMoon   |
                                   BodyClassification::DwarfPlanet |
                                   BodyClassification::Asteroid;

// TODO: share this constant and function with render.cpp
constexpr float MinRelativeOccluderRadius = 0.005f;

bool
testEclipse(const Body& receiver, const Body& caster, double now)
{
    // Ignore situations where the shadow casting body is much smaller than
    // the receiver, as these shadows aren't likely to be relevant.  Also,
    // ignore eclipses where the caster is not an ellipsoid, since we can't
    // generate correct shadows in this case.
    if (caster.getRadius() < receiver.getRadius() * MinRelativeOccluderRadius ||
        !caster.isEllipsoid())
    {
        return false;
    }

    // All of the eclipse related code assumes that both the caster
    // and receiver are spherical.  Irregular receivers will work more
    // or less correctly, but casters that are sufficiently non-spherical
    // will produce obviously incorrect shadows.  Another assumption we
    // make is that the distance between the caster and receiver is much
    // less than the distance between the sun and the receiver.  This
    // approximation works everywhere in the solar system, and likely
    // works for any orbitally stable pair of objects orbiting a star.
    Eigen::Vector3d posReceiver = receiver.getAstrocentricPosition(now);
    Eigen::Vector3d posCaster = caster.getAstrocentricPosition(now);

    const Star* sun = receiver.getTimeline()->findPhase(now).getFrameTree()->getRoot(now);
    if (!sun)
        return false;

    double distToSun = posReceiver.norm();
    float appSunRadius = (float) (sun->getRadius() / distToSun);

    Eigen::Vector3d dir = posCaster - posReceiver;
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
    double dist = math::distance(posReceiver, Eigen::ParametrizedLine<double, 3>(posCaster, posCaster));

    // Ignore "eclipses" where the caster and receiver have
    // intersecting bounding spheres.
    return dist < R && distToCaster > caster.getRadius();
}

double
findEclipseSpan(const Body& receiver, const Body& caster,
                double now, double dt)
{
    double t = now;
    while (testEclipse(receiver, caster, t))
        t += dt;

    return t;
}

void
addEclipse(const Body& receiver, const Body& occulter,
           double now,
           double /*startStep*/, double /*minStep*/,
           std::vector<Eclipse>& eclipses,
           std::vector<double>& previousEclipseEndTimes, int i)
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

struct BodyInfo
{
    BodyInfo(const Body* _body, double _startTime, double _endTime) :
        body(_body), startTime(_startTime), endTime(_endTime)
    {}

    const Body* body;
    double startTime;
    double endTime;
};

std::vector<BodyInfo>
getTestBodies(const Body* body, double startDate, double endDate)
{
    std::vector<BodyInfo> testBodies;
    const FrameTree* frameTree = body->getFrameTree();
    if (!frameTree)
        return testBodies;

    const float minRadius = body->getRadius() * MinRelativeOccluderRadius;
    // For each body, store the start and end time of the unprocessed window
    for (unsigned int i = 0, nPhases = frameTree->childCount(); i < nPhases; ++i)
    {
        const auto* phase = frameTree->getChild(i);
        if (phase->startTime() > endDate || phase->endTime() <= startDate)
            continue;

        double phaseStart = std::max(startDate, phase->startTime());
        double phaseEnd = std::min(endDate, phase->endTime());

        if (phaseEnd <= phaseStart)
            continue;

        const Body* testBody = phase->body();
        if (!util::is_set(testBody->getClassification(), EclipseObjectMask) ||
            testBody->getRadius() < minRadius)
        {
            continue;
        }

        auto it = std::find_if(testBodies.begin(), testBodies.end(),
                               [testBody](const BodyInfo& info) { return info.body == testBody; });
        if (it == testBodies.end())
        {
            testBodies.emplace_back(testBody, phaseStart, phaseEnd);
        }
        else
        {
            it->startTime = std::min(it->startTime, phaseStart);
            it->endTime = std::max(it->endTime, phaseEnd);
        }
    }

    return testBodies;
}

} // end unnamed namespace

EclipseFinder::EclipseFinder(const Body* _body,
                             EclipseFinderWatcher* _watcher) :
    body(_body),
    watcher(_watcher)
{
}

void EclipseFinder::findEclipses(double startDate,
                                 double endDate,
                                 Eclipse::Type eclipseTypeMask,
                                 std::vector<Eclipse>& eclipses)
{
    std::vector<BodyInfo> testBodies = getTestBodies(body, startDate, endDate);
    if (testBodies.empty())
        return;

    std::vector<double> previousEclipseEndTimes(testBodies.size());
    std::transform(testBodies.cbegin(), testBodies.cend(), previousEclipseEndTimes.begin(),
                   [](const BodyInfo& info) { return info.startTime - 1.0; });

    // TODO: Use a fixed step of one hour for now; we should use a binary
    // search instead.
    constexpr double searchStep = 1.0 / 24.0; // one hour

    // Precision of eclipse duration calculation
    constexpr double durationPrecision = 1.0 / (24.0 * 360.0); // ten seconds

    for (double t = startDate; t <= endDate; t += searchStep)
    {
        if (watcher && watcher->eclipseFinderProgressUpdate(t) == EclipseFinderWatcher::Status::AbortOperation)
            return;

        for (unsigned int i = 0; i < testBodies.size(); ++i)
        {
            const BodyInfo& info = testBodies[i];

            // Only test for an eclipse if we're not in the middle of
            // of previous one.
            if (t < info.startTime || t >= info.endTime || t <= previousEclipseEndTimes[i])
                continue;

            if (util::is_set(eclipseTypeMask, Eclipse::Type::Solar))
                addEclipse(*body, *info.body, t, searchStep, durationPrecision, eclipses, previousEclipseEndTimes, i);

            if (util::is_set(eclipseTypeMask, Eclipse::Type::Lunar))
                addEclipse(*info.body, *body, t, searchStep, durationPrecision, eclipses, previousEclipseEndTimes, i);
        }
    }
}
