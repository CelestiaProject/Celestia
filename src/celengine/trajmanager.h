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
#include <functional>
#include <memory>
#include <unordered_map>

#include <celcompat/filesystem.h>
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

    std::shared_ptr<const ephem::Orbit> find(const fs::path& source,
                                             const fs::path& path,
                                             ephem::TrajectoryInterpolation interpolation,
                                             ephem::TrajectoryPrecision precision);

private:
    struct Key
    {
        fs::path path;
        ephem::TrajectoryInterpolation interpolation;
        ephem::TrajectoryPrecision precision;

        friend bool operator==(const Key& lhs, const Key& rhs) noexcept
        {
            return lhs.path == rhs.path && lhs.interpolation == rhs.interpolation && lhs.precision == rhs.precision;
        }

        friend bool operator!=(const Key& lhs, const Key& rhs) noexcept
        {
            return !(lhs == rhs);
        }
    };

    struct KeyHasher
    {
        std::size_t operator()(const Key& key) const noexcept
        {
            // Only support 32-bit and 64-bit size_t for now
            static_assert(sizeof(std::size_t) == sizeof(std::uint32_t) || sizeof(std::size_t) == sizeof(std::uint64_t));

            // Based on documentation of boost::hash_combine
            // Prevent Sonar complaining about platform-dependent casts that happen to be redundant on the system it runs on
            constexpr std::size_t phi = sizeof(std::size_t) == sizeof(std::uint32_t)
                ? static_cast<std::size_t>(0x9e3779b9) //NOSONAR
                : static_cast<std::size_t>(0x9e3779b97f4a7c15); //NOSONAR

            auto seed = fs::hash_value(key.path);
            seed ^= std::hash<ephem::TrajectoryInterpolation>{}(key.interpolation) + phi + (seed << 6) + (seed >> 2);
            seed ^= std::hash<ephem::TrajectoryPrecision>{}(key.precision) + phi + (seed << 6) + (seed >> 2);
            return seed;
        }
    };

    std::unordered_map<Key, std::weak_ptr<const ephem::Orbit>, KeyHasher> orbits;
};

TrajectoryManager* GetTrajectoryManager();

} // end namespace celestia::engine
