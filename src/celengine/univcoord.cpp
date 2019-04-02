// univcoord.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include "univcoord.h"


UniversalCoord operator+(const UniversalCoord& uc0, const UniversalCoord& uc1)
{
    return UniversalCoord(uc0.x + uc1.x, uc0.y + uc1.y, uc0.z + uc1.z);
}

UniversalCoord UniversalCoord::difference(const UniversalCoord& uc) const
{
    return UniversalCoord(x - uc.x, y - uc.y, z - uc.z);
}
