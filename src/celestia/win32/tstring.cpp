// tstring.cpp
//
// Copyright (C) 2023, Celestia Development Team
//
// String conversions to/from Windows character sets
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "tstring.h"

#include <algorithm>

namespace celestia::win32
{

namespace
{

int
NonLocalizedCompare(std::string_view lhs, std::string_view rhs)
{
    if (lhs == rhs)
        return 0;
    if (lhs < rhs)
        return -1;
    return 1;
}

} // end unnamed namespace

int
UTF8ToTChar(std::string_view str, tstring::value_type* dest, int destSize)
{
    if (str.empty())
        return 0;
    const auto srcLength = static_cast<int>(str.size());
#ifdef _UNICODE
    return std::max(MultiByteToWideChar(CP_UTF8, 0, str.data(), srcLength, dest, destSize), 0);
#else
    fmt::basic_memory_buffer<wchar_t> wbuffer;
    int wideLength = AppendUTF8ToWide(str, wbuffer);
    if (wideLength <= 0)
        return 0;

    return std::max(WideCharToMultiByte(CP_ACP, 0, wbuffer.data(), wideLength, dest, destSize, nullptr, nullptr), 0);
#endif
}

int
CompareUTF8Localized(std::string_view lhs, std::string_view rhs)
{
    if (lhs.empty())
        return rhs.empty() ? 0 : -1;
    if (rhs.empty())
        return 1;

    fmt::basic_memory_buffer<wchar_t, 256> wbuffer0;
    int wlength0 = AppendUTF8ToWide(lhs, wbuffer0);

    fmt::basic_memory_buffer<wchar_t, 256> wbuffer1;
    int wlength1 = AppendUTF8ToWide(rhs, wbuffer1);

    if (wlength0 <= 0 || wlength1 <= 0)
        return NonLocalizedCompare(lhs, rhs);

    int result = CompareStringEx(LOCALE_NAME_USER_DEFAULT, NORM_LINGUISTIC_CASING,
                                 wbuffer0.data(), wlength0,
                                 wbuffer1.data(), wlength1,
                                 nullptr, nullptr, 0);
    if (result <= 0)
        return NonLocalizedCompare(lhs, rhs);

    return result - 2;
}

}
