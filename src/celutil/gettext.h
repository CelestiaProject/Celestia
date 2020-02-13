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

// gettext / libintl setup
#ifndef _ /* unless somebody already took care of this */
#define _(string) gettext (string)
#endif

#ifndef gettext_noop
#define gettext_noop(string) string
#endif

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

#endif // ENABLE_NLS
