// sampfile.cpp
//
// Utility functions for sampled orbit and rotation files.
//
// Copyright (C) 2023, Celestia Development Team
//
// Extracted from samporbit.cpp/samporient.cpp
// Copyright (C) 2008, Celestia Development Team
// Initial implementation by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "sampfile.h"

#include <algorithm>
#include <cctype>

#include <celutil/gettext.h>
#include <celutil/logger.h>

using celestia::util::GetLogger;

namespace celestia::ephem
{

namespace detail
{

bool
checkSampleOrdering(double tdb,
                    double& lastSampleTime,
                    bool& hasOutOfOrderSamples,
                    const fs::path& filename)
{
    if (tdb > lastSampleTime)
    {
        lastSampleTime = tdb;
        return true;
    }

    if (!hasOutOfOrderSamples)
    {
        GetLogger()->warn(_("Skipping out-of-order samples in {}.\n"), filename);
        hasOutOfOrderSamples = true;
    }

    return false;
}


bool
logIfNoSamples(bool hasSamples, const fs::path& filename)
{
    if (!hasSamples)
        GetLogger()->error(_("No samples found in sample file {}.\n"), filename);

    return hasSamples;
}


void
logReadError(const fs::path& filename)
{
    GetLogger()->error(_("Error reading sample file {}.\n"), filename);
}


void
logOpenAsciiFail(const fs::path& filename)
{
    GetLogger()->error(_("Error opening ASCII sample file {}.\n"), filename);
}


void
logSkipCommentsFail(const fs::path& filename)
{
    GetLogger()->error(_("Error finding data in ASCII sample file {}.\n"), filename);
}


// Scan past comments. A comment begins with the # character and ends
// with a newline. Return true if the stream state is good. The stream
// position will be at the first non-comment, non-whitespace character.
bool skipComments(std::istream& in)
{
    bool inComment = false;
    for (;;)
    {
        int c = in.get();
        if (!in.good())
            return false;

        if (inComment)
        {
            if (c == '\n')
                inComment = false;
        }
        else
        {
            if (c == '#')
            {
                inComment = true;
            }
            else if (!std::isspace(static_cast<unsigned char>(c)))
            {
                in.unget();
                return in.good();
            }
        }
    }
}

} // end namespace celestia::ephem::detail

// Do a binary search to find the samples that define the orientation
// at the current time. Cache the previous sample used and avoid
// the search if it covers the requested time.
std::uint32_t
GetSampleIndex(double jd,
               std::uint32_t& lastSample,
               celestia::util::array_view<double> sampleTimes)
{
    std::uint32_t n = lastSample;
    if (n < 1 || n >= sampleTimes.size() || jd < sampleTimes[n - 1] || jd > sampleTimes[n])
    {
        auto iter = std::lower_bound(sampleTimes.begin(), sampleTimes.end(), jd);
        n = static_cast<std::uint32_t>(iter - sampleTimes.begin());
        lastSample = n;
    }

    return n;
}

} // end namespace celestia::ephem
