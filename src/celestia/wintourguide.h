// wintourguide.h
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Space 'tour guide' dialog for Windows.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _WINTOURGUIDE_H_
#define _WINTOURGUIDE_H_

#include "celestiacore.h"


class TourGuide
{
 public:
    TourGuide(HINSTANCE, HWND, CelestiaCore*);

 public:
    CelestiaCore* appCore;
    Destination* selectedDest;
    HWND parent;
    HWND hwnd;
};


#endif // _TOURGUIDE_H_

