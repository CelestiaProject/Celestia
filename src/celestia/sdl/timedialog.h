// timedialog.h
//
// Copyright (C) 2025-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string>

class CelestiaCore;

namespace celestia::sdl
{

class TimeDialog
{
public:
    explicit TimeDialog(CelestiaCore*);
    ~TimeDialog() = default;

    TimeDialog(const TimeDialog&) = delete;
    TimeDialog& operator=(const TimeDialog&) = delete;
    TimeDialog(TimeDialog&&) noexcept = default;
    TimeDialog& operator=(TimeDialog&&) noexcept = default;

    void setTime(double tdb);
    void show(bool* isOpen);

private:
    void setTDB();
    void setDateTimeStrings();

    CelestiaCore* m_appCore;
    double m_tdb{ 0.0 };
    std::string m_dateString;
    std::string m_timeString;
    bool m_isValid{ false };
};

} // end namespace celestia::sdl
