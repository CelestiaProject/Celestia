// surface.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>

#include <celutil/color.h>
#include <celutil/texhandle.h>

struct Surface
{
    explicit Surface(Color c = Color{}) :
        color(c)
    {}

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

    std::uint32_t appearanceFlags{ 0 };
    Color color;
    Color specularColor;
    float specularPower{ 0.0f };
    celestia::util::TextureHandle baseTexture{ celestia::util::TextureHandle::Invalid };    // surface colors
    celestia::util::TextureHandle bumpTexture{ celestia::util::TextureHandle::Invalid };    // normal map based on terrain relief
    celestia::util::TextureHandle nightTexture{ celestia::util::TextureHandle::Invalid };   // artificial lights to show on night side
    celestia::util::TextureHandle specularTexture{ celestia::util::TextureHandle::Invalid };// specular mask
    celestia::util::TextureHandle overlayTexture{ celestia::util::TextureHandle::Invalid }; // overlay texture, applied last
    float bumpHeight{ 0.0f };       // scale of bump map relief
    float lunarLambert{ 0.0f };     // mix between Lambertian and Lommel-Seeliger (lunar-like) photometric functions
};
