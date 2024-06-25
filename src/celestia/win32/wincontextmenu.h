// wincontextmenu.h
//
// Copyright (C) 2023, Celestia Development Team
//
// Extracted from winmain.cpp:
// Copyright (C) 2001-2007, Chris Laurel <claurel@shatters.net>
//
// Windows context menu.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <celestia/celestiacore.h>

#include <windows.h>

class Selection;

namespace celestia::win32
{

class MainWindow;

constexpr inline UINT MENU_CHOOSE_PLANET  = 32000;
constexpr inline UINT MENU_CHOOSE_SURFACE = 31000;

class WinContextMenuHandler : public CelestiaCore::ContextMenuHandler
{
public:
    WinContextMenuHandler(const CelestiaCore*, HWND, MainWindow*);

    void requestContextMenu(float x, float y, Selection sel) override;
    void setHwnd(HWND newHWnd) { hwnd = newHWnd; }

private:
    const CelestiaCore* appCore;
    HWND hwnd;
    MainWindow* mainWindow;
};

}
