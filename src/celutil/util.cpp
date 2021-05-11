// util.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Miscellaneous useful functions.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifdef _WIN32
#include <windows.h>
#include <celutil/winutil.h>
#endif
#include <iostream>
#include "util.h"
#include "gettext.h"

using namespace std;

bool GetTZInfo(std::string_view tzName, int &dstBias)
{
#ifdef _WIN32
    TIME_ZONE_INFORMATION tzi;
    DWORD dst = GetTimeZoneInformation(&tzi);
    if (dst == TIME_ZONE_ID_INVALID)
        return false;

    LONG bias = 0;
    WCHAR* name = nullptr;

    switch (dst)
    {
    case TIME_ZONE_ID_STANDARD:
        bias = tzi.StandardBias;
        name = tzi.StandardName;
        break;
    case TIME_ZONE_ID_DAYLIGHT:
        bias = tzi.DaylightBias;
        name = tzi.DaylightName;
        break;
    default:
        cerr << _("Unknown value returned by GetTimeZoneInformation()\n");
        return false;
    }

    tzName = name == nullptr ? "   " : WideToUTF8(name);
    dstBias = (tzi.Bias + bias) * -60;
    return true;
#else
    struct tm result;
    time_t curtime = time(nullptr); // required only to get TZ info
    if (!localtime_r(&curtime, &result))
        return false;

    dstBias = result.tm_gmtoff;
    tzName = result.tm_zone;
    return true;
#endif
}
