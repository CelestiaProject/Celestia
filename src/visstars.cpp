// visstars.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Keep track of the subset of stars within a database that are visible
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cmath>
#include "celestia.h"
#include "astro.h"
#include "visstars.h"
#include <stdio.h>

// This class walks through the stars in a database to determine which are
// visible to an observer.  At worst, we'll go through the entire database
// every time update is called, but if the position of the observer is
// changing slowly enough, this isn't necessary.  Instead, we walk through
// only a portion of the database with each update.  We flag not only stars
// that are visible, but also stars that might become visible shortly; this
// way, we can limit the effect of the lag due to only checking part of the
// database per update.  Two checks are applied to find stars that might
// be visible soon--we look for stars slightly below the limiting magnitude,
// and look for very close stars even if they're well below the limiting
// magnitude.  The second check is helpful because very faint stars are
// visible over a much smaller range than bright star, thus a fast-moving
// observer is more likely to miss a faint star.


VisibleStarSet::VisibleStarSet(StarDatabase* db) :
    starDB(db),
    faintest(6.0f),   // average limit of human vision under ideal conditions
    closeDistance(0),
    firstIndex(0)
{
    current = new vector<uint32>;
    last = new vector<uint32>;
    currentNear = new vector<uint32>;
    lastNear = new vector<uint32>;
}


VisibleStarSet::~VisibleStarSet()
{
    delete current;
    delete last;
    delete currentNear;
    delete lastNear;
}


// Search through some fraction of the star database and determine
// visibility.  By default, search through the entire database.
void VisibleStarSet::update(const Observer& obs, float fraction)
{
    uint32 nStars = starDB->size();
    uint32 nUpdate = (uint32) (fraction * nStars);
    bool willFinish = firstIndex + nUpdate >= nStars;
    uint32 endIndex = willFinish ? nStars : firstIndex + nUpdate;

    float closeDistance2 = closeDistance * closeDistance;
    Point3d obsPositionD = obs.getPosition();
    Point3f obsPosition((float) obsPositionD.x,
                        (float) obsPositionD.y,
                        (float) obsPositionD.z);

    // Compute the irradiance from a star with the minimum apparent
    // magnitude.  By definition, the apparent magnitude of a star
    // viewed from a distance of 10pc is the same as it's absolute
    // magnitude, so this is the distance for which we calculate
    // irradiance.  We omit the factor of 4*pi since it doesn't affect
    // comparison of irradiances, which is all we're interested in.
    float threshholdLum = astro::absMagToLum(faintest + 0.5f);
    float threshholdIrradiance = threshholdLum /
        (astro::parsecsToLightYears(10) * astro::parsecsToLightYears(10));
    for (uint32 i = firstIndex; i < endIndex; i++)
    {
        Star* star = starDB->getStar(i);
        float dist2 = obsPosition.distanceToSquared(star->getPosition());

        // Divisions are costly, so instead of comparing lum/dist2 to
        // the threshhold irradiance, we compare lum to threshhold*dist2
        if (dist2 < closeDistance2)
        {
            currentNear->insert(currentNear->end(), i);
            current->insert(current->end(), i);
        }
        else if (star->getLuminosity() >= threshholdIrradiance * dist2)
        {
            current->insert(current->end(), i);
        }
    }

    if (willFinish)
    {
        firstIndex = 0;
        swap(current, last);
        current->clear();
        swap(currentNear, lastNear);
        currentNear->clear();
    }
    else
    {
        firstIndex = endIndex;
    }
}


// Search the entire star database for visible stars.  This resets
// the results of any previous calls to update.  This method is
// appropriate to call to initialize the visible star set or when
// the observer has 'teleported' to a new location.
void VisibleStarSet::updateAll(const Observer& obs)
{
    firstIndex = 0;
    current->clear();
    currentNear->clear();
    update(obs, 1);
}


// Get a list of stars currently visible; this list is only valid until the
// next call to update()
vector<uint32>* VisibleStarSet::getVisibleSet() const
{
    return last;
}


// Get a list of stars within closeDistance; this list is only valid until the
// next call to update()
vector<uint32>* VisibleStarSet::getCloseSet() const
{
    return lastNear;
}


void VisibleStarSet::setLimitingMagnitude(float mag)
{
    faintest = mag;
}


void VisibleStarSet::setCloseDistance(float distance)
{
    closeDistance = distance;
}
