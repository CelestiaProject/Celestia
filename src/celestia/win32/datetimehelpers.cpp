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

std::vector<tstring>
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

    constexpr std::array<tstring_view, monthCount> defaultMonthNames
    {
        TEXT("Jan"sv), TEXT("Feb"sv), TEXT("Mar"sv),
        TEXT("Apr"sv), TEXT("May"sv), TEXT("Jun"sv),
        TEXT("Jul"sv), TEXT("Aug"sv), TEXT("Sep"sv),
        TEXT("Oct"sv), TEXT("Nov"sv), TEXT("Dec"sv),
    };

    std::vector<tstring> months;
    months.reserve(monthCount);

#ifndef _UNICODE
    fmt::basic_memory_buffer<wchar_t, 256> buffer;
#endif
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

#ifdef _UNICODE
        std::wstring& name = months.emplace_back(static_cast<std::size_t>(length), L'\0');
        length = GetCalendarInfoEx(LOCALE_NAME_USER_DEFAULT, CAL_GREGORIAN, nullptr, calType, name.data(), length, nullptr);
        if (length > 1)
            name.resize(static_cast<std::size_t>(length - 1));
        else
            name = defaultMonthNames[i];
#else
        buffer.resize(static_cast<std::size_t>(length));
        GetCalendarInfoEx(LOCALE_NAME_USER_DEFAULT, CAL_GREGORIAN, nullptr, calType, buffer.data(), length, nullptr);
        months.push_back(WideToCurrentCP(buffer.data()));
#endif
    }

    return months;
}

} // end unnamed namespace

util::array_view<tstring>
GetLocalizedMonthNames()
{
    static const std::vector<tstring> monthNames = CreateLocalizedMonthNames();
    return monthNames;
}

} // end namespace celestia::win32
