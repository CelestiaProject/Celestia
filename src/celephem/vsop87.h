// vsop87.cpp
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>

#include "customorbittype.h"

namespace celestia::ephem
{

class Orbit;

std::unique_ptr<Orbit> CreateVSOP87Orbit(CustomOrbitType);

}
