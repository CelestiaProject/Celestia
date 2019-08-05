// winutil.cpp
//
// Copyright (C) 2019, Celestia Development Team
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "winutil.h"

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
        RECT o, i;
        if (GetWindowRect(hParent, &o))
        {
            if (GetWindowRect(hWnd, &i))
            {
                int x, y;

                x = o.left + (o.right - o.left - (i.right - i.left)) / 2;
                y = o.top + (o.bottom - o.top - (i.bottom - i.top)) / 2;;
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

const char* CurrentCP()
{
    static bool set = false;
    static char cp[20] = "CP";
    if (!set) {
        GetLocaleInfo(GetThreadLocale(), LOCALE_IDEFAULTANSICODEPAGE, cp+2, 18);
        set = true;
    }
    return cp;
}

string UTF8ToCurrentCP(const string& str)
{
    string localeStr;
    LPWSTR wout = new wchar_t[str.length() + 1];
    LPSTR out = new char[str.length() + 1];
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wout, str.length() + 1);
    WideCharToMultiByte(CP_ACP, 0, wout, -1, out, str.length() + 1, nullptr, nullptr);
    localeStr = out;
    delete [] wout;
    delete [] out;
    return localeStr;
}

string CurrentCPToUTF8(const string& str)
{
    string localeStr;
    LPWSTR wout = new wchar_t[str.length() + 1];
    LPSTR out = new char[str.length() + 1];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, wout, str.length() + 1);
    WideCharToMultiByte(CP_UTF8, 0, wout, -1, out, str.length() + 1, nullptr, nullptr);
    localeStr = out;
    delete [] wout;
    delete [] out;
    return localeStr;
}
