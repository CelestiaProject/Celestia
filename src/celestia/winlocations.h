// winlocations.h
// 
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// Miscellaneous utilities for Locations UI implementation.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELESTIA_WINLOCATIONS_H_
#define _CELESTIA_WINLOCATIONS_H_

#include <windows.h>
#include <commctrl.h>
#include "celestiacore.h"

class LocationsDialog : public CelestiaWatcher
{
 public:
    LocationsDialog(HINSTANCE, HWND, CelestiaCore*);

    void SetControls(HWND);
    void RestoreSettings(HWND);

    virtual void notifyChange(CelestiaCore*, int);

 public:
    CelestiaCore* appCore;
    HWND parent;
    HWND hwnd;
    int initialLocationFlags;
    float initialFeatureSize;
};

#endif // _CELESTIA_WINLOCATIONS_H_

