// wineclipses.h by Kendrix <kendrix@wanadoo.fr>
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Compute Solar Eclipses for our Solar System planets
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _WINECLIPSES_H_
#define _WINECLIPSES_H_

#include <vector>

#include "celestiacore.h"


class EclipseFinderDialog
{
 public:
    EclipseFinderDialog(HINSTANCE, HWND, CelestiaCore*);

 public:
    CelestiaCore* appCore;
    HWND parent;
    HWND hwnd;

    std::string strPlaneteToFindOn;
    SYSTEMTIME fromTime, toTime;
    double TimetoSet_;
    Body* BodytoSet_;
    bool bSolar;
};

#endif // _WINECLIPSES_H_

