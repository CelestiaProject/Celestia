// trajmanager.cpp
//
// Copyright (C) 2001-2008 Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <fstream>
#include <cassert>
#include <fmt/format.h>
#include <celephem/samporbit.h>
#include <celutil/logger.h>
#include <celutil/filetype.h>
#include "trajmanager.h"

using namespace std;
using celestia::ephem::TrajectoryInterpolation;
using celestia::ephem::TrajectoryPrecision;
using celestia::util::GetLogger;

static TrajectoryManager* trajectoryManager = nullptr;

constexpr const char UniqueSuffixChar = '!';


TrajectoryManager* GetTrajectoryManager()
{
    if (trajectoryManager == nullptr)
        trajectoryManager = new TrajectoryManager("data");
    return trajectoryManager;
}


fs::path TrajectoryInfo::resolve(const fs::path& baseDir)
{
    // Ensure that trajectories with different interpolation or precision get resolved to different objects by
    // adding a 'uniquifying' suffix to the filename that encodes the properties other than filename which can
    // distinguish two trajectories. This suffix is stripped before the file is actually loaded.
    auto uniquifyingSuffix = fmt::format("{}{}{}", UniqueSuffixChar, (unsigned int) interpolation, (unsigned int) precision);

    if (!path.empty())
    {
        fs::path filename = path / "data" / source;
        ifstream in(filename);
        if (in.good())
            return filename += uniquifyingSuffix;
    }

    return (baseDir / source) += uniquifyingSuffix;
}

celestia::ephem::Orbit* TrajectoryInfo::load(const fs::path& filename)
{
    // strip off the uniquifying suffix
    auto uniquifyingSuffixStart = filename.string().rfind(UniqueSuffixChar);
    fs::path strippedFilename = filename.string().substr(0, uniquifyingSuffixStart);
    ContentType filetype = DetermineFileType(strippedFilename);

    GetLogger()->debug("Loading trajectory: {}\n", strippedFilename);

    // TODO use unique_ptr here and replace the use of .release()
    celestia::ephem::Orbit* sampTrajectory = nullptr;

    if (filetype == Content_CelestiaXYZVTrajectory)
    {
        switch (precision)
        {
        case TrajectoryPrecision::Single:
            sampTrajectory = LoadXYZVTrajectorySinglePrec(strippedFilename, interpolation).release();
            break;
        case TrajectoryPrecision::Double:
            sampTrajectory = LoadXYZVTrajectoryDoublePrec(strippedFilename, interpolation).release();
            break;
        default:
            assert(0);
            break;
        }
    }
    else if (filetype == Content_CelestiaXYZVBinary)
    {
        switch (precision)
        {
        case TrajectoryPrecision::Single:
            sampTrajectory = LoadXYZVBinarySinglePrec(strippedFilename, interpolation).release();
            break;
        case TrajectoryPrecision::Double:
            sampTrajectory = LoadXYZVBinaryDoublePrec(strippedFilename, interpolation).release();
            break;
        default:
            assert(0);
            break;
        }
    }
    else
    {
        switch (precision)
        {
        case TrajectoryPrecision::Single:
            sampTrajectory = LoadSampledTrajectorySinglePrec(strippedFilename, interpolation).release();
            break;
        case TrajectoryPrecision::Double:
            sampTrajectory = LoadSampledTrajectoryDoublePrec(strippedFilename, interpolation).release();
            break;
        default:
            assert(0);
            break;
        }
    }

    return sampTrajectory;
}
