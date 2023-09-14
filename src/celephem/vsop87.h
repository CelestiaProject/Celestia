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

std::unique_ptr<Orbit> CreateVSOP87MercuryOrbit();
std::unique_ptr<Orbit> CreateVSOP87VenusOrbit();
std::unique_ptr<Orbit> CreateVSOP87EarthOrbit();
std::unique_ptr<Orbit> CreateVSOP87MarsOrbit();
std::unique_ptr<Orbit> CreateVSOP87JupiterOrbit();
std::unique_ptr<Orbit> CreateVSOP87SaturnOrbit();
std::unique_ptr<Orbit> CreateVSOP87UranusOrbit();
std::unique_ptr<Orbit> CreateVSOP87NeptuneOrbit();
std::unique_ptr<Orbit> CreateVSOP87SunOrbit();

}
