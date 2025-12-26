// solarsys.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <map>
#include <memory>

#include <Eigen/Core>

class FrameTree;
class PlanetarySystem;
class Star;
class Universe;

namespace celestia::engine
{
class GeometryPaths;
class TexturePaths;
}

class SolarSystem
{
public:
    SolarSystem(Star*);
    ~SolarSystem();

    Star* getStar() const;
    Eigen::Vector3f getCenter() const;
    PlanetarySystem* getPlanets() const;
    FrameTree* getFrameTree() const;

private:
    Star* star;
    std::unique_ptr<PlanetarySystem> planets;
    std::unique_ptr<FrameTree> frameTree;
};

using SolarSystemCatalog = std::map<std::uint32_t, std::unique_ptr<SolarSystem>>;

bool LoadSolarSystemObjects(std::istream& in,
                            Universe& universe,
                            const std::filesystem::path& dir,
                            celestia::engine::GeometryPaths& geometryPaths,
                            celestia::engine::TexturePaths& texturePaths);
