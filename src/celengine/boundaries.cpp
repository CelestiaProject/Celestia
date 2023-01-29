// boundaries.cpp
//
// Copyright (C) 2002-2019, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include <istream>

#include "astro.h"
#include "boundaries.h"

constexpr const float BoundariesDrawDistance = 10000.0f;

ConstellationBoundaries::ConstellationBoundaries()
{
    currentChain = new Chain();
    currentChain->emplace_back(Eigen::Vector3f::Zero());
}

ConstellationBoundaries::~ConstellationBoundaries()
{
    for (const auto chain : chains)
        delete chain;
    chains.clear();

    delete currentChain;
}


const std::vector<ConstellationBoundaries::Chain*>&
ConstellationBoundaries::getChains() const
{
    return chains;
}


void ConstellationBoundaries::moveto(float ra, float dec)
{
    assert(currentChain != nullptr);

    Eigen::Vector3f v = astro::equatorialToCelestialCart(ra, dec, BoundariesDrawDistance);
    if (currentChain->size() > 1)
    {
        chains.emplace_back(currentChain);
        currentChain = new Chain();
        currentChain->emplace_back(v);
    }
    else
    {
        (*currentChain)[0] = v;
    }
}


void ConstellationBoundaries::lineto(float ra, float dec)
{
    currentChain->emplace_back(astro::equatorialToCelestialCart(ra, dec, BoundariesDrawDistance));
}


ConstellationBoundaries* ReadBoundaries(std::istream& in)
{
    auto* boundaries = new ConstellationBoundaries();
    std::string lastCon;
    int conCount = 0;
    int ptCount = 0;

    for (;;)
    {
        float ra = 0.0f;
        float dec = 0.0f;
        in >> ra;
        if (!in.good())
            break;
        in >> dec;

        std::string pt;
        std::string con;

        in >> con;
        in >> pt;
        if (!in.good())
            break;

        if (con != lastCon)
        {
            boundaries->moveto(ra, dec);
            lastCon = con;
            conCount++;
        }
        else
        {
            boundaries->lineto(ra, dec);
        }
        ptCount++;
    }

    return boundaries;
}
