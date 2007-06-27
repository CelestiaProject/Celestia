// color.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstdio>
#include <cstring>
#include <cctype>
#include "color.h"

const Color Color::White = Color(1.0f, 1.0f, 1.0f);
const Color Color::Black = Color(0.0f, 0.0f, 0.0f);

template<class T> T clamp(T x)
{
    if (x < 0)
        return 0;
    else if (x > 1)
        return 1;
    else
        return x;
}

Color::Color()
{
    c[Red] = c[Green] = c[Blue] = 0;
    c[Alpha] = 0xff;
}


Color::Color(float r, float g, float b)
{
    c[Red] = (unsigned char) (clamp(r) * 255.99f);
    c[Green] = (unsigned char) (clamp(g) * 255.99f);
    c[Blue] = (unsigned char) (clamp(b) * 255.99f);
    c[Alpha] = 0xff;
}


Color::Color(float r, float g, float b, float a)
{
    c[Red]   = (unsigned char) (clamp(r) * 255.99f);
    c[Green] = (unsigned char) (clamp(g) * 255.99f);
    c[Blue]  = (unsigned char) (clamp(b) * 255.99f);
    c[Alpha] = (unsigned char) (clamp(a) * 255.99f);
}


Color::Color(unsigned char r, unsigned char g, unsigned char b)
{
    c[Red] = r;
    c[Green] = g;
    c[Blue] = b;
    c[Alpha] = 0xff;
}


Color::Color(const Color& color, float alpha)
{
    *this = color;
    c[Alpha] = (unsigned char) (clamp(alpha) * 255.99f);
}


// Parse a color string and return true if it was a valid color, otherwise
// false.  Accetable inputs are HTML/X11 style #xxxxxx colors (where x is 
// hexadecimal digit) or one of a list of named colors.
bool Color::parse(const char* s, Color& c)
{
    if (s[0] == '#')
    {
        s++;

        int length = strlen(s);

        // Verify that the string contains only hex digits
        for (int i = 0; i < length; i++)
        {
            if (!isxdigit(s[i]))
                return false;
        }
        
        unsigned int n;
        sscanf(s, "%x", &n);
        if (length == 3)
        {
            c = Color((unsigned char) ((n >> 8) * 17),
                      (unsigned char) (((n & 0x0f0) >> 4) * 17),
                      (unsigned char) ((n & 0x00f) * 17));
            return true;
        }
        else if (length == 6)
        {
            c = Color((unsigned char) (n >> 16),
                      (unsigned char) ((n & 0x00ff00) >> 8),
                      (unsigned char) (n & 0x0000ff));
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        // TODO: replace this sequence of strcmps with an STL map
        if (!strcmp(s, "red"))
            c = Color(1.0f, 0.0f, 0.0f);
        else if (!strcmp(s, "green"))
            c = Color(0.0f, 1.0f, 0.0f);
        else if (!strcmp(s, "blue"))
            c = Color(0.0f, 0.0f, 1.0f);
        else if (!strcmp(s, "cyan"))
            c = Color(0.0f, 1.0f, 1.0f);
        else if (!strcmp(s, "magenta"))
            c = Color(1.0f, 0.0f, 1.0f);
        else if (!strcmp(s, "yellow"))
            c = Color(1.0f, 1.0f, 0.0f);
        else if (!strcmp(s, "black"))
            c = Color(0.0f, 0.0f, 0.0f);
        else if (!strcmp(s, "white"))
            c = Color(1.0f, 1.0f, 1.0f);
        else
            return false;
        
        return true;
    }
}
