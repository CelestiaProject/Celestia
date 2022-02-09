// starbrowser.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Star browser tool for Celestia.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <string>
#include <algorithm>
#include <set>
#include "starbrowser.h"

using namespace Eigen;
using namespace std;

struct CloserStarPredicate
{
    Vector3f pos;
    bool operator()(const Star* star0, const Star* star1) const
    {
        return (pos - star0->getPosition()).squaredNorm() < (pos - star1->getPosition()).squaredNorm();
    }
};

struct BrighterStarPredicate
{
    Vector3f pos;
    UniversalCoord ucPos;
    bool operator()(const Star* star0, const Star* star1) const
    {
        float d0 = (pos - star0->getPosition()).norm();
        float d1 = (pos - star1->getPosition()).norm();

        // If the stars are closer than one light year, use
        // a more precise distance estimate.
        if (d0 < 1.0f)
            d0 = ucPos.offsetFromLy(star0->getPosition()).norm();
        if (d1 < 1.0f)
            d1 = ucPos.offsetFromLy(star1->getPosition()).norm();

        return star0->getApparentMagnitude(d0) < star1->getApparentMagnitude(d1);
    }
};

struct BrightestStarPredicate
{
    bool operator()(const Star* star0, const Star* star1) const
    {
        return star0->getAbsoluteMagnitude() < star1->getAbsoluteMagnitude();
    }
};

struct SolarSystemPredicate
{
    Vector3f pos;
    SolarSystemCatalog* solarSystems;

    bool operator()(const Star* star0, const Star* star1) const
    {
        SolarSystemCatalog::iterator iter;

        iter = solarSystems->find(star0->getIndex());
        bool hasPlanets0 = (iter != solarSystems->end());
        iter = solarSystems->find(star1->getIndex());
        bool hasPlanets1 = (iter != solarSystems->end());
        if (hasPlanets1 == hasPlanets0)
        {
            return ((pos - star0->getPosition()).squaredNorm() < (pos - star1->getPosition()).squaredNorm());
        }
        else
        {
            return hasPlanets0;
        }
    }
};


// Find the nearest/brightest/X-est N stars in a database.  The
// supplied predicate determines which of two stars is a better match.
template<class Pred> static std::vector<const Star*>*
findStars(const StarDatabase& stardb, Pred pred, int nStars)
{
    std::vector<const Star*>* finalStars = new std::vector<const Star*>();
    if (nStars == 0)
        return finalStars;
    if(nStars > 500)
        nStars = 500;

    typedef std::multiset<const Star*, Pred> StarSet;
    StarSet firstStars(pred);

    int totalStars = stardb.size();
    if (totalStars < nStars)
        nStars = totalStars;

    // We'll need at least nStars in the set, so first fill
    // up the list indiscriminately.
    int i = 0;
    for (i = 0; i < nStars; i++)
        firstStars.insert(stardb.getStar(i));

    // From here on, only add a star to the set if it's
    // a better match than the worst matching star already
    // in the set.
    const Star* lastStar = *--firstStars.end();
    for (; i < totalStars; i++)
    {
        Star* star = stardb.getStar(i);
        if (pred(star, lastStar))
        {
            firstStars.insert(star);
            firstStars.erase(--firstStars.end());
            lastStar = *--firstStars.end();
        }
    }

    // Move the best matching stars into the vector
    finalStars->reserve(nStars);
    for (const auto& star : firstStars)
        finalStars->push_back(star);

    return finalStars;
}


const Star* StarBrowser::nearestStar()
{
    Universe* univ = appSim->getUniverse();
    CloserStarPredicate closerPred;
    closerPred.pos = pos;
    std::vector<const Star*>* stars = findStars(*(univ->getStarCatalog()), closerPred, 1);
    const Star *star = (*stars)[0];
    delete stars;
    return star;
}


std::vector<const Star*>*
StarBrowser::listStars(unsigned int nStars)
{
    Universe* univ = appSim->getUniverse();
    switch(predicate)
    {
    case BrighterStars:
        {
            BrighterStarPredicate brighterPred;
            brighterPred.pos = pos;
            brighterPred.ucPos = ucPos;
            return findStars(*(univ->getStarCatalog()), brighterPred, nStars);
        }
        break;

    case BrightestStars:
        {
            BrightestStarPredicate brightestPred;
            return findStars(*(univ->getStarCatalog()), brightestPred, nStars);
        }
        break;

    case StarsWithPlanets:
        {
            SolarSystemCatalog* solarSystems = univ->getSolarSystemCatalog();
            if (!solarSystems)
                return nullptr;
            SolarSystemPredicate solarSysPred;
            solarSysPred.pos = pos;
            solarSysPred.solarSystems = solarSystems;
            return findStars(*(univ->getStarCatalog()), solarSysPred,
                             min((size_t) nStars, solarSystems->size()));
        }
        break;

    case NearestStars:
    default:
        {
            CloserStarPredicate closerPred;
            closerPred.pos = pos;
            return findStars(*(univ->getStarCatalog()), closerPred, nStars);
        }
        break;
    }

    return nullptr;  // keep compiler happy
}


bool StarBrowser::setPredicate(int pred)
{
    if ((pred < NearestStars) || (pred > StarsWithPlanets))
        return false;
    predicate = pred;
    return true;
}


void StarBrowser::refresh()
{
    ucPos = appSim->getObserver().getPosition();
    pos = ucPos.toLy().cast<float>();
}


void StarBrowser::setSimulation(Simulation *_appSim)
{
    appSim = _appSim;
    refresh();
}


StarBrowser::StarBrowser(Simulation* _appSim, int pred) :
    appSim(_appSim)
{
    ucPos = appSim->getObserver().getPosition();
    pos = ucPos.toLy().cast<float>();

    predicate = pred;
}


StarBrowser::StarBrowser() :
    pos(Vector3f::Zero()),
    ucPos(UniversalCoord::Zero()),
    appSim(nullptr),
    predicate(NearestStars)
{
}
