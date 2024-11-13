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

#include <iterator>
#include <string>

#include <libintl.h>
#include <fmt/format.h>

namespace celestia::util::detail
{

inline const char*
pgettextAux(const char* ctx_string, const char* fallback)
{
    const char* translation = gettext(ctx_string);
    return translation == ctx_string ? fallback : translation;
}

inline const char*
dpgettextAux(const char* ctx_string, const char* fallback)
{
    const char* translation = dgettext("celestia-data", ctx_string);
    return translation == ctx_string ? fallback : translation;
}

}

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

#ifndef C_
#define C_(ctx_string, string) ::celestia::util::detail::pgettextAux(ctx_string "\004" string, string)
#endif

#ifdef CX_
#undef CX_
#endif

inline const char*
CX_(const char *ctx_string, const char *string)
{
    fmt::basic_memory_buffer<char> buffer;
    fmt::format_to(std::back_inserter(buffer), "{}\004{}", ctx_string, string);
    buffer.push_back('\0');
    return celestia::util::detail::pgettextAux(buffer.data(), string);
}

inline const char*
CX_(const char *ctx_string, const std::string &string)
{
    return CX_(ctx_string, string.c_str());
}

#ifndef NC_
#define NC_(ctx_string, string) string
#endif

#ifndef D_
#define D_(string) dgettext("celestia-data", string)
#endif

#ifndef DC_
#define DC_(ctx_string, string) ::celestia::util::detail::dpgettextAux(ctx_string "\004" string, string)
#endif

#ifdef DCX_
#undef DCX_
#endif

inline const char*
DCX_(const char *ctx_string, const char *string)
{
    fmt::basic_memory_buffer<char> buffer;
    fmt::format_to(std::back_inserter(buffer), "{}\004{}", ctx_string, string);
    buffer.push_back('\0');
    return celestia::util::detail::dpgettextAux(buffer.data(), string);
}

inline const char*
DCX_(const char *ctx_string, const std::string &string)
{
    return DCX_(ctx_string, string.c_str());
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

#ifndef D_
#define D_(string) string
#endif

#ifndef DC_
#define DC_(ctx_string, string) string
#endif

#ifndef DCX_
#define DCX_(ctx_string, string) string
#endif

#endif // ENABLE_NLS
