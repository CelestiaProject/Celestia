// celestia.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELESTIA_H_
#define _CELESTIA_H_

#if _MSC_VER >= 1000
// Make VC shut up about long variable names from templates
#pragma warning(disable : 4786)
#endif // _MSC_VER

#ifndef DEBUG
#define DPRINTF //
#else
#define DPRINTF DebugPrint
extern void DebugPrint(char *format, ...);
#endif

#ifdef _MSC_VER
#define BROKEN_FRIEND_TEMPLATES
#endif // _MSC_VER

#define HAVE_SSTREAM
#ifdef __GNUC__
#undef HAVE_SSTREAM
#define NONSTANDARD_STRING_COMPARE
#endif // __GNUC__

extern void Log(char *format, ...);

#endif // _CELESTIA_H_

