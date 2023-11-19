// boundaries.cpp
//
// Copyright (C) 2002-2019, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "boundaries.h"

#include <istream>
#include <string>
#include <utility>

#include <celastro/astro.h>

namespace astro = celestia::astro;

namespace
{

constexpr float BoundariesDrawDistance = 10000.0f;

}


ConstellationBoundaries::ConstellationBoundaries(std::vector<Chain>&& _chains) :
    chains(std::move(_chains))
{
}


const std::vector<ConstellationBoundaries::Chain>&
ConstellationBoundaries::getChains() const
{
    return chains;
}


std::unique_ptr<ConstellationBoundaries>
ReadBoundaries(std::istream& in)
{
    std::vector<ConstellationBoundaries::Chain> chains;
    ConstellationBoundaries::Chain currentChain;

    std::string pt;
    std::string con;
    std::string lastCon;

    for (;;)
    {
        float ra = 0.0f;
        float dec = 0.0f;
        in >> ra;
        if (!in.good())
            break;
        in >> dec;

        con.clear();
        pt.clear();

        in >> con;
        in >> pt;
        if (!in.good())
            break;

        if (con != lastCon)
        {
            if (currentChain.size() > 1)
                chains.emplace_back(std::move(currentChain));

            lastCon = con;
            currentChain.clear();
        }

        currentChain.emplace_back(astro::equatorialToCelestialCart(ra, dec,
                                                                   BoundariesDrawDistance));
    }

    if (currentChain.size() > 1)
        chains.emplace_back(std::move(currentChain));

    return std::make_unique<ConstellationBoundaries>(std::move(chains));
}
