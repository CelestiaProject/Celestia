// color.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _COLOR_H_
#define _COLOR_H_

class Color
{
 public:
    Color();
    Color(float, float, float);
    Color(float, float, float, float);
    Color(unsigned char, unsigned char, unsigned char);
    Color(Color&, float);

    enum {
        Red    = 0,
        Green  = 1,
        Blue   = 2,
        Alpha  = 3
    };

    inline float red() const;
    inline float green() const;
    inline float blue() const;
    inline float alpha() const;

 private:
    unsigned char c[4];
};


float Color::red() const
{
    return c[Red] * (1.0f / 255.0f);
}

float Color::green() const
{
    return c[Green] * (1.0f / 255.0f);
}

float Color::blue() const
{
    return c[Blue] * (1.0f / 255.0f);
}

float Color::alpha() const
{
    return c[Alpha] * (1.0f / 255.0f);
}

#endif // _COLOR_H_

