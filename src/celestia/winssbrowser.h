// winssbrowser.h
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Solar system browser tool for Windows.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _WINSSBROWSER_H_
#define _WINSSBROWSER_H_

#include "celestiacore.h"


class SolarSystemBrowser
{
 public:
    SolarSystemBrowser(HINSTANCE, HWND, CelestiaCore*);
    ~SolarSystemBrowser();

 public:
    CelestiaCore* appCore;
    HWND parent;
    HWND hwnd;
};


#endif // _WINSTARBROWSER_H_
