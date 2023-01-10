// color.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

#include <Eigen/Core>


class Color
{
 private:
    static constexpr std::uint8_t scaleFloat(float a)
    {
        return static_cast<std::uint8_t>(std::clamp(a, 0.0f, 1.0f) * 255.99f);
    }

    std::array<std::uint8_t, 4> c;

 public:
    constexpr Color() noexcept :
        c({ 0, 0, 0, 0xff })
    {}
    constexpr Color(float r, float g, float b, float a) noexcept :
        c({ scaleFloat(r), scaleFloat(g), scaleFloat(b), scaleFloat(a) })
    {}
    constexpr Color(float r, float g, float b) noexcept :
        Color(r, g, b, 1.0f)
    {}
    constexpr Color(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) noexcept :
        c({ r, g, b, a })
    {}
    constexpr Color(std::uint8_t r, std::uint8_t g, std::uint8_t b) noexcept:
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

    constexpr float red() const { return static_cast<float>(c[Red]) * (1.0f / 255.0f); }
    constexpr float green() const { return static_cast<float>(c[Green]) * (1.0f / 255.0f); }
    constexpr float blue() const { return static_cast<float>(c[Blue]) * (1.0f / 255.0f); }
    constexpr float alpha() const { return static_cast<float>(c[Alpha]) * (1.0f / 255.0f); }
    inline Color& alpha(float a)
    {
        c[Alpha] = scaleFloat(a);
        return *this;
    }

    inline void get(std::uint8_t *rgba) const
    {
        rgba[0] = c[Red];
        rgba[1] = c[Green];
        rgba[2] = c[Blue];
        rgba[3] = c[Alpha];
    }

    constexpr const std::uint8_t* data() const { return c.data(); }

    /** Return the color as a vector, with red, green, and blue in the
     *  the x, y, and z components of the vector. Each component is a
     *  floating point value between 0 and 1, inclusive.
     */
    inline Eigen::Vector3f toVector3() const { return { red(), green(), blue() }; }

    /** Return the color as a vector, with red, green, blue, and alpha in the
     *  the x, y, z, and w components of the vector. Each component is a
     *  floating point value between 0 and 1, inclusive.
     */
    inline Eigen::Vector4f toVector4() const { return { red(), green(), blue(), alpha() }; }

    constexpr Color& operator*=(float m)
    {
        c[Red] = scaleFloat(red() * m);
        c[Green] = scaleFloat(green() * m);
        c[Blue] = scaleFloat(blue() * m);
        return *this;
    }

    constexpr Color& operator*=(const Color& rhs)
    {
        c[Red] = scaleFloat(red() * rhs.red());
        c[Green] = scaleFloat(green() * rhs.green());
        c[Blue] = scaleFloat(blue() * rhs.blue());
        c[Alpha] = scaleFloat(alpha() * rhs.alpha());
        return *this;
    }

    friend constexpr bool operator==(Color, Color);

    static /*constexpr*/ const Color Black/* = Color(1.0f, 1.0f, 1.0f)*/;
    static /*constexpr*/ const Color White/* = Color(0.0f,0.0f, 0.0f)*/;

    static bool parse(std::string_view, Color&);
};

constexpr bool operator==(Color a, Color b)
{
    return (a.c[0] == b.c[0] && a.c[1] == b.c[1] &&
            a.c[2] == b.c[2] && a.c[3] == b.c[3]);
}

constexpr bool operator!=(Color a, Color b)
{
    return !(a == b);
}

constexpr Color operator*(Color a, float b)
{
    a *= b;
    return a;
}

constexpr Color operator*(float a, Color b)
{
    b *= a;
    return b;
}

constexpr Color operator*(Color a, const Color& b)
{
    a *= b;
    return a;
}
