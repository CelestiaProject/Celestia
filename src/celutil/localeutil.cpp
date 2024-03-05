// localeutil.cpp
//
// Copyright (C) 2024-present, Celestia Development Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <clocale>
#include <locale>
#include <fmt/format.h>

namespace celestia::util
{

void InitLocale()
{
    /* try to set locale, fallback to "classic" */
    if (setlocale(LC_ALL, ""))
    {
        std::locale::global(std::locale(""));
    }
    else
    {
        fmt::print("Could not find locale, falling back to classic.\n");
    }
    /* Force number displays into C locale. */
    setlocale(LC_NUMERIC, "C");
}

}
