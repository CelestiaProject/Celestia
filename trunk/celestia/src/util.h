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

#ifndef _UTIL_H_
#define _UTIL_H_

#include <string>
#include <iostream>
#include <functional>

extern int compareIgnoringCase(const std::string& s1, const std::string& s2);

template <class T> struct printlineFunc : public std::unary_function<T, void>
{
    printlineFunc(std::ostream& o) : out(o) {};
    void operator() (T x) { out << x << '\n'; };
    std::ostream& out;
};

#endif // _UTIL_H_
