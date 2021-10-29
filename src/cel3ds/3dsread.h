// 3dsread.h
//
// Copyright (C) 2000, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <iosfwd>
#include <memory>
#include <celcompat/filesystem.h>

class M3DScene;

std::unique_ptr<M3DScene> Read3DSFile(std::istream& in);
std::unique_ptr<M3DScene> Read3DSFile(const fs::path& filename);
