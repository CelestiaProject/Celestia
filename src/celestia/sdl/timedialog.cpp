// timedialog.cpp
//
// Copyright (C) 2025-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "timedialog.h"

#include <array>
#include <iterator>
#include <string_view>
#include <system_error>

#include <fmt/format.h>
#include <imgui.h>
#include <imgui_stdlib.h>

#include <celastro/date.h>
#include <celcompat/charconv.h>
#include <celengine/simulation.h>
#include <celestia/celestiacore.h>

namespace celestia::sdl
{

namespace
{

constexpr std::array<unsigned int, 12> MonthDays = { 31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U };

bool parseDate(std::string_view src, int& year, int& month, int& day)
{
    if (src.size() < 8)
        return false;

    auto ymSplit = src.find('-', 1U);
    if (ymSplit == std::string::npos)
        return false;

    std::string_view srcYear = src.substr(0, ymSplit);
    if (srcYear.size() < 2)
        return false;

    // charconv cannot handle leading +
    if (srcYear[0] == '+')
        srcYear = srcYear.substr(1);
    else if (srcYear[0] != '-')
        return false;

    if (auto [ptr, ec] = compat::from_chars(srcYear.data(), srcYear.data() + srcYear.size(), year);
        ec != std::errc{} || ptr != srcYear.data() + srcYear.size())
    {
        return false;
    }

    src = src.substr(ymSplit + 1);
    if (src.size() != 5 || src[2] != '-')
        return false;

    unsigned int uMonth;
    if (auto [ptr, ec] = compat::from_chars(src.data(), src.data() + 2, uMonth);
        ec != std::errc{} || ptr != src.data() + 2 || uMonth < 1 || uMonth > 12)
    {
        return false;
    }

    unsigned int monthEnd = MonthDays[uMonth - 1];
    if (uMonth == 2 && (year % 4) == 0 && (year <= 1582 || (year % 100) != 0 || (year % 400) == 0))
        ++monthEnd;

    unsigned int uDay;
    if (auto [ptr, ec] = compat::from_chars(src.data() + 3, src.data() + 5, uDay);
        ec != std::errc{} || ptr != src.data() + 5 || uDay < 1 || uDay > monthEnd)
    {
        return false;
    }

    // Julian-Gregorian transition
    if (year == 1582 && uMonth == 10 && day > 4 && day < 15)
        return false;

    month = static_cast<int>(uMonth);
    day = static_cast<int>(uDay);
    return true;
}

bool parseTime(std::string_view src, int& hour, int& minute, double& seconds)
{
    if (src.size() != 12 || src[2] != ':' || src[5] != ':' || src[8] != '.')
        return false;

    unsigned int uHour;
    if (auto [ptr, ec] = compat::from_chars(src.data(), src.data() + 2, uHour);
        ec != std::errc{} || ptr != src.data() + 2 || uHour >= 24)
    {
        return false;
    }

    unsigned int uMinute;
    if (auto [ptr, ec] = compat::from_chars(src.data() + 3, src.data() + 5, uMinute);
        ec != std::errc{} || ptr != src.data() + 5 || uMinute >= 60)
    {
        return false;
    }

    src = src.substr(6);
    for (std::string_view::size_type i = 0; i < src.size(); ++i)
    {
        if (i == 2)
        {
            if (src[i] != '.')
                return false;
        }
        else
        {
            if (src[i] < '0' || src[i] > '9')
                return false;
        }
    }

    if (auto [ptr, ec] = compat::from_chars(src.data(), src.data() + src.size(), seconds);
        ec != std::errc{} || ptr != src.data() + src.size() || seconds < 0.0 || seconds >= 60.0)
    {
        return false;
    }

    hour = static_cast<int>(uHour);
    minute = static_cast<int>(uMinute);
    return true;
}

} // end unnamed namespace

TimeDialog::TimeDialog(CelestiaCore* appCore) : m_appCore(appCore)
{
}

void
TimeDialog::setTime(double tdb)
{
    m_tdb = tdb;
    setDateTimeStrings();
}

void
TimeDialog::show(bool* isOpen)
{
    if (!*isOpen)
        return;

    if (ImGui::Begin("Set time", isOpen))
    {
        if (ImGui::InputText("Date (TDB)", &m_dateString))
            setTDB();
        if (ImGui::InputText("Time (TDB)", &m_timeString))
            setTDB();

        ImGui::Separator();
        if (ImGui::InputDouble("TDB", &m_tdb, 1e-8, 1, "%.8f"))
            setDateTimeStrings();

        ImGui::Separator();
        if (!m_isValid)
            ImGui::BeginDisabled();

        if (ImGui::Button("Ok##setDate"))
            m_appCore->getSimulation()->getActiveObserver()->setTime(m_tdb);

        if (!m_isValid)
            ImGui::EndDisabled();
    }
    ImGui::End();
}

void
TimeDialog::setDateTimeStrings()
{
    astro::Date date(m_tdb);
    m_dateString.clear();

    fmt::format_to(std::back_inserter(m_dateString), "{:+}-{:02}-{:02}", date.year, date.month, date.day);
    m_timeString.clear();
    fmt::format_to(std::back_inserter(m_timeString), "{:02}:{:02}:{:.3f}", date.hour, date.minute, date.seconds);
    m_isValid = true;
}

void
TimeDialog::setTDB()
{
    astro::Date date;
    if (!parseDate(m_dateString, date.year, date.month, date.day)
        || !parseTime(m_timeString, date.hour, date.minute, date.seconds))
    {
        m_isValid = false;
        return;
    }

    m_isValid = true;
    m_tdb = static_cast<double>(date);
}

} // end namespace celestia::sdl
