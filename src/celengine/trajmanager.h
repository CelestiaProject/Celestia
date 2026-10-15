// trajmanager.h
//
// Copyright (C) 2001-2007 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <unordered_map>

#include <celephem/orbit.h>
#include <celephem/samporbit.h>

namespace celestia::engine
{

class TrajectoryManager
{
public:
    TrajectoryManager() = default;
    ~TrajectoryManager() = default;

    TrajectoryManager(const TrajectoryManager&) = delete;
    TrajectoryManager& operator=(const TrajectoryManager&) = delete;

    std::shared_ptr<const ephem::Orbit> find(const std::filesystem::path& source,
                                             const std::filesystem::path& path,
                                             ephem::TrajectoryInterpolation interpolation,
                                             ephem::TrajectoryPrecision precision);

private:
    struct Key
    {
        std::filesystem::path path;
        ephem::TrajectoryInterpolation interpolation;
        ephem::TrajectoryPrecision precision;

        bool operator==(const Key&) const noexcept = default;
    };

    struct KeyHasher
    {
        std::size_t operator()(const Key& key) const;
    };

    std::unordered_map<Key, std::weak_ptr<const ephem::Orbit>, KeyHasher> orbits;
};

TrajectoryManager* GetTrajectoryManager();

} // end namespace celestia::engine
