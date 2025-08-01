// samporient.h
//
// Copyright (C) 2006, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <filesystem>
#include <memory>

namespace celestia::ephem
{

class RotationModel;

std::shared_ptr<const RotationModel> LoadSampledOrientation(const std::filesystem::path& filename);

}
