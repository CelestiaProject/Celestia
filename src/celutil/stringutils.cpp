// stringutils.cpp
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@shatters.net>
//
// Miscellaneous useful functions.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cctype>
#include "stringutils.h"

using namespace std;

int compareIgnoringCase(std::string_view s1, std::string_view s2)
{
    auto i1 = s1.begin();
    auto i2 = s2.begin();

    while (i1 != s1.end() && i2 != s2.end())
    {
        if (toupper(*i1) != toupper(*i2))
            return (toupper(*i1) < toupper(*i2)) ? -1 : 1;
        ++i1;
        ++i2;
    }

    return s2.size() - s1.size();
}

int compareIgnoringCase(std::string_view s1, std::string_view s2, int n)
{
    auto i1 = s1.begin();
    auto i2 = s2.begin();

    while (i1 != s1.end() && i2 != s2.end() && n > 0)
    {
        if (toupper(*i1) != toupper(*i2))
            return (toupper(*i1) < toupper(*i2)) ? -1 : 1;
        ++i1;
        ++i2;
        n--;
    }

    return n > 0 ? s2.size() - s1.size() : 0;
}

bool CompareIgnoringCasePredicate::operator()(std::string_view s1,
                                              std::string_view s2) const
{
    return compareIgnoringCase(s1, s2) < 0;
}
