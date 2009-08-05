// color.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELUTIL_COLOR_H_
#define _CELUTIL_COLOR_H_

#include <map>
#include <string>
#include <Eigen/Core>


class Color
{
 public:
    Color();
    Color(float, float, float);
    Color(float, float, float, float);
    Color(unsigned char, unsigned char, unsigned char);
    Color(const Color&, float);

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
    inline void get(unsigned char*) const;
    
    inline Eigen::Vector3f toVector3() const;
    inline Eigen::Vector4f toVector4() const;
 
    friend bool operator==(Color, Color);
    friend bool operator!=(Color, Color);
    friend Color operator*(Color, Color);

    static const Color Black;
    static const Color White;

    static bool parse(const char*, Color&);

 private:
    static void buildX11ColorMap();

 private:
    unsigned char c[4];

    typedef std::map<const std::string, Color> ColorMap;
    static ColorMap x11Colors;
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

void Color::get(unsigned char* rgba) const
{
    rgba[0] = c[Red];
    rgba[1] = c[Green];
    rgba[2] = c[Blue];
    rgba[3] = c[Alpha];
}

/** Return the color as a vector, with red, green, and blue in the
 *  the x, y, and z components of the vector. Each component is a
 *  floating point value between 0 and 1, inclusive.
 */
Eigen::Vector3f Color::toVector3() const
{
    return Eigen::Vector3f(red(), green(), blue());
}

/** Return the color as a vector, with red, green, blue, and alpha in the
 *  the x, y, z, and w components of the vector. Each component is a
 *  floating point value between 0 and 1, inclusive.
 */
Eigen::Vector4f Color::toVector4() const
{
    return Eigen::Vector4f(red(), green(), blue(), alpha());
}

inline bool operator==(Color a, Color b)
{
    return (a.c[0] == b.c[2] && a.c[1] == b.c[1] &&
            a.c[2] == b.c[2] && a.c[3] == b.c[3]);
}

inline bool operator!=(Color a, Color b)
{
    return !(a == b);
}

inline Color operator*(Color a, Color b)
{
    return Color(a.red() * b.red(),
                 a.green() * b.green(),
                 a.blue() * b.blue(),
                 a.alpha() * b.alpha());
}

#endif // _CELUTIL_COLOR_H_
