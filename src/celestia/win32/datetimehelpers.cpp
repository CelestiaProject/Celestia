// datehelpers.cpp
//
// Copyright (C) 2023, Celestia Development Team
//
// Utilities for date handling in the Windows UI
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "datetimehelpers.h"

#include <array>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#include <windows.h>

using namespace std::string_view_literals;

namespace celestia::win32
{

namespace
{

std::array<std::wstring, 12>
CreateLocalizedMonthNames()
{
    constexpr std::array<CALTYPE, 12> monthConstants
    {
        CAL_SABBREVMONTHNAME1,
        CAL_SABBREVMONTHNAME2,
        CAL_SABBREVMONTHNAME3,
        CAL_SABBREVMONTHNAME4,
        CAL_SABBREVMONTHNAME5,
        CAL_SABBREVMONTHNAME6,
        CAL_SABBREVMONTHNAME7,
        CAL_SABBREVMONTHNAME8,
        CAL_SABBREVMONTHNAME9,
        CAL_SABBREVMONTHNAME10,
        CAL_SABBREVMONTHNAME11,
        CAL_SABBREVMONTHNAME12,
    };

    constexpr std::array<std::wstring_view, 12> defaultMonthNames
    {
        L"Jan"sv, L"Feb"sv, L"Mar"sv,
        L"Apr"sv, L"May"sv, L"Jun"sv,
        L"Jul"sv, L"Aug"sv, L"Sep"sv,
        L"Oct"sv, L"Nov"sv, L"Dec"sv,
    };

    std::array<std::wstring, 12> months;

    for (std::size_t i = 0; i < 12; ++i)
    {
        CALTYPE calType = monthConstants[i];
        int length = GetCalendarInfoEx(LOCALE_NAME_USER_DEFAULT, CAL_GREGORIAN, nullptr, calType, nullptr, 0, nullptr);
        // length includes the null terminator, so also exclude length = 1
        if (length <= 1)
        {
            months[i] = defaultMonthNames[i];
            continue;
        }

        std::wstring& name = months[i];
        name.resize(static_cast<std::size_t>(length), L'\0');
        length = GetCalendarInfoEx(LOCALE_NAME_USER_DEFAULT, CAL_GREGORIAN, nullptr, calType, name.data(), length, nullptr);
        if (length > 1)
            name.resize(static_cast<std::size_t>(length - 1));
        else
            name = defaultMonthNames[i];
    }

    return months;
}

} // end unnamed namespace

std::span<const std::wstring, 12>
GetLocalizedMonthNames()
{
    static const std::array<std::wstring, 12> monthNames = CreateLocalizedMonthNames();
    return monthNames;
}

} // end namespace celestia::win32
