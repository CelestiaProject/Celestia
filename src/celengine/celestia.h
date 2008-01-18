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

#ifdef _MSC_VER

#define BROKEN_FRIEND_TEMPLATES
#define snprintf _snprintf

#if _MSC_VER >= 1000
// Make VC shut up about long variable names from templates
#pragma warning(disable : 4786)
#endif // _MSC_VER

#endif // _MSC_VER


#define HAVE_SSTREAM
#ifdef __GNUC__
#undef HAVE_SSTREAM
#define NONSTANDARD_STRING_COMPARE
#endif // __GNUC__

#ifndef _WIN32
#ifndef TARGET_OS_MAC
#include <config.h>
#endif /* TARGET_OS_MAC */
#endif /* _WIN32 */



#endif // _CELESTIA_H_

