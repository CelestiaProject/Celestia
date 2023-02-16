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

#include <celutil/color.h>
#include <celutil/array_view.h>

class ColorTemperatureTable
{
 public:
    ColorTemperatureTable(celestia::util::array_view<const Color> _colors,
                          float maxTemp) :
        colors(_colors),
        tempScale(static_cast<float>(_colors.size() - 1) / maxTemp)
    {};

    Color lookupColor(float temp) const
    {
        auto colorTableIndex = static_cast<unsigned int>(temp * tempScale);
        if (colorTableIndex >= colors.size())
            return colors.back();
        else
            return colors[colorTableIndex];
    }

 private:
    celestia::util::array_view<const Color> colors;
    float tempScale;
};

enum class ColorTableType
{
    Enhanced,
    Blackbody_D65,
};

const ColorTemperatureTable* GetStarColorTable(ColorTableType);
