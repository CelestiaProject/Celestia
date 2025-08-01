// scriptorbit.h
//
// Copyright (C) 2006, Chris Laurel <claurel@shatters.net>
//
// Interface for a Celestia trajectory implemented via a Lua script.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <filesystem>
#include <memory>
#include <string>

namespace celestia
{

namespace util
{
class AssociativeArray;
}

namespace ephem
{

class Orbit;

std::shared_ptr<const Orbit>
CreateScriptedOrbit(const std::string* moduleName,
                    const std::string& funcName,
                    const util::AssociativeArray& parameters,
                    const std::filesystem::path& path);

} // end namespace celestia::ephem
} // end namespace celestia
