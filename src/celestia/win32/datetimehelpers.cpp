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

#include <fmt/format.h>

#include <windows.h>

using namespace std::string_view_literals;

namespace celestia::win32
{

namespace
{

std::vector<std::wstring>
CreateLocalizedMonthNames()
{
    constexpr std::size_t monthCount = 12;

    constexpr std::array<CALTYPE, monthCount> monthConstants
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

    constexpr std::array<std::wstring_view, monthCount> defaultMonthNames
    {
        L"Jan"sv, L"Feb"sv, L"Mar"sv,
        L"Apr"sv, L"May"sv, L"Jun"sv,
        L"Jul"sv, L"Aug"sv, L"Sep"sv,
        L"Oct"sv, L"Nov"sv, L"Dec"sv,
    };

    std::vector<std::wstring> months;
    months.reserve(monthCount);

    for (std::size_t i = 0; i < monthCount; ++i)
    {
        CALTYPE calType = monthConstants[i];
        int length = GetCalendarInfoEx(LOCALE_NAME_USER_DEFAULT, CAL_GREGORIAN, nullptr, calType, nullptr, 0, nullptr);
        // length includes the null terminator, so also exclude length = 1
        if (length <= 1)
        {
            months.emplace_back(defaultMonthNames[i]);
            continue;
        }

        std::wstring& name = months.emplace_back(static_cast<std::size_t>(length), L'\0');
        length = GetCalendarInfoEx(LOCALE_NAME_USER_DEFAULT, CAL_GREGORIAN, nullptr, calType, name.data(), length, nullptr);
        if (length > 1)
            name.resize(static_cast<std::size_t>(length - 1));
        else
            name = defaultMonthNames[i];
    }

    return months;
}

} // end unnamed namespace

util::array_view<std::wstring>
GetLocalizedMonthNames()
{
    static const std::vector<std::wstring> monthNames = CreateLocalizedMonthNames();
    return monthNames;
}

} // end namespace celestia::win32
