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

#include <array>
#include <map>
#include <string>
#include <Eigen/Core>
#include <celmath/mathlib.h>

#define C(a) uint8_t(celmath::clamp(a) * 255.99f)

class Color
{
 public:
    constexpr Color() noexcept :
        c({ 0, 0, 0, 0xff })
    {}
    constexpr Color(float r, float g, float b, float a) noexcept :
        c({ C(r), C(g), C(b), C(a) })
    {}
    constexpr Color(float r, float g, float b) noexcept :
        Color(r, g, b, 1.0f)
    {}
    constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept :
        c({ r, g, b, a })
    {}
    constexpr Color(uint8_t r, uint8_t g, uint8_t b) noexcept:
        Color(r, g, b, 0xff)
    {}
    constexpr Color(const Color &color, float alpha) noexcept :
        Color(color.red(), color.green(), color.blue(), alpha)
    {}
    Color(const Eigen::Vector3f &v) noexcept :
        Color(v.x(), v.y(), v.z())
    {}
    Color(const Eigen::Vector4f &v) noexcept :
        Color(v.x(), v.y(), v.z(), v.w())
    {}

    enum
    {
        Red    = 0,
        Green  = 1,
        Blue   = 2,
        Alpha  = 3
    };

    inline constexpr float red() const;
    inline constexpr float green() const;
    inline constexpr float blue() const;
    inline constexpr float alpha() const;
    inline Color& alpha(float a);

    inline void get(uint8_t*) const;
    inline const uint8_t* data() const;

    inline Eigen::Vector3f toVector3() const;
    inline Eigen::Vector4f toVector4() const;
    inline operator Eigen::Vector3f() const;
    inline operator Eigen::Vector4f() const;

    inline Color operator*(float m) const;
    friend bool operator==(Color, Color);
    friend bool operator!=(Color, Color);
    friend Color operator*(Color, Color);

    static /*constexpr*/ const Color Black/* = Color(1.0f, 1.0f, 1.0f)*/;
    static /*constexpr*/ const Color White/* = Color(0.0f,0.0f, 0.0f)*/;

    static bool parse(const char*, Color&);

 private:
    static void buildX11ColorMap();

    std::array<uint8_t, 4> c;

    typedef std::map<const std::string, Color> ColorMap;
    static ColorMap x11Colors;
};

constexpr float Color::red() const
{
    return c[Red] * (1.0f / 255.0f);
}

constexpr float Color::green() const
{
    return c[Green] * (1.0f / 255.0f);
}

constexpr float Color::blue() const
{
    return c[Blue] * (1.0f / 255.0f);
}

constexpr float Color::alpha() const
{
    return c[Alpha] * (1.0f / 255.0f);
}

void Color::get(uint8_t* rgba) const
{
    rgba[0] = c[Red];
    rgba[1] = c[Green];
    rgba[2] = c[Blue];
    rgba[3] = c[Alpha];
}

const uint8_t* Color::data() const
{
    return c.data();
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
    return (a.c[0] == b.c[0] && a.c[1] == b.c[1] &&
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

inline Color::operator Eigen::Vector3f() const
{
    return toVector3();
}

inline Color::operator Eigen::Vector4f() const
{
    return toVector4();
}

inline Color Color::operator*(float m) const
{
    return { red() * m, green() * m, blue() *m };
}

inline Color& Color::alpha(float a)
{
    c[Alpha] = C(a);
    return *this;
}
#undef C

#endif // _CELUTIL_COLOR_H_
