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

#include <string>
#include <celengine/orbit.h>

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

extern Orbit* LoadSampledTrajectoryDoublePrec(const std::string& name, TrajectoryInterpolation interpolation);
extern Orbit* LoadSampledTrajectorySinglePrec(const std::string& name, TrajectoryInterpolation interpolation);
extern Orbit* LoadXYZVTrajectoryDoublePrec(const std::string& name, TrajectoryInterpolation interpolation);
extern Orbit* LoadXYZVTrajectorySinglePrec(const std::string& name, TrajectoryInterpolation interpolation);

#endif // _CELENGINE_SAMPORBIT_H_
