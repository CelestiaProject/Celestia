// winbookmarks.h
//
// Copyright (C) 2023, Celestia Development Team
//
// Original version:
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// Miscellaneous utilities for Locations UI implementation.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <windows.h>

class CelestiaCore;

namespace celestia::win32
{

class ODMenu;

void ShowAddBookmarkDialog(HINSTANCE appInstance,
                           HMODULE hRes,
                           HWND hWnd,
                           HMENU menuBar,
                           ODMenu* odMenu,
                           CelestiaCore* appCore);

void ShowOrganizeBookmarksDialog(HINSTANCE appInstance,
                                 HMODULE hRes,
                                 HWND hWnd,
                                 HMENU menuBar,
                                 ODMenu* odMenu,
                                 CelestiaCore* appCore);

void BuildFavoritesMenu(HMENU menuBar,
                        CelestiaCore* appCore,
                        HINSTANCE appInstance,
                        ODMenu* odMenu);

}
