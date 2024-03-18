// localeutil.h
//
// Copyright (C) 2024-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <stdlib.h>  // for strtod_l
#if defined __APPLE__ || defined(__FreeBSD__)
# include <xlocale.h>  // for LC_NUMERIC_MASK on OS X
#endif

#ifdef _MSC_VER
typedef _locale_t locale_t;

enum { LC_NUMERIC_MASK = LC_NUMERIC };

static inline locale_t newlocale(int category_mask, const char *locale, locale_t)
{
    return _create_locale(category_mask, locale);
}

static inline void freelocale(locale_t locale)
{
    _free_locale(locale);
}

static inline float strtof_l(const char *nptr, char **endptr, locale_t loc)
{
    return _strtof_l(nptr, endptr, loc);
}

static inline double strtod_l(const char *nptr, char **endptr, locale_t loc)
{
    return _strtod_l(nptr, endptr, loc);
}

static inline long double strtold_l(const char *nptr, char **endptr, locale_t loc)
{
    return _strtold_l(nptr, endptr, loc);
}
#endif
