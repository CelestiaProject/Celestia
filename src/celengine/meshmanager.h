// meshmanager.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <filesystem>
#include <memory>
#include <tuple>
#include <utility>

#include <Eigen/Core>

#include <celutil/resmanager.h>
#include "geometry.h"

namespace celestia::engine
{

class GeometryInfo
{
public:
    using ResourceType = Geometry;

    // Ensure that models with different centers get resolved to different objects by
    // encoding the center, scale and normalization state in the key.
    struct ResourceKey
    {
        std::filesystem::path resolvedPath;
        Eigen::Vector3f center;
        float scale;
        bool isNormalized;
        bool resolvedToPath;

        ResourceKey(std::filesystem::path&& _resolvedPath,
                    const Eigen::Vector3f& _center,
                    float _scale,
                    bool _isNormalized,
                    bool _resolvedToPath) :
            resolvedPath(std::move(_resolvedPath)),
            center(_center),
            scale(_scale),
            isNormalized(_isNormalized),
            resolvedToPath(_resolvedToPath)
        {}
    };

    GeometryInfo(const std::filesystem::path& _source,
                 const std::filesystem::path& _path = "") :
        source(_source),
        path(_path)
    {
    }

    GeometryInfo(const std::filesystem::path& _source,
                 const std::filesystem::path& _path,
                 const Eigen::Vector3f& _center,
                 float _scale,
                 bool _isNormalized) :
        source(_source),
        path(_path),
        center(_center),
        scale(_scale),
        isNormalized(_isNormalized)
    {
    }

    ResourceKey resolve(const std::filesystem::path&) const;
    std::unique_ptr<Geometry> load(const ResourceKey&) const;

private:
    std::filesystem::path source;
    std::filesystem::path path;
    Eigen::Vector3f center{ Eigen::Vector3f::Zero() };
    float scale{ 1.0f };
    bool isNormalized{ true };

    friend bool operator<(const GeometryInfo&, const GeometryInfo&);
};

inline bool operator<(const GeometryInfo& g0, const GeometryInfo& g1)
{
    return std::tie(g0.source, g0.path, g0.isNormalized, g0.scale, g0.center.x(), g0.center.y(), g0.center.z()) <
           std::tie(g1.source, g1.path, g1.isNormalized, g1.scale, g1.center.x(), g1.center.y(), g1.center.z());
}

inline bool operator<(const GeometryInfo::ResourceKey& k0,
                      const GeometryInfo::ResourceKey& k1)
{
    // we do not use the boolean resolvedToPath field here
    return std::tie(k0.resolvedPath, k0.center.x(), k0.center.y(), k0.center.z(), k0.scale, k0.isNormalized) <
           std::tie(k1.resolvedPath, k1.center.x(), k1.center.y(), k1.center.z(), k1.scale, k1.isNormalized);
}

using GeometryManager = ResourceManager<GeometryInfo>;

GeometryManager* GetGeometryManager();

} // end namespace celestia::engine
