// skygrid.h
//
// Celestial longitude/latitude grids.
//
// Copyright (C) 2008-present, the Celestia Development Team
// Initial version by Chris Laurel, <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <Eigen/Geometry>

#include <celutil/color.h>

namespace celestia::engine
{

struct SkyGrid
{
    enum LongitudeUnits
    {
        LongitudeDegrees,
        LongitudeHours,
    };

    enum LongitudeDirection
    {
        IncreasingCounterclockwise,
        IncreasingClockwise,
    };

    Eigen::Quaterniond orientation{ Eigen::Quaterniond::Identity() };
    Color lineColor{ Color::White };
    Color labelColor{ Color::White };
    LongitudeUnits longitudeUnits{ LongitudeHours };
    LongitudeDirection longitudeDirection{ IncreasingCounterclockwise };
};

} // end namespace celestia::engine
