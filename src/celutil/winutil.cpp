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

namespace
{

std::string
WStringToString(UINT codePage, std::wstring_view ws)
{
    if (ws.empty())
        return {};

    // get a converted string length
    const auto srcLen = static_cast<int>(ws.size());
    const auto len = WideCharToMultiByte(codePage, 0, ws.data(), srcLen, nullptr, 0, nullptr, nullptr);
    if (len <= 0)
        return {};

    std::string out(static_cast<std::string::size_type>(len), '\0');
    WideCharToMultiByte(codePage, 0, ws.data(), srcLen, out.data(), len, nullptr, nullptr);
    return out;
}

std::wstring
StringToWString(UINT codePage, std::string_view s)
{
    if (s.empty())
        return {};

    // get a converted string length
    const auto srcLen = static_cast<int>(s.size());
    const auto len = MultiByteToWideChar(codePage, 0, s.data(), srcLen, nullptr, 0);
    if (len <= 0)
        return {};

    std::wstring out(static_cast<std::wstring::size_type>(len), L'\0');
    MultiByteToWideChar(codePage, 0, s.data(), srcLen, out.data(), len);
    return out;
}

inline std::wstring
UTF8ToWide(std::string_view s)
{
    return StringToWString(CP_UTF8, s);
}

inline std::string
WideToCurrentCP(std::wstring_view ws)
{
    return WStringToString(CP_ACP, ws);
}

} // end unnamed namespace

std::string
UTF8ToCurrentCP(std::string_view str)
{
    return WideToCurrentCP(UTF8ToWide(str));
}

std::string
WideToUTF8(std::wstring_view ws)
{
    return WStringToString(CP_UTF8, ws);
}

} // end namespace celestia::util
