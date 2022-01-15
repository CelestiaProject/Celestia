// winuiutils.h
//
// Copyright (C) 2021-present, Celestia Development Team
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// Miscellaneous useful Windows-related functions.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <windows.h>
#include <commctrl.h>

namespace celestia::win32
{

void SetMouseCursor(LPCTSTR lpCursor);
void CenterWindow(HWND hParent, HWND hWnd);
void RemoveButtonDefaultStyle(HWND hWnd);
void AddButtonDefaultStyle(HWND hWnd);
int DpToPixels(int dp, HWND hWnd);
UINT GetDPIForWindow(HWND hWnd);
int GetSystemMetricsForWindow(int index, HWND hWnd);

}
