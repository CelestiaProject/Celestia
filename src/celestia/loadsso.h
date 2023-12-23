// loadsso.h
//
// Copyright (C) 2001-2023, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <celengine/solarsys.h>

class ProgressNotifier;
class Universe;
struct CelestiaConfig;

namespace celestia
{

void loadSSO(const CelestiaConfig &config, ProgressNotifier *progressNotifier, Universe *universe);

} // namespace celestia
