// atmosphere.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _ATMOSPHERE_H_
#define _ATMOSPHERE_H_

#include "basictypes.h"
#include "color.h"


class Atmosphere
{
 public:
    Atmosphere() : height(0.0f) {};
 public:
    float height;
    Color lowerColor;
    Color upperColor;
    Color skyColor;
};

#endif // _ATMOSPHERE_H_

