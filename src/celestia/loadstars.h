// loadstars.h
//
// Copyright (C) 2001-2023, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>

class ProgressNotifier;
struct CelestiaConfig;

namespace celestia
{

namespace engine
{
class StarDatabase;
}

std::unique_ptr<engine::StarDatabase>
loadStars(const CelestiaConfig &config, ProgressNotifier *progressNotifier);

} // namespace celestia
