// atmosphere.cpp
//
// Copyright (C) 2001-2025, the Celestia Development Team
// Extracted from atmosphere.h, original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "atmosphere.h"

#ifndef HAVE_CONSTEXPR_CMATH
#include <cmath>

const float LogAtmosphereExtinctionThreshold = std::log(AtmosphereExtinctionThreshold);
#endif
