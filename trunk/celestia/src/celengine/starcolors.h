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

#ifndef _CELENGINE_STARCOLORS_H_
#define _CELENGINE_STARCOLORS_H_

#include <celutil/color.h>


class ColorTemperatureTable
{
 public:
    ColorTemperatureTable(Color* _colors,
                          unsigned int _nColors,
                          float maxTemp) :
        colors(_colors),
        nColors(_nColors),
        tempScale((float) (_nColors - 1) / maxTemp)
    {};

    Color lookupColor(float temp) const
    {
        unsigned int colorTableIndex = (unsigned int) (temp * tempScale);
        if (colorTableIndex >= nColors)
            return colors[nColors - 1];
        else
            return colors[colorTableIndex];
    }

 private:
    const Color* colors;
    unsigned nColors;
    float tempScale;
};

enum ColorTableType
{
    ColorTable_Enhanced,
    ColorTable_Blackbody_D65,
};

extern ColorTemperatureTable* GetStarColorTable(ColorTableType);


#endif // _CELENGINE_STARCOLORS_H_
