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

#include <string>
#include "basictypes.h"
#include "color.h"


class Surface
{
 public:
    Surface() : appearanceFlags(0), color(0.0f, 0.0f, 0.0f) {};
    Surface(Color c) : appearanceFlags(0), color(c) {};

    // Appearance flags
    enum {
        BlendTexture      = 0x1,
        ApplyBaseTexture  = 0x2,
        ApplyBumpMap      = 0x4,
    };

    Color color;
    std::string baseTexture;
    std::string bumpTexture;
    uint32 appearanceFlags;
};

#endif // _SURFACE_H_

