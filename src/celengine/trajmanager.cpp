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

#include <boost/container_hash/hash.hpp>

#include <celephem/samporbit.h>

namespace celestia::engine
{

std::size_t
TrajectoryManager::KeyHasher::operator()(const Key& key) const
{
    std::size_t seed = 0;
    boost::hash_combine(seed, std::filesystem::hash_value(key.path));
    boost::hash_combine(seed, key.interpolation);
    boost::hash_combine(seed, key.precision);
    return seed;
}

std::shared_ptr<const ephem::Orbit>
TrajectoryManager::find(const std::filesystem::path& source,
                        const std::filesystem::path& path,
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
