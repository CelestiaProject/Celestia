// color.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "mathlib.h"
#include "color.h"


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


Color::Color(Color& color, float alpha)
{
    *this = color;
    c[Alpha] = (unsigned char) (clamp(alpha) * 255.99f);
}

