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

#pragma once

#include <vector>

#include <celengine/starbrowser.h>

#include <windows.h>

class CelestiaCore;

namespace celestia::win32
{

class StarBrowser
{
public:
    StarBrowser(HINSTANCE, HWND, CelestiaCore*);
    ~StarBrowser();

public:
    CelestiaCore* appCore;
    HWND parent;
    HWND hwnd;

    int sortColumn{-1};
    bool sortColumnReverse{false};

    celestia::engine::StarBrowser starBrowser;
    std::vector<celestia::engine::StarBrowserRecord> stars;
};

} // end namespace celestia::win32
