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

#include "winutil.h"

#include <windows.h>

namespace celestia::util
{

std::string
WideToUTF8(std::wstring_view ws)
{
    if (ws.empty())
        return {};

    // get a converted string length
    const auto srcLen = static_cast<int>(ws.size());
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.data(), srcLen, nullptr, 0, nullptr, nullptr);
    if (len <= 0)
        return {};

    std::string out(static_cast<std::string::size_type>(len), '\0');
    len = WideCharToMultiByte(CP_UTF8, 0, ws.data(), srcLen, out.data(), len, nullptr, nullptr);
    if (len <= 0)
        return {};

    out.resize(static_cast<std::size_t>(len));
    return out;
}

} // end namespace celestia::util
