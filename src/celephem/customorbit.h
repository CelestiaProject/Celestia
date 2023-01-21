// customorbit.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>
#include <string_view>

namespace celestia::ephem
{

class Orbit;

std::unique_ptr<Orbit> GetCustomOrbit(std::string_view name);

}
