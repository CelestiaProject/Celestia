// customrotation.h
//
// Custom rotation models for Solar System bodies.
//
// Copyright (C) 2008, the Celestia Development Team
// Initial version by Chris Laurel, claurel@gmail.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string_view>

class RotationModel;

namespace celestia::ephem
{

RotationModel* GetCustomRotationModel(std::string_view name);

}
