// utf8.cpp
//
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "utf8.h"


// Decode the UTF-8 characters in string str beginning at position pos.
// The decoded character is returned in ch; the return value of the function
// is true if a valid UTF-8 sequence was successfully decoded.
bool UTF8Decode(const std::string& str, int pos, wchar_t& ch)
{
    unsigned int c0 = (unsigned int) str[pos];
    int charlen = UTF8EncodedSizeFromFirstByte(c0);

    // Bad UTF-8 character that extends past end of string
    if (pos + charlen > str.length())
        return false;

    // TODO: Should check that the bytes of characters after the first are all
    // of the form 01xxxxxx
    // TODO: Need to reject overlong encoding sequences

    switch (charlen)
    {
    case 1:
        ch = c0;
        return true;

    case 2:
        ch = ((c0 & 0x1f) << 6) | ((unsigned int) str[pos + 1] & 0x3f);
        return true;

    case 3:
        ch = ((c0 & 0x0f) << 12) |
            (((unsigned int) str[pos + 1] & 0x3f) << 6) |
            ((unsigned int)  str[pos + 2] & 0x3f);
        return true;

    case 4:
        ch = ((c0 & 0x07) << 18) |
            (((unsigned int) str[pos + 1] & 0x3f) << 12) |
            (((unsigned int) str[pos + 2] & 0x3f) << 6)  |
            ((unsigned int)  str[pos + 3] & 0x3f);
        return true;

    case 5:
        ch = ((c0 & 0x03) << 24) |
            (((unsigned int) str[pos + 1] & 0x3f) << 18) |
            (((unsigned int) str[pos + 2] & 0x3f) << 12) |
            (((unsigned int) str[pos + 3] & 0x3f) << 6)  |
            ((unsigned int)  str[pos + 4] & 0x3f);
        return true;

    case 6:
        ch = ((c0 & 0x01) << 30) |
            (((unsigned int) str[pos + 1] & 0x3f) << 24) |
            (((unsigned int) str[pos + 2] & 0x3f) << 18) |
            (((unsigned int) str[pos + 3] & 0x3f) << 12) |
            (((unsigned int) str[pos + 4] & 0x3f) << 6)  |
            ((unsigned int)  str[pos + 5] & 0x3f);
        return true;

    default:
        return false;
    }
}


// Decode the UTF-8 characters in string str beginning at position pos.
// The decoded character is returned in ch; the return value of the function
// is true if a valid UTF-8 sequence was successfully decoded.
bool UTF8Decode(const char* str, int pos, int length, wchar_t& ch)
{
    unsigned int c0 = (unsigned int) str[pos];
    int charlen = UTF8EncodedSizeFromFirstByte(c0);

    // Bad UTF-8 character that extends past end of string
    if (pos + charlen > length)
        return false;

    // TODO: Should check that the bytes of characters after the first are all
    // of the form 01xxxxxx
    // TODO: Need to reject overlong encoding sequences

    switch (charlen)
    {
    case 1:
        ch = c0;
        return true;

    case 2:
        ch = ((c0 & 0x1f) << 6) | ((unsigned int) str[pos + 1] & 0x3f);
        return true;

    case 3:
        ch = ((c0 & 0x0f) << 12) |
            (((unsigned int) str[pos + 1] & 0x3f) << 6) |
            ((unsigned int)  str[pos + 2] & 0x3f);
        return true;

    case 4:
        ch = ((c0 & 0x07) << 18) |
            (((unsigned int) str[pos + 1] & 0x3f) << 12) |
            (((unsigned int) str[pos + 2] & 0x3f) << 6)  |
            ((unsigned int)  str[pos + 3] & 0x3f);
        return true;

    case 5:
        ch = ((c0 & 0x03) << 24) |
            (((unsigned int) str[pos + 1] & 0x3f) << 18) |
            (((unsigned int) str[pos + 2] & 0x3f) << 12) |
            (((unsigned int) str[pos + 3] & 0x3f) << 6)  |
            ((unsigned int)  str[pos + 4] & 0x3f);
        return true;

    case 6:
        ch = ((c0 & 0x01) << 30) |
            (((unsigned int) str[pos + 1] & 0x3f) << 24) |
            (((unsigned int) str[pos + 2] & 0x3f) << 18) |
            (((unsigned int) str[pos + 3] & 0x3f) << 12) |
            (((unsigned int) str[pos + 4] & 0x3f) << 6)  |
            ((unsigned int)  str[pos + 5] & 0x3f);
        return true;

    default:
        return false;
    }
}


// UTF-8 encode the Unicode character ch into the string s and return the
// encoded length.  There should be space for at least 7 characters in s--up
// to six encoded bytes, plus one byte for the terminating null character.
int UTF8Encode(wchar_t ch, char* s)
{
    if (ch < 0x80)
    {
        s[0] = (char) ch;
        s[1] = '\0';
        return 1;
    }
    else if (ch < 0x800)
    {
        s[0] = (char) (0xc0 | ((ch & 0x7c0) >> 6));
        s[1] = (char) (0x80 | (ch & 0x3f));
        s[2] = '\0';
        return 2;
    }
    else if (ch < 0x10000)
    {
        s[0] = (char) (0xe0 | ((ch & 0xf000) >> 12));
        s[1] = (char) (0x80 | ((ch & 0x0fc0) >> 6));
        s[2] = (char) (0x80 | ((ch & 0x003f)));
        s[3] = '\0';
        return 3;
    }
    else if (ch < 0x200000)
    {
        s[0] = (char) (0xf0 | ((ch & 0x1c0000) >> 18));
        s[1] = (char) (0x80 | ((ch & 0x03f000) >> 12));
        s[2] = (char) (0x80 | ((ch & 0x000fc0) >>  6));
        s[3] = (char) (0x80 | ((ch & 0x00003f)));
        s[4] = '\0';
        return 4;
    }
    else if (ch < 0x4000000)
    {
        s[0] = (char) (0xf8 | ((ch & 0x3000000) >> 24));
        s[1] = (char) (0x80 | ((ch & 0x0fc0000) >> 18));
        s[2] = (char) (0x80 | ((ch & 0x003f000) >> 12));
        s[3] = (char) (0x80 | ((ch & 0x0000fc0) >>  6));
        s[4] = (char) (0x80 | ((ch & 0x000003f)));
        s[5] = '\0';
        return 5;
    }
    else
    {
        s[0] = (char) (0xfc | ((ch & 0x40000000) >> 30));
        s[1] = (char) (0x80 | ((ch & 0x3f000000) >> 24));
        s[2] = (char) (0x80 | ((ch & 0x00fc0000) >> 18));
        s[3] = (char) (0x80 | ((ch & 0x0003f000) >> 12));
        s[4] = (char) (0x80 | ((ch & 0x00000fc0) >>  6));
        s[5] = (char) (0x80 | ((ch & 0x0000003f)));
        s[6] = '\0';
        return 6;
    }
}


// Currently incomplete, but could be a helpful class for dealing with
// UTF-8 streams
class UTF8StringIterator
{
public:
    UTF8StringIterator(const std::string& _str);
    UTF8StringIterator(const UTF8StringIterator& iter);

    UTF8StringIterator& operator++();
    UTF8StringIterator& operator++(int);

private:
    const std::string& str;
    int position;
};


UTF8StringIterator::UTF8StringIterator(const std::string& _str) :
    str(_str),
    position(0)
{
}


UTF8StringIterator::UTF8StringIterator(const UTF8StringIterator& iter) :
    str(iter.str),
    position(iter.position)
{
}


UTF8StringIterator& UTF8StringIterator::operator++()
{
    return *this;
}


UTF8StringIterator& UTF8StringIterator::operator++(int)
{
    return *this;
}
