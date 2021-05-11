// gettext.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
// Copyright (C) 2020, the Celestia Development Team
//
// Miscellaneous useful functions.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#ifdef ENABLE_NLS

#include <libintl.h>
#include <fmt/format.h>

inline const char* cel_pgettext_aux(const char *ctx_string, const char *fallback);

// gettext / libintl setup
#ifndef _ /* unless somebody already took care of this */
#define _(string) gettext(string)
#endif

#ifndef gettext_noop
#define gettext_noop(string) string
#endif

#ifndef N_
#define N_(string) gettext_noop(string)
#endif

#ifndef pgettext
#define pgettext(ctx_string, string) cel_pgettext_aux(ctx_string "\004" string, string)
#endif

#ifndef C_
#define C_(ctx_string, string) pgettext(ctx_string, string)
#endif

#ifndef CX_
#undef CX_
inline const char* CX_(const char *ctx_string, const char *string)
{
    return cel_pgettext_aux(fmt::format("{}\004{}", ctx_string, string).c_str(), string);
}

inline const char* CX_(const char *ctx_string, const std::string &string)
{
    return CX_(ctx_string, string.c_str());
}
#endif

#ifndef NC_
#define NC_(ctx_string, string) string
#endif

inline const char* cel_pgettext_aux(const char *ctx_string, const char *fallback)
{
    const char *translation = gettext(ctx_string);
    return translation == ctx_string ? fallback : translation;
}

#ifdef _WIN32
// POSIX provides an extension to printf family to reorder their arguments,
// so GNU GetText provides own replacement for them on windows platform
#ifdef fprintf
#undef fprintf
#endif
#ifdef printf
#undef printf
#endif
#ifdef sprintf
#undef sprintf
#endif
#endif // _WIN32

#else // ENABLE_NLS

#ifndef _
#define _(string) string
#endif

#ifndef gettext_noop
#define gettext_noop(string) string
#endif

#ifndef N_
#define N_(string) string
#endif

#ifndef C_
#define C_(ctx_string, string) string
#endif

#ifndef CX_
#define CX_(ctx_string, string) string
#endif

#ifndef NC_
#define NC_(ctx_string, string) string
#endif

#endif // ENABLE_NLS
