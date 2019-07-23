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
#ifdef _WIN32
#include <Shlobj.h>
#else
#include <wordexp.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

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

    return n > 0 ? s2.size() - s1.size() : 0;
}


bool CompareIgnoringCasePredicate::operator()(const string& s1,
                                              const string& s2) const
{
    return compareIgnoringCase(s1, s2) < 0;
}


string LocaleFilename(const string & filename)
{
    string localeFilename;
    struct stat filestat;
    string::size_type pos;

    if ((pos = filename.rfind('.')) != string::npos)
    {
        localeFilename = filename.substr(0, pos) + '_' + _("LANGUAGE") + filename.substr(pos);
    }
    else
    {
        localeFilename = filename + '_' + _("LANGUAGE");
    }

    if (stat(localeFilename.c_str(), &filestat) != 0)
    {
        localeFilename = string("locale/") + localeFilename;
        if (stat(localeFilename.c_str(), &filestat) != 0)
        {
            localeFilename = filename;
        }
    }

    return localeFilename;
}


string WordExp(const std::string& filename)
{
#ifdef _WIN32
    if (filename[0] == '~' && filename[1] == '/')
    {
        char path[MAX_PATH + 1];
        if (SHGetSpecialFolderPathA(HWND_DESKTOP, path, CSIDL_DESKTOPDIRECTORY, FALSE))
            return string(path) + filename.substr(1);
    }
    return filename;
#else
    wordexp_t result;

    switch(wordexp(filename.c_str(), &result, WRDE_NOCMD))
    {
    case 0: // successful
        break;
    case WRDE_NOSPACE:
            // If the error was `WRDE_NOSPACE',
            // then perhaps part of the result was allocated.
        wordfree(&result);
    default: // some other error
        return filename;
    }

    if (result.we_wordc != 1)
    {
        wordfree(&result);
        return filename;
    }

    string expanded(result.we_wordv[0]);
    wordfree(&result);
    return expanded;
#endif
}
