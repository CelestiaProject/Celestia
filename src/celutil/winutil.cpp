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

const char* CurrentCP()
{
    static bool set = false;
    static char cp[20] = "CP";
    if (!set)
    {
        GetLocaleInfoA(GetThreadLocale(), LOCALE_IDEFAULTANSICODEPAGE, cp+2, 18);
        set = true;
    }
    return cp;
}

string UTF8ToCurrentCP(const string& str)
{
    return WideToCurrentCP(UTF8ToWide(str));
}

string CurrentCPToUTF8(const string& str)
{
    return WideToUTF8(CurrentCPToWide(str));
}

string WideToCurrentCP(const wstring& ws)
{
    string out(ws.length()+1, 0);
    WideCharToMultiByte(CP_ACP, 0, ws.c_str(), -1, &out[0], ws.length()+1, nullptr, nullptr);
    return out;
}

wstring CurrentCPToWide(const string& str)
{
    wstring w(str.length()+1, 0);
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &w[0], str.length()+1);
    return w;
}

string WideToUTF8(const wstring& ws)
{
    // get a converted string length
    auto len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    string out(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &out[0], len, nullptr, nullptr);
    return out;
}

wstring UTF8ToWide(const string& s)
{
    // get a converted string length
    auto len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    wstring out(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &out[0], len);
    return out;
}
