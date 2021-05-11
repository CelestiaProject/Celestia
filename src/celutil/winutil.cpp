// winutil.cpp
//
// Copyright (C) 2019-present, Celestia Development Team
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// Miscellaneous useful Windows-related functions.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <windows.h>
#include "winutil.h"

using namespace std;

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

string WStringToString(UINT codePage, const wstring& ws)
{
    if (ws.empty())
        return {};
    // get a converted string length
    auto len = WideCharToMultiByte(codePage, 0, ws.c_str(), ws.length(), NULL, 0, nullptr, nullptr);
    string out(len, 0);
    WideCharToMultiByte(codePage, 0, ws.c_str(), ws.length(), &out[0], len, nullptr, nullptr);
    return out;
}

wstring StringToWString(UINT codePage, const string& s)
{
    if (s.empty())
        return {};
    // get a converted string length
    auto len = MultiByteToWideChar(codePage, 0, s.c_str(), s.length(), nullptr, 0);
    wstring out(len, 0);
    MultiByteToWideChar(codePage, 0, s.c_str(), s.length(), &out[0], len);
    return out;
}

string WideToCurrentCP(const wstring& ws)
{
    return WStringToString(CP_ACP, ws);
}

wstring CurrentCPToWide(const string& s)
{
    return StringToWString(CP_ACP, s);
}

string WideToUTF8(const wstring& ws)
{
    return WStringToString(CP_UTF8, ws);
}

wstring UTF8ToWide(const string& s)
{
    return StringToWString(CP_UTF8, s);
}
