// trajmanager.cpp
//
// Copyright (C) 2001-2008 Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <config.h>
#include "trajmanager.h"
#include <celephem/samporbit.h>
#include <celutil/debug.h>
#include <celutil/filetype.h>
#include <iostream>
#include <fstream>
#include <cassert>
#include <celutil/debug.h>

using namespace std;


static TrajectoryManager* trajectoryManager = nullptr;

constexpr const fs::path::value_type UniqueSuffixChar = '!';


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
    fs::path::string_type uniquifyingSuffix, format;
#ifdef _WIN32
    format = L"%c%u%u";
#else
    format = "%c%u%u";
#endif
    uniquifyingSuffix = fmt::sprintf(format, UniqueSuffixChar, (unsigned int) interpolation, (unsigned int) precision);

    if (!path.empty())
    {
        fs::path filename = path / "data" / source;
        ifstream in(filename.string());
        if (in.good())
            return filename += uniquifyingSuffix;
    }

    return (baseDir / source) += uniquifyingSuffix;
}

Orbit* TrajectoryInfo::load(const fs::path& filename)
{
    // strip off the uniquifying suffix
    string::size_type uniquifyingSuffixStart = filename.string().rfind(UniqueSuffixChar);
    fs::path strippedFilename = filename.string().substr(0, uniquifyingSuffixStart);
    ContentType filetype = DetermineFileType(strippedFilename);

    DPRINTF(LOG_LEVEL_INFO, "Loading trajectory: %s\n", strippedFilename);

    Orbit* sampTrajectory = nullptr;

    if (filetype == Content_CelestiaXYZVTrajectory)
    {
        switch (precision)
        {
        case TrajectoryPrecisionSingle:
            sampTrajectory = LoadXYZVTrajectorySinglePrec(strippedFilename, interpolation);
            break;
        case TrajectoryPrecisionDouble:
            sampTrajectory = LoadXYZVTrajectoryDoublePrec(strippedFilename, interpolation);
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
        case TrajectoryPrecisionSingle:
            sampTrajectory = LoadSampledTrajectorySinglePrec(strippedFilename, interpolation);
            break;
        case TrajectoryPrecisionDouble:
            sampTrajectory = LoadSampledTrajectoryDoublePrec(strippedFilename, interpolation);
            break;
        default:
            assert(0);
            break;
        }
    }

    return sampTrajectory;
}
