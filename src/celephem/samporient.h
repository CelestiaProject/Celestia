// samporient.h
//
// Copyright (C) 2006, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>

#include <celcompat/filesystem.h>

namespace celestia::ephem
{

class RotationModel;

std::unique_ptr<RotationModel> LoadSampledOrientation(const fs::path& filename);

}
