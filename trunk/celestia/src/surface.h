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

#include "basictypes.h"
#include "color.h"
#include "reshandle.h"


class Surface
{
 public:
    Surface(Color c = Color(0.0f, 0.0f, 0.0f)) :
        appearanceFlags(0),
        color(c),
        baseTexture(InvalidResource),
        bumpTexture(InvalidResource),
        cloudTexture(InvalidResource),
        nightTexture(InvalidResource)
    {};

    // Appearance flags
    enum {
        BlendTexture         = 0x1,
        ApplyBaseTexture     = 0x2,
        ApplyBumpMap         = 0x4,
        ApplyCloudMap        = 0x10,
        ApplyNightMap        = 0x20,
    };

    uint32 appearanceFlags;
    Color color;
    ResourceHandle baseTexture;
    ResourceHandle bumpTexture;
    ResourceHandle cloudTexture;
    ResourceHandle nightTexture;
    float bumpHeight;
};

#endif // _SURFACE_H_

