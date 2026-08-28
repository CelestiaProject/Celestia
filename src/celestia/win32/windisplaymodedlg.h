// windisplaymodedlg.h
//
// Copyright (C) 2023, Celestia Development Team
//
// Extracted from winmain.cpp:
// Copyright (C) 2001-2007, Chris Laurel <claurel@shatters.net>
//
// Windows display mode selection.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <span>

#include <windows.h>

namespace celestia::win32
{

class DisplayModeDialog
{
public:
    DisplayModeDialog(HINSTANCE, HWND, std::span<const DEVMODE>, int);

    HWND parent;
    HWND hwnd{ nullptr };
    std::span<const DEVMODE> displayModes;
    int screenMode;
    bool update{ false };
};

}
