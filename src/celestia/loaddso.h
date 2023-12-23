// loaddso.h
//
// Copyright (C) 2001-2023, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>

class DSODatabase;
class ProgressNotifier;
struct CelestiaConfig;

namespace celestia
{

std::unique_ptr<DSODatabase> loadDSO(const CelestiaConfig &config,
                                     ProgressNotifier     *progressNotifier);

} // namespace celestia
