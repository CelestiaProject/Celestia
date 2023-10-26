// sampfile.h
//
// Utility functions for sampled orbit and rotation files.
//
// Copyright (C) 2023, Celestia Development Team
//
// Extracted from samporbit.h/samporient.h
// Copyright (C) 2008, Celestia Development Team
// Initial implementation by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>
#include <fstream>
#include <istream>
#include <limits>
#include <utility>
#include <vector>

#include <celcompat/filesystem.h>
#include <celutil/array_view.h>

namespace celestia::ephem
{

namespace detail
{

bool checkSampleOrdering(double tdb,
                         double& lastSampleTime,
                         bool& hasOutOfOrderSamples,
                         const fs::path& filename);

bool logIfNoSamples(bool, const fs::path&);
void logReadError(const fs::path&);
void logOpenAsciiFail(const fs::path&);
void logSkipCommentsFail(const fs::path&);

bool skipComments(std::istream& in);

}


std::uint32_t GetSampleIndex(double jd,
                             std::uint32_t& lastSample,
                             celestia::util::array_view<double> sampleTimes);


template<typename T, typename F>
bool
LoadSamples(std::istream& in,
            const fs::path& filename,
            std::vector<double>& sampleTimes,
            std::vector<T>& samples,
            F readSample)
{
    double lastSampleTime = -std::numeric_limits<double>::infinity();
    bool hasOutOfOrderSamples = false;
    for (;;)
    {
        double tdb;
        T sample;
        if (!readSample(in, tdb, sample))
        {
            if (in.eof())
                return detail::logIfNoSamples(!sampleTimes.empty(), filename);

            detail::logReadError(filename);
            return false;
        }

        // Skip samples with duplicate or out-of-order times; such trajectories
        // are invalid, but are unfortunately used in some existing add-ons.
        if (!detail::checkSampleOrdering(tdb, lastSampleTime, hasOutOfOrderSamples, filename))
            continue;

        sampleTimes.push_back(tdb);
        samples.push_back(std::move(sample));
    }
}


template<typename T, typename F>
bool
LoadAsciiSamples(const fs::path& filename,
                 std::vector<double>& sampleTimes,
                 std::vector<T>& samples,
                 F readSample)
{
    std::ifstream in(filename);
    if (!in.good())
    {
        detail::logOpenAsciiFail(filename);
        return false;
    }

    if (!detail::skipComments(in))
    {
        detail::logSkipCommentsFail(filename);
        return false;
    }

    return LoadSamples(in, filename, sampleTimes, samples, readSample);
}

} // end namespace celestia::ephem
