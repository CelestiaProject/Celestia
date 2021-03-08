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

#include <iosfwd>
#include <functional>
#include <celcompat/string_view.h>

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
template<typename T> constexpr typename T::size_type memsize(const T &c)
{
    return c.size() * sizeof(typename T::value_type);
}

bool GetTZInfo(std::string_view, int&);

#endif // _CELUTIL_UTIL_H_
