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

namespace celestia::ephem
{

class Orbit;

std::shared_ptr<const Orbit> CreateVSOP87MercuryOrbit();
std::shared_ptr<const Orbit> CreateVSOP87VenusOrbit();
std::shared_ptr<const Orbit> CreateVSOP87EarthOrbit();
std::shared_ptr<const Orbit> CreateVSOP87MarsOrbit();
std::shared_ptr<const Orbit> CreateVSOP87JupiterOrbit();
std::shared_ptr<const Orbit> CreateVSOP87SaturnOrbit();
std::shared_ptr<const Orbit> CreateVSOP87UranusOrbit();
std::shared_ptr<const Orbit> CreateVSOP87NeptuneOrbit();
std::shared_ptr<const Orbit> CreateVSOP87SunOrbit();

}
