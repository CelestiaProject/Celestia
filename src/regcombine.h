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

#include "texture.h"
#include "color.h"

extern void SetupCombinersBumpMap(CTexture& bumpTexture,
                                  CTexture& normalizationTexture,
                                  Color ambientColor);
void SetupCombinersSmooth(CTexture& baseTexture,
                          CTexture& normalizationTexture,
                          Color ambientColor);
extern void DisableCombiners();

#endif // _REGCOMBINE_H_
