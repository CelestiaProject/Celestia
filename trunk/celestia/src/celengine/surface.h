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
#include "multitexture.h"


class Surface
{
 public:
    Surface(Color c = Color(0.0f, 0.0f, 0.0f)) :
        appearanceFlags(0),
        color(c),
        specularPower(0.0f),
        baseTexture(),
        bumpTexture(),
        nightTexture(),
        overlayTexture(),
        bumpHeight(0.0f),
#ifdef USE_HDR
        nightLightRadiance(1.e-5f*.5f),
#endif
        lunarLambert(0.0f)
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
        SeparateSpecularMap  = 0x100,
        ApplyOverlay         = 0x200,
    };

    uint32 appearanceFlags;
    Color color;
    Color hazeColor;
    Color specularColor;
    float specularPower;
    MultiResTexture baseTexture;    // surface colors
    MultiResTexture bumpTexture;    // normal map based on terrain relief
    MultiResTexture nightTexture;   // artificial lights to show on night side
    MultiResTexture specularTexture;// specular mask
    MultiResTexture overlayTexture; // overlay texture, applied last
    float bumpHeight;               // scale of bump map relief
    float lunarLambert;             // mix between Lambertian and Lommel-Seeliger (lunar-like) photometric functions
#ifdef USE_HDR
    float nightLightRadiance;       // W sr^-1 m^-2
#endif
};

#endif // _SURFACE_H_

