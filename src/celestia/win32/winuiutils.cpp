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
