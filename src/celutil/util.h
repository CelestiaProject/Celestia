// util.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Miscellaneous useful functions.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELUTIL_UTIL_H_
#define _CELUTIL_UTIL_H_

#include <string>
#include <iostream>
#include <functional>
#include <libintl.h>
#include <celcompat/filesystem.h>

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
#endif

extern int compareIgnoringCase(const std::string& s1, const std::string& s2);
extern int compareIgnoringCase(const std::string& s1, const std::string& s2, int n);
extern fs::path LocaleFilename(const fs::path& filename);

struct CompareIgnoringCasePredicate
{
    bool operator()(const std::string&, const std::string&) const;
};

template <class T> struct printlineFunc
{
    printlineFunc(std::ostream& o) : out(o) {};
    void operator() (T x) { out << x << '\n'; };
    std::ostream& out;
};

template <class T> struct deleteFunc
{
    void operator() (T x) { delete x; };
};

// size in bytes of memory required to store a container data
template<typename T> constexpr typename T::size_type memsize(T c)
{
    return c.size() * sizeof(typename T::value_type);
}

fs::path PathExp(const fs::path& filename);
fs::path homeDir();

#endif // _CELUTIL_UTIL_H_
