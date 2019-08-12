// samporbit.h
//
// Copyright (C) 2002-2007, Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_SAMPORBIT_H_
#define _CELENGINE_SAMPORBIT_H_

#include "orbit.h"
#include <celutil/filesystem.h>

enum TrajectoryInterpolation
{
    TrajectoryInterpolationLinear,
    TrajectoryInterpolationCubic,
};

enum TrajectoryPrecision
{
    TrajectoryPrecisionSingle,
    TrajectoryPrecisionDouble
};

extern Orbit* LoadSampledTrajectoryDoublePrec(const fs::path& filename, TrajectoryInterpolation interpolation);
extern Orbit* LoadSampledTrajectorySinglePrec(const fs::path& filename, TrajectoryInterpolation interpolation);
extern Orbit* LoadXYZVTrajectoryDoublePrec(const fs::path& filename, TrajectoryInterpolation interpolation);
extern Orbit* LoadXYZVTrajectorySinglePrec(const fs::path& filename, TrajectoryInterpolation interpolation);

#endif // _CELENGINE_SAMPORBIT_H_
