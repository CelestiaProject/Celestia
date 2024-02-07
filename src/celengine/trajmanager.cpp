// trajmanager.cpp
//
// Copyright (C) 2001-2008 Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "trajmanager.h"

#include <utility>

#include <celephem/samporbit.h>

namespace celestia::engine
{

std::shared_ptr<const ephem::Orbit>
TrajectoryManager::find(const fs::path& source,
                        const fs::path& path,
                        ephem::TrajectoryInterpolation interpolation,
                        ephem::TrajectoryPrecision precision)
{
    auto filename = path.empty()
        ? "data" / source
        : path / "data" / source;

    auto it = orbits.try_emplace(Key { std::move(filename), interpolation, precision }).first;
    if (auto cachedOrbit = it->second.lock(); cachedOrbit != nullptr)
        return cachedOrbit;

    auto orbit = ephem::LoadSampledTrajectory(it->first.path, interpolation, precision);
    if (orbit == nullptr)
    {
        orbits.erase(it);
        return nullptr;
    }

    it->second = orbit;
    return orbit;
}

TrajectoryManager*
GetTrajectoryManager()
{
    static TrajectoryManager* const manager = std::make_unique<TrajectoryManager>().release(); //NOSONAR
    return manager;
}

} // end namespace celestia::engine
