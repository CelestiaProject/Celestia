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

int compareIgnoringCase(std::string_view s1, std::string_view s2)
{
    auto i1 = s1.begin();
    auto i2 = s2.begin();

    for (;;)
    {
        if (i1 == s1.end()) { return i2 == s2.end() ? 0 : -1; }
        if (i2 == s2.end()) { return 1; }
        auto c1 = std::toupper(static_cast<unsigned char>(*i1));
        auto c2 = std::toupper(static_cast<unsigned char>(*i2));
        if (c1 != c2) { return c1 < c2 ? -1 : 1; }
        ++i1;
        ++i2;
    }
}

int compareIgnoringCase(std::string_view s1,
                        std::string_view s2,
                        std::string_view::size_type n)
{
    auto i1 = s1.begin();
    auto i2 = s2.begin();

    for (;;)
    {
        if (n-- == 0) { return 0; }
        if (i1 == s1.end()) { return i2 == s2.end() ? 0 : -1; }
        if (i2 == s2.end()) { return 1; }
        auto c1 = std::toupper(static_cast<unsigned char>(*i1));
        auto c2 = std::toupper(static_cast<unsigned char>(*i2));
        if (c1 != c2) { return c1 < c2 ? -1 : 1; }
        ++i1;
        ++i2;
    }
}

bool CompareIgnoringCasePredicate::operator()(std::string_view s1,
                                              std::string_view s2) const
{
    return compareIgnoringCase(s1, s2) < 0;
}
