// wineclipses.h by Kendrix <kendrix@wanadoo.fr>
// modified by Celestia Development Team
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Compute Solar Eclipses for our Solar System planets
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string>
#include <vector>

#include <celestia/eclipsefinder.h>
#include <celutil/array_view.h>

#include <windows.h>

#include "tstring.h"

class Body;
class CelestiaCore;

namespace celestia::win32
{

class EclipseFinderDialog
{
public:
    enum class TargetBody
    {
        Earth,
        Jupiter,
        Saturn,
        Uranus,
        Neptune,
        Pluto,
        Count_,
    };

    EclipseFinderDialog(HINSTANCE, HWND, CelestiaCore*);
    ~EclipseFinderDialog();

    CelestiaCore* appCore;
    HWND parent;
    HWND hwnd;

    std::vector<Eclipse> eclipseList;
    util::array_view<tstring> monthNames;
    TargetBody targetBody;
    SYSTEMTIME fromTime, toTime;
    double TimetoSet_;
    Body* BodytoSet_;
    Eclipse::Type type;

    int sortColumn{ -1 };
    bool sortColumnReverse{ false };
};

} // end namespace celestia::win32
