// winstarbrowser.h
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Star browser tool for Windows.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _WINSTARBROWSER_H_
#define _WINSTARBROWSER_H_

#include "celestiacore.h"


class StarBrowser
{
 public:
    StarBrowser(HINSTANCE, HWND, CelestiaCore*);
    ~StarBrowser();

 public:
    CelestiaCore* appCore;
    HWND parent;
    HWND hwnd;

    // The star browser data is valid for a particular point
    // in space, and for performance issues is not continuously
    // updated.
    Eigen::Vector3f pos;
    UniversalCoord ucPos;

    int predicate;
    int nStars;
};


#endif // _WINSTARBROWSER_H_
