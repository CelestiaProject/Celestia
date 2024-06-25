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
#include <string_view>
#include <system_error>
#include <utility>

#include <celastro/astro.h>
#include <celcompat/charconv.h>

using namespace std::string_view_literals;
namespace astro = celestia::astro;

namespace
{

constexpr std::string_view Whitespace = " \t"sv;

constexpr float BoundariesDrawDistance = 10000.0f;

void
trimLeadingWhitespace(std::string_view& str)
{
    if (auto pos = str.find_first_not_of(Whitespace); pos == std::string_view::npos)
        str = {};
    else
        str = str.substr(pos);
}

bool
readFloat(std::string_view& line, float& value)
{
    using celestia::compat::from_chars;

    trimLeadingWhitespace(line);
    if (line.empty())
        return false;

    // charconv cannot handle leading + sign
    if (line.front() == '+')
        line = line.substr(1);

    auto [ptr, ec] = from_chars(line.data(), line.data() + line.size(), value);
    if (ec != std::errc{})
        return false;

    line = line.substr(static_cast<std::size_t>(ptr - line.data()));
    return true;
}

} // end unnamed namespace

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
    // Format of the boundaries.dat file (text):
    // * Right ascension (floating point)
    // * Declination (floating point, + sign used for positive)
    // * Constellation (3 characters)
    // * Additional field of 1 character, unused
    // Fields are separated by whitespace, line may include leading space
    // Comment lines start with #

    std::vector<ConstellationBoundaries::Chain> chains;
    ConstellationBoundaries::Chain currentChain;

    std::string buffer(std::size_t(1024), '\0');
    std::array<char, 3> lastConstellation{ '\0', '\0', '\0' };
    while (!in.eof())
    {
        in.getline(buffer.data(), buffer.size());
        std::size_t lineLength;
        // delimiter is extracted and contributes to gcount() but is not written to buffer
        if (in.good())
            lineLength = static_cast<std::size_t>(in.gcount() - 1);
        else if (in.eof())
            lineLength = static_cast<std::size_t>(in.gcount());
        else
            break;

        auto line = static_cast<std::string_view>(buffer).substr(0, lineLength);
        if (line.empty() || line.front() == '#')
            continue;

        float ra;
        if (!readFloat(line, ra))
            break;
        float dec;
        if (!readFloat(line, dec))
            break;

        trimLeadingWhitespace(line);
        auto pos = line.find_first_of(Whitespace);
        auto constellation = line.substr(0, pos);
        if (constellation.empty() || constellation == "XXX"sv)
            break;
        if (constellation != std::string_view(lastConstellation.data(), lastConstellation.size()))
        {
            if (currentChain.size() > 1)
                chains.emplace_back(std::move(currentChain));

            constellation.copy(lastConstellation.data(), lastConstellation.size());
            currentChain.clear();
        }

        currentChain.emplace_back(astro::equatorialToCelestialCart(ra, dec, BoundariesDrawDistance));
    }

    if (currentChain.size() > 1)
        chains.emplace_back(std::move(currentChain));

    return std::make_unique<ConstellationBoundaries>(std::move(chains));
}
