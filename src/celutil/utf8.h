// utf8.h
//
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELUTIL_UTF8_
#define _CELUTIL_UTF8_

#include <string>

#define UTF8_DEGREE_SIGN         "\302\260"
#define UTF8_MULTIPLICATION_SIGN "\303\227"


bool UTF8Decode(const std::string& str, int pos, wchar_t& ch);
bool UTF8Decode(const char* str, int pos, int length, wchar_t& ch);
int UTF8Encode(wchar_t ch, char* s);
int UTF8StringCompare(const std::string& s0, const std::string& s1);
int UTF8Length(const std::string& s);

inline int UTF8EncodedSize(wchar_t ch)
{
    if (ch < 0x80)
        return 1;
    else if (ch < 0x800)
        return 2;
    else if (ch < 0x10000)
        return 3;
    else if (ch < 0x200000)
        return 4;
    else if (ch < 0x4000000)
        return 5;
    else
        return 6;
}

inline int UTF8EncodedSizeFromFirstByte(unsigned int ch)
{
    int charlen = 1;

    if (ch < 0x80)
        charlen = 1;
    else if ((ch & 0xe0) == 0xc0)
        charlen = 2;
    else if ((ch & 0xf0) == 0xe0)
        charlen = 3;
    else if ((ch & 0xf8) == 0xf0)
        charlen = 4;
    else if ((ch & 0xfc) == 0xf8)
        charlen = 5;
    else if ((ch & 0xfe) == 0xfc)
        charlen = 6;

    return charlen;
}

#endif // _CELUTIL_UTF8_
