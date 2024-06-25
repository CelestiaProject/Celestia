// timeinfo.h
//
// Copyright (C) 2023, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

namespace celestia
{

struct TimeInfo
{
    static constexpr double MinimumTimeRate = 1.0e-15;

    double currentTime{ 0.0 };
    float fps{ 0.0f };
    int timeZoneBias{ 0 }; // Diff in secs between local time and GMT
    bool lightTravelFlag{ false };
};

}
