// starcolors.cpp
//
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//
// Tables of star colors, indexed by temperature.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <celmath/vecgl.h>
#include <celutil/color.h>
#include <celutil/array_view.h>

enum class ColorTableType
{
    Enhanced,
    Blackbody_D65,
};

class ColorTemperatureTable
{
 public:
    ColorTemperatureTable(celestia::util::array_view<const Color> _colors,
                          float maxTemp,
                          ColorTableType _type) :
        colors(_colors),
        tempScale(static_cast<float>(_colors.size() - 1) / maxTemp),
        tableType(_type)
    {};

    Color lookupColor(float temp) const
    {
        auto colorTableIndex = static_cast<unsigned int>(temp * tempScale);
        if (colorTableIndex >= colors.size())
            return colors.back();
        else
            return colors[colorTableIndex];
    }

    Color lookupTintColor(float temp, float saturation, float fadeFactor) const
    {
        Eigen::Vector3f color = celmath::mix(Eigen::Vector3f::Ones(),
                                             lookupColor(temp).toVector3(),
                                             saturation) * fadeFactor;
        return Color(color);
    }

    ColorTableType type() const
    {
        return tableType;
    }

 private:
    celestia::util::array_view<const Color> colors;
    float tempScale;
    ColorTableType tableType;
};

const ColorTemperatureTable* GetStarColorTable(ColorTableType);
const ColorTemperatureTable* GetTintColorTable();
