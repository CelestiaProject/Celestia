// cc.cpp
//
// Copyright (C) 2021-present, Celestia Development Team.
//
// from_chars() functions family as in C++17.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "cc.h"

namespace celestia
{
namespace compat
{

auto from_chars(const char* first, const char* last, float &value,
                celestia::compat::chars_format fmt)
    -> celestia::compat::from_chars_result
{
    char *end;
    errno = 0;
    float result = strtof(first, &end);
    if (result == 0.0f && end == first)
        return { first, std::errc::invalid_argument };
    if (errno == ERANGE)
    {
        errno = 0;
        return { first, std::errc::invalid_argument };
    }
    value = result;
    return { end, std::errc() };
}

auto from_chars(const char* first, const char* last, double &value,
                celestia::compat::chars_format fmt)
    -> celestia::compat::from_chars_result
{
    char *end;
    errno = 0;
    double result = strtod(first, &end);
    if (result == 0.0 && end == first)
        return { first, std::errc::invalid_argument };
    if (errno == ERANGE)
    {
        errno = 0;
        return { first, std::errc::invalid_argument };
    }
    value = result;
    return { end, std::errc() };
}

auto from_chars(const char* first, const char* last, long double &value,
                celestia::compat::chars_format fmt)
    -> celestia::compat::from_chars_result
{
    char *end;
    errno = 0;
    double result = strtold(first, &end);
    if (result == 0.0l && end == first)
        return { first, std::errc::invalid_argument };
    if (errno == ERANGE)
    {
        errno = 0;
        return { first, std::errc::invalid_argument };
    }
    value = result;
    return { end, std::errc() };
}

}
}
