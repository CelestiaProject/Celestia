// trajmanager.h
//
// Copyright (C) 2001-2007 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>
#include <string>
#include <tuple>
#include <utility>

#include <celcompat/filesystem.h>
#include <celephem/orbit.h>
#include <celephem/samporbit.h>
#include <celutil/resmanager.h>


class TrajectoryInfo
{
 public:
    // Ensure that trajectories with different interpolation or precision get resolved to different objects by
    // storing the interpolation and precision in the resource key.
    struct ResourceKey
    {
        fs::path resolvedPath;
        celestia::ephem::TrajectoryInterpolation interpolation;
        celestia::ephem::TrajectoryPrecision precision;

        ResourceKey(fs::path&& _resolvedPath,
                    celestia::ephem::TrajectoryInterpolation _interpolation,
                    celestia::ephem::TrajectoryPrecision _precision) :
            resolvedPath(std::move(_resolvedPath)),
            interpolation(_interpolation),
            precision(_precision)
        {}
    };

 private:
    std::string source;
    fs::path path;
    celestia::ephem::TrajectoryInterpolation interpolation;
    celestia::ephem::TrajectoryPrecision precision;

    friend bool operator<(const TrajectoryInfo&, const TrajectoryInfo&);

 public:
    using ResourceType = celestia::ephem::Orbit;

    TrajectoryInfo(const std::string& _source,
                   const fs::path& _path = "",
                   celestia::ephem::TrajectoryInterpolation _interpolation = celestia::ephem::TrajectoryInterpolation::Cubic,
                   celestia::ephem::TrajectoryPrecision _precision = celestia::ephem::TrajectoryPrecision::Single) :
        source(_source), path(_path), interpolation(_interpolation), precision(_precision) {}

    ResourceKey resolve(const fs::path&) const;
    std::unique_ptr<celestia::ephem::Orbit> load(const ResourceKey&) const;
};

// Sort trajectory info records. The same trajectory can be loaded multiple times with
// different attributes for precision and interpolation. How the ordering is defined isn't
// as important making this operator distinguish between trajectories with either different
// sources or different attributes.
inline bool operator<(const TrajectoryInfo& ti0, const TrajectoryInfo& ti1)
{
    return std::tie(ti0.interpolation, ti0.precision, ti0.source, ti0.path) <
           std::tie(ti1.interpolation, ti1.precision, ti1.source, ti1.path);
}

inline bool operator<(const TrajectoryInfo::ResourceKey& k0,
                      const TrajectoryInfo::ResourceKey& k1)
{
    return std::tie(k0.resolvedPath, k0.interpolation, k0.precision) <
           std::tie(k1.resolvedPath, k1.interpolation, k1.precision);
}

using TrajectoryManager = ResourceManager<TrajectoryInfo>;

TrajectoryManager* GetTrajectoryManager();
