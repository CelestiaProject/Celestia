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

#include <cmath>
#include <cstddef>
#include <vector>

#include <Eigen/Core>

#include <celmath/vecgl.h>
#include <celutil/color.h>

enum class ColorTableType
{
    Enhanced = 0,
    Blackbody_D65 = 1,
    SunWhite = 2,
    VegaWhite = 3,
};

class ColorTemperatureTable
{
 public:
    explicit ColorTemperatureTable(ColorTableType _type);

    Color lookupColor(float temp) const
    {
        auto colorTableIndex = static_cast<std::size_t>(std::nearbyint(temp * tempScale));
        if (colorTableIndex >= colors.size())
            return colors.back();
        else
            return colors[colorTableIndex];
    }

    Color lookupTintColor(float temp, float saturation, float fadeFactor) const
    {
        Eigen::Vector3f color = celestia::math::mix(Eigen::Vector3f::Ones(),
                                                    lookupColor(temp).toVector3(),
                                                    saturation) * fadeFactor;
        return Color(color);
    }

    ColorTableType type() const
    {
        return tableType;
    }

    bool setType(ColorTableType _type);

 private:
    std::vector<Color> colors{ };
    float tempScale;
    ColorTableType tableType;
};
