// dateformatter.h
//
// Copyright (C) 2023-present, Celestia Development Team.
//
// ICU date formatter
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#ifdef USE_ICU
#ifdef HAVE_WIN_ICU_COMBINED_HEADER
#include <icu.h>
#elif defined(HAVE_WIN_ICU_SEPARATE_HEADERS)
#include <icucommon.h>
#include <icui18n.h>
#else
#include <unicode/udat.h>
#include <unicode/ustring.h>
#endif
#include <array>
#endif

#include <celengine/astro.h>

namespace celestia::engine
{

class DateFormatter
{
public:
    DateFormatter() = default;
    ~DateFormatter();
    DateFormatter(const DateFormatter &) = delete;
    DateFormatter(DateFormatter &&) = default;
    DateFormatter &operator=(const DateFormatter &) = delete;
    DateFormatter &operator=(DateFormatter &&) = delete;

    std::string formatDate(double tdb, bool local, astro::Date::Format format);

#ifdef USE_ICU
private:
    std::array<UDateFormat*, static_cast<size_t>(astro::Date::FormatCount)> localFormatters;
    std::array<UDateFormat*, static_cast<size_t>(astro::Date::FormatCount)> utcFormatters;

    UDateFormat *getFormatter(bool local, astro::Date::Format format);
#endif
};

}
