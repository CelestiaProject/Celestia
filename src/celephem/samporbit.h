// samporbit.h
//
// Copyright (C) 2002-2007, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>

#include <celcompat/filesystem.h>

namespace celestia::ephem
{

class Orbit;

enum class TrajectoryInterpolation
{
    Linear,
    Cubic,
};

enum class TrajectoryPrecision
{
    Single,
    Double,
};

std::shared_ptr<const Orbit> LoadSampledTrajectory(const fs::path&,
                                                   TrajectoryInterpolation,
                                                   TrajectoryPrecision);

} // end namespace celestia::ephem
