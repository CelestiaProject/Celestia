// trajmanager.cpp
//
// Copyright (C) 2001-2008 Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "trajmanager.h"

#include <cassert>
#include <fstream>

#include <celutil/filetype.h>
#include <celutil/logger.h>

using celestia::ephem::TrajectoryInterpolation;
using celestia::ephem::TrajectoryPrecision;
using celestia::util::GetLogger;

TrajectoryManager*
GetTrajectoryManager()
{
    static TrajectoryManager* trajectoryManager = nullptr;
    if (trajectoryManager == nullptr)
        trajectoryManager = std::make_unique<TrajectoryManager>("data").release();
    return trajectoryManager;
}


TrajectoryInfo::ResourceKey
TrajectoryInfo::resolve(const fs::path& baseDir) const
{
    if (!path.empty())
    {
        fs::path filename = path / "data" / source;
        std::ifstream in(filename);
        if (in.good())
            return ResourceKey(std::move(filename), interpolation, precision);
    }

    return ResourceKey(baseDir / source, interpolation, precision);
}

std::unique_ptr<celestia::ephem::Orbit>
TrajectoryInfo::load(const ResourceKey& key) const
{
    ContentType filetype = DetermineFileType(key.resolvedPath);

    GetLogger()->debug("Loading trajectory: {}\n", key.resolvedPath);

    // TODO use unique_ptr here and replace the use of .release()
    std::unique_ptr<celestia::ephem::Orbit> sampTrajectory = nullptr;

    if (filetype == ContentType::CelestiaXYZVTrajectory)
    {
        switch (key.precision)
        {
        case TrajectoryPrecision::Single:
            sampTrajectory = LoadXYZVTrajectorySinglePrec(key.resolvedPath, key.interpolation);
            break;
        case TrajectoryPrecision::Double:
            sampTrajectory = LoadXYZVTrajectoryDoublePrec(key.resolvedPath, key.interpolation);
            break;
        default:
            assert(0);
            break;
        }
    }
    else if (filetype == ContentType::CelestiaXYZVBinary)
    {
        switch (key.precision)
        {
        case TrajectoryPrecision::Single:
            sampTrajectory = LoadXYZVBinarySinglePrec(key.resolvedPath, key.interpolation);
            break;
        case TrajectoryPrecision::Double:
            sampTrajectory = LoadXYZVBinaryDoublePrec(key.resolvedPath, key.interpolation);
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
            sampTrajectory = LoadSampledTrajectorySinglePrec(key.resolvedPath, interpolation);
            break;
        case TrajectoryPrecision::Double:
            sampTrajectory = LoadSampledTrajectoryDoublePrec(key.resolvedPath, interpolation);
            break;
        default:
            assert(0);
            break;
        }
    }

    return sampTrajectory;
}
