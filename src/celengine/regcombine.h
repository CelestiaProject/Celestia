// regcombine.h
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Some functions for setting up the nVidia register combiners
// extension for pretty rendering effects.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _REGCOMBINE_H_
#define _REGCOMBINE_H_

#include <celutil/color.h>
#include <celengine/texture.h>

extern void SetupCombinersBumpMap(Texture& bumpTexture,
                                  Texture& normalizationTexture,
                                  Color ambientColor);
extern void SetupCombinersSmooth(Texture& baseTexture,
                                 Texture& normalizationTexture,
                                 Color ambientColor,
                                 bool invert);
extern void SetupCombinersDecalAndBumpMap(Texture& bumpTexture,
                                          Color ambientColor,
                                          Color diffuseColor);
extern void SetupCombinersGlossMap(int glossMap = 0);
extern void SetupCombinersGlossMapWithFog(int glossMap = 0);
extern void DisableCombiners();

#endif // _REGCOMBINE_H_
