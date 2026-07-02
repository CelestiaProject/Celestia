// tass17.cpp
//
// Copyright (C) 2026-present, Celestia Development Team
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

std::shared_ptr<const Orbit> CreateTASS17MimasOrbit();
std::shared_ptr<const Orbit> CreateTASS17EnceladusOrbit();
std::shared_ptr<const Orbit> CreateTASS17TethysOrbit();
std::shared_ptr<const Orbit> CreateTASS17DioneOrbit();
std::shared_ptr<const Orbit> CreateTASS17RheaOrbit();
std::shared_ptr<const Orbit> CreateTASS17TitanOrbit();
std::shared_ptr<const Orbit> CreateTASS17HyperionOrbit();
std::shared_ptr<const Orbit> CreateTASS17IapetusOrbit();

}
