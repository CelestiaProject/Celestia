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

class Rect
{
 public:
    enum class Type
    {
        BorderOnly  = 0x0001,
        Filled      = 0x0002,
    };

    Rect() = default;
    Rect(float _x, float _y, float _w, float _h) :
        x(_x), y(_y), w(_w), h(_h)
    {
    };
    void setColor(const Color &_color)
    {
        color = _color;
        nColors = 1;
    }
    void setColor(const std::array<Color,4> _colors)
    {
        std::copy(_colors.begin(), _colors.end(), colors.begin());
        nColors = 4;
    }
    void setLineWidth(float _lw)
    {
        lw = _lw;
    }
    void setType(Type _type)
    {
        type = _type;
    }
    float x, y, w, h;
    float lw        { 1.0f };
    union
    {
        std::array<Color,4> colors;
        Color color;
    };
    Texture *tex    { nullptr };
    Type type       { Type::Filled };
    int nColors     { 0 };
};
