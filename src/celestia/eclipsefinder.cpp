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
#include <sstream>
#include <algorithm>
#include <set>
#include <cassert>
#include "eclipsefinder.h"
#include "celmath/mathlib.h"
#include "celmath/ray.h"
#include "celmath/distance.h"
#include <celengine/eigenport.h>

using namespace Eigen;
using namespace std;




Eclipse::Eclipse(int Y, int M, int D) :
    body(NULL)
{
    date = new astro::Date(Y, M, D);
}

Eclipse::Eclipse(double JD) :
    body(NULL)
{
    date = new astro::Date(JD);
}


// TODO: share this constant and function with render.cpp
static const float MinRelativeOccluderRadius = 0.005f;

bool EclipseFinder::testEclipse(const Body& receiver, const Body& caster,
                                double now) const
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
        assert(sun != NULL);
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


double EclipseFinder::findEclipseSpan(const Body& receiver, const Body& caster,
                       double now, double dt) const
{
    double t = now;
    while (testEclipse(receiver, caster, t))
        t += dt;

    return t;
}


int EclipseFinder::CalculateEclipses()
{
    Simulation* sim = appCore->getSimulation();
    
    Eclipse* eclipse;
    double* JDback = NULL;
    
    int nIDplanetetofindon = 0;
    int nSattelites = 0;

    const SolarSystem* sys = sim->getNearestSolarSystem();
    
    toProcess = false;
    
    if ((!sys))
    {
        eclipse = new Eclipse(0.);
        eclipse->planete = "None";
        Eclipses_.insert(Eclipses_.end(), *eclipse);
        delete eclipse;
        return 1;
    }
    else if (sys->getStar()->getCatalogNumber() != 0)
    {
        eclipse = new Eclipse(0.);
        eclipse->planete = "None";
        Eclipses_.insert(Eclipses_.end(), *eclipse);
        delete eclipse;
        return 1;
    }

    PlanetarySystem* system = sys->getPlanets();
    int nbPlanets = system->getSystemSize();

    for (int i = 0; i < nbPlanets; ++i)
    {
        Body* planete = system->getBody(i);
        if (planete != NULL)
            if (strPlaneteToFindOn == planete->getName())
            {
                nIDplanetetofindon = i;
                PlanetarySystem* satellites = planete->getSatellites();
                if (satellites)
                {
                    nSattelites = satellites->getSystemSize();
                    JDback = new double[nSattelites];
                    memset(JDback, 0, nSattelites*sizeof(double));
                }
                break;
            }
    }

    Body* planete = system->getBody(nIDplanetetofindon);
    while (JDfrom < JDto)
    {
        PlanetarySystem* satellites = planete->getSatellites();
        if (satellites)
        {
            for (int j = 0; j < nSattelites; ++j)
            {
                Body* caster = NULL;
                Body* receiver = NULL;
                bool test = false;

                if (satellites->getBody(j)->getClassification() != Body::Spacecraft)
                {
                    if (type == Eclipse::Solar)
                    {
                        caster = satellites->getBody(j);
                        receiver = planete;
                    }
                    else
                    {
                        caster = planete;
                        receiver = satellites->getBody(j);
                    }

                    test = testEclipse(*receiver, *caster, JDfrom);
                }

                if (test && JDfrom - JDback[j] > 1)
                {
                    JDback[j] = JDfrom;
                    eclipse = new Eclipse(JDfrom);
                    eclipse->startTime = findEclipseSpan(*receiver, *caster,
                                                         JDfrom,
                                                         -1.0 / (24.0 * 60.0));
                    eclipse->endTime   = findEclipseSpan(*receiver, *caster,
                                                         JDfrom,
                                                         1.0 / (24.0 * 60.0));
                    eclipse->body = receiver;
                    eclipse->planete = planete->getName();
                    eclipse->sattelite = satellites->getBody(j)->getName();
                    Eclipses_.insert(Eclipses_.end(), *eclipse);
                    delete eclipse;
                }
            }
        }
        JDfrom += 1.0 / 24.0;
    }
    if (JDback)
        delete JDback;
    if (Eclipses_.empty())
    {
        eclipse = new Eclipse(0.);
        eclipse->planete = "None";
        Eclipses_.insert(Eclipses_.end(), *eclipse);
        delete eclipse;
    }
    return 0;
}

