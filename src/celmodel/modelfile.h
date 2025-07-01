// modelfile.h
//
// Copyright (C) 2004-2010, Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <functional>
#include <iosfwd>
#include <memory>

#include <filesystem>
#include <celutil/reshandle.h>
#include "model.h"

namespace cmod
{

using HandleGetter = std::function<ResourceHandle(const std::filesystem::path&)>;
using SourceGetter = std::function<std::filesystem::path(ResourceHandle)>;

std::unique_ptr<Model> LoadModel(std::istream& in, HandleGetter getHandle);

bool SaveModelAscii(const Model* model, std::ostream& out, SourceGetter getSource);
bool SaveModelBinary(const Model* model, std::ostream& out, SourceGetter getSource);
}
