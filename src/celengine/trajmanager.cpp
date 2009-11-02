// trajmanager.cpp
//
// Copyright (C) 2001-2008 Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "celestia.h"
#include "trajmanager.h"
#include <celephem/samporbit.h>
#include <celutil/debug.h>
#include <celutil/filetype.h>
#include <iostream>
#include <fstream>
#include <cassert>

using namespace std;


static TrajectoryManager* trajectoryManager = NULL;

static const char UniqueSuffixChar = '!';


TrajectoryManager* GetTrajectoryManager()
{
    if (trajectoryManager == NULL)
        trajectoryManager = new TrajectoryManager("data");
    return trajectoryManager;
}


string TrajectoryInfo::resolve(const string& baseDir)
{
    // Ensure that trajectories with different interpolation or precision get resolved to different objects by
    // adding a 'uniquifying' suffix to the filename that encodes the properties other than filename which can
    // distinguish two trajectories. This suffix is stripped before the file is actually loaded.
    char uniquifyingSuffix[32];
    sprintf(uniquifyingSuffix, "%c%u%u", UniqueSuffixChar, (unsigned int) interpolation, (unsigned int) precision);

    if (!path.empty())
    {
        string filename = path + "/data/" + source;
        ifstream in(filename.c_str());
        if (in.good())
            return filename + uniquifyingSuffix;
    }

    return baseDir + "/" + source + uniquifyingSuffix;
}

Orbit* TrajectoryInfo::load(const string& filename)
{
    // strip off the uniquifying suffix
    string::size_type uniquifyingSuffixStart = filename.rfind(UniqueSuffixChar);
    string strippedFilename(filename, 0, uniquifyingSuffixStart);
    ContentType filetype = DetermineFileType(strippedFilename);

    DPRINTF(1, "Loading trajectory: %s\n", strippedFilename.c_str());

    Orbit* sampTrajectory = NULL;

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
