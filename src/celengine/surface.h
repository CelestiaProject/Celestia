// surface.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _SURFACE_H_
#define _SURFACE_H_

#include <celutil/basictypes.h>
#include <celutil/color.h>
#include <celutil/reshandle.h>


class Surface
{
 public:
    Surface(Color c = Color(0.0f, 0.0f, 0.0f)) :
        appearanceFlags(0),
        color(c),
        baseTexture(InvalidResource),
        bumpTexture(InvalidResource),
        nightTexture(InvalidResource),
        specBaseTexture(InvalidResource),
        bumpHeight(0.0f)
    {};

    // Appearance flags
    enum {
        BlendTexture         = 0x1,
        ApplyBaseTexture     = 0x2,
        ApplyBumpMap         = 0x4,
        ApplyNightMap        = 0x10,
        ApplySpecularityMap  = 0x20,
        SpecularReflection   = 0x40,
        Emissive             = 0x80,
    };

    uint32 appearanceFlags;
    Color color;
    Color hazeColor;
    Color specularColor;
    float specularPower;
    ResourceHandle baseTexture;     // surface colors
    ResourceHandle bumpTexture;     // normal map based on terrain relief
    ResourceHandle nightTexture;    // artificial lights to show on night side
    ResourceHandle specBaseTexture; // base tex with specularity in alpha
    float bumpHeight;               // scale of bump map relief
};

#endif // _SURFACE_H_

