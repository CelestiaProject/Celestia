// wintime.cpp
//
// Copyright (C) 2005, Chris Laurel <claurel@shatters.net>
//
// Win32 set time dialog box for Celestia
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

void ShowSetTimeDialog(HINSTANCE appInstance, HWND appWindow, CelestiaCore* appCore);

}
