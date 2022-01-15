// winuiutils.cpp
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

#include "winuiutils.h"

namespace
{
using GetDpiForWindowFn = UINT STDAPICALLTYPE(HWND hWnd);
using GetDpiForSystemFn = UINT STDAPICALLTYPE();
using GetSystemMetricsForDpiFn = UINT STDAPICALLTYPE(int nIndex, UINT dpi);
GetDpiForWindowFn *pfnGetDpiForWindow = nullptr;
GetDpiForSystemFn *pfnGetDpiForSystem = nullptr;
GetSystemMetricsForDpiFn *pfnGetSystemMetricsForDpi = nullptr;
bool dpiFunctionPointersInitialized = false;

void InitializeDPIFunctionPointersIfNeeded()
{
    if (dpiFunctionPointersInitialized)
        return;

    HMODULE hUser = GetModuleHandle("user32.dll");
    if (!hUser)
        return;
    pfnGetDpiForWindow = reinterpret_cast<GetDpiForWindowFn *>(GetProcAddress(hUser, "GetDpiForWindow"));
    pfnGetDpiForSystem = reinterpret_cast<GetDpiForSystemFn *>(GetProcAddress(hUser, "GetDpiForSystem"));
    pfnGetSystemMetricsForDpi = reinterpret_cast<GetSystemMetricsForDpiFn *>(GetProcAddress(hUser, "GetSystemMetricsForDpi"));
    dpiFunctionPointersInitialized = true;
}
}

namespace celestia::win32
{

void SetMouseCursor(LPCTSTR lpCursor)
{
    HCURSOR hNewCrsr;

    if (hNewCrsr = LoadCursor(nullptr, lpCursor))
        SetCursor(hNewCrsr);
}

void CenterWindow(HWND hParent, HWND hWnd)
{
    //Center window with hWnd handle relative to hParent.
    if (hParent && hWnd)
    {
        RECT ort, irt;
        if (GetWindowRect(hParent, &ort))
        {
            if (GetWindowRect(hWnd, &irt))
            {
                int x, y;

                x = ort.left + (ort.right - ort.left - (irt.right - irt.left)) / 2;
                y = ort.top + (ort.bottom - ort.top - (irt.bottom - irt.top)) / 2;;
                SetWindowPos(hWnd, HWND_TOP, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            }
        }
    }
}

void RemoveButtonDefaultStyle(HWND hWnd)
{
    SetWindowLong(hWnd, GWL_STYLE,
        ::GetWindowLong(hWnd, GWL_STYLE) & ~BS_DEFPUSHBUTTON);
    InvalidateRect(hWnd, nullptr, TRUE);
}

void AddButtonDefaultStyle(HWND hWnd)
{
    SetWindowLong(hWnd, GWL_STYLE,
        ::GetWindowLong(hWnd, GWL_STYLE) | BS_DEFPUSHBUTTON);
    InvalidateRect(hWnd, nullptr, TRUE);
}

UINT GetBaseDPI()
{
    return 96;
}

UINT GetDPIForWindow(HWND hWnd)
{
    InitializeDPIFunctionPointersIfNeeded();
    if (hWnd && pfnGetDpiForWindow)
        return pfnGetDpiForWindow(hWnd);
    if (pfnGetDpiForSystem)
        return pfnGetDpiForSystem();
    HDC hDC = GetDC(hWnd);
    if (hDC)
    {
        auto dpi = GetDeviceCaps(hDC, LOGPIXELSX);
        ReleaseDC(hWnd, hDC);
        return dpi;
    }
    return GetBaseDPI();
}

int DpToPixels(int dp, HWND hWnd)
{
    return dp * GetDPIForWindow(hWnd) / GetBaseDPI();
}

int GetSystemMetricsForWindow(int index, HWND hWnd)
{
    InitializeDPIFunctionPointersIfNeeded();
    auto dpi = GetDPIForWindow(hWnd);
    if (pfnGetSystemMetricsForDpi)
    {
        return pfnGetSystemMetricsForDpi(index, dpi);
    }
    auto systemDpi = GetDPIForWindow(nullptr);
    return GetSystemMetrics(index) * dpi / systemDpi;
}

}
