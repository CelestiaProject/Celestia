// rectangle.h
//
// Copyright (C) 2019, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <algorithm>
#include <array>
#include <celutil/color.h>

class Renderer;
class Texture;

namespace celestia
{
class Rect
{
 public:
    Rect() = delete;
    Rect(float _x, float _y, float _w, float _h) :
        x(_x), y(_y), w(_w), h(_h)
    {
    };
    void setColor(const Color &_color)
    {
        colors.fill(_color);
        hasColors = true;
    }
    void setColor(const std::array<Color,4> &_colors)
    {
        std::copy(_colors.begin(), _colors.end(), colors.begin());
        hasColors = true;
    }
    void setLineWidth(float _lw)
    {
        lw = _lw;
    }
    float x, y, w, h;
    float lw        { 1.0f };
    std::array<Color,4> colors;
    Texture *tex    { nullptr };
    bool hasColors  { false };
};

inline bool operator==(const Rect& a, const Rect& b)
{
    return (a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h &&
            a.lw == b.lw && a.tex == b.tex &&
            (a.hasColors ? (b.hasColors && a.colors == b.colors) : !b.hasColors));
}

inline bool operator!=(const Rect& a, const Rect& b)
{
    return !(a == b);
}
}
