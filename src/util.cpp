// util.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Miscellaneous useful functions.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "util.h"

using namespace std;


int compareIgnoringCase(const string& s1, const string& s2)
{
    string::const_iterator i1 = s1.begin();
    string::const_iterator i2 = s2.begin();

    while (i1 != s1.end() && i2 != s2.end())
    {
        if (toupper(*i1) != toupper(*i2))
            return (toupper(*i1) < toupper(*i2)) ? -1 : 1;
        ++i1;
        ++i2;
    }

    return s2.size() - s1.size();
}


int compareIgnoringCase(const string& s1, const string& s2, int n)
{
    string::const_iterator i1 = s1.begin();
    string::const_iterator i2 = s2.begin();

    while (i1 != s1.end() && i2 != s2.end() && n > 0)
    {
        if (toupper(*i1) != toupper(*i2))
            return (toupper(*i1) < toupper(*i2)) ? -1 : 1;
        ++i1;
        ++i2;
        n--;
    }

    if (n > 0)
        return s2.size() - s1.size();
    else
        return 0;
}
