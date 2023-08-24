// dateformatter.cpp
//
// Copyright (C) 2023-present, Celestia Development Team.
//
// ICU date formatter
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "dateformatter.h"
#ifdef USE_ICU
#include <celutil/gettext.h>
#endif

namespace celestia::engine
{

DateFormatter::~DateFormatter()
{
#ifdef USE_ICU
    for (const auto &formatter : localFormatters)
        if (formatter != nullptr)
            udat_close(formatter);
    for (const auto &formatter : utcFormatters)
        if (formatter != nullptr)
            udat_close(formatter);
#endif
}


std::string DateFormatter::formatDate(double tdb, bool local, astro::Date::Format format)
{
#ifdef USE_ICU
    auto formatter = getFormatter(local, format);
    if (formatter == nullptr)
        return {};

    UErrorCode error = U_ZERO_ERROR;
    static auto epoch = astro::Date(1970, 1, 1);
    auto date = (astro::TDBtoUTC(tdb) - epoch) * 86400.0 * 1000.0;
    auto requiredSize = udat_format(formatter, date, nullptr, 0, nullptr, &error);
    if (U_FAILURE(error) && error != U_BUFFER_OVERFLOW_ERROR)
        return {};

    error = U_ZERO_ERROR;

    std::u16string formattedDate;
    formattedDate.resize(requiredSize);
    udat_format(formatter, date, formattedDate.data(), requiredSize, nullptr, &error);

    if (U_FAILURE(error))
        return {};

    u_strToUTF8(nullptr, 0, &requiredSize, formattedDate.data(), formattedDate.size(), &error);
    if (U_FAILURE(error) && error != U_BUFFER_OVERFLOW_ERROR)
        return {};

    error = U_ZERO_ERROR;
    std::string utf8FormattedDate;
    utf8FormattedDate.resize(requiredSize);

    u_strToUTF8(utf8FormattedDate.data(), requiredSize, nullptr, formattedDate.data(), formattedDate.size(), &error);

    if (U_FAILURE(error))
        return {};

    return utf8FormattedDate;
#else
    astro::Date d = local ? astro::TDBtoLocal(tdb) : astro::TDBtoUTC(tdb);
    return d.toCStr(format);
#endif
}

#ifdef USE_ICU
UDateFormat *DateFormatter::getFormatter(bool local, astro::Date::Format format)
{
    auto& formatters = local ? localFormatters : utcFormatters;
    auto index = static_cast<size_t>(format);
    if (auto formatter = formatters[index]; formatter != nullptr)
        return formatter;

    static const char *locale = nullptr;
    if (locale == nullptr)
    {
        const char *orig = N_("LANGUAGE");
        const char *lang = _(orig);
        locale = lang == orig ? "en" : lang;
    }

    const UChar *pattern;
    UDateFormatStyle dateStyle;
    UDateFormatStyle timeStyle;
    switch (format)
    {
    case astro::Date::ISO8601:
        pattern = u"yyyy-MM-dd'T'HH:mm:ss.SSSZZZZZ";
        dateStyle = UDAT_PATTERN;
        timeStyle = UDAT_PATTERN;
        break;
    case astro::Date::Locale:
        pattern = nullptr;
        dateStyle = UDAT_LONG;
        timeStyle = UDAT_MEDIUM;
        break;
    case astro::Date::TZName:
        pattern = u"yyyy MMM dd HH:mm:ss zzz";
        dateStyle = UDAT_PATTERN;
        timeStyle = UDAT_PATTERN;
        break;
    default:
        pattern = u"yyyy MMM dd HH:mm:ss ZZ";
        dateStyle = UDAT_PATTERN;
        timeStyle = UDAT_PATTERN;
        break;
    }

    UErrorCode error = U_ZERO_ERROR;
    auto formatter = udat_open(timeStyle, dateStyle, locale, local ? nullptr : u"UTC", -1, pattern, -1, &error);
    if (U_FAILURE(error))
        return nullptr;

    formatters[index] = formatter;
    return formatter;
}
#endif

}
