// meshmanager.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>
#include <tuple>
#include <utility>

#include <Eigen/Core>

#include <celcompat/filesystem.h>
#include <celutil/resmanager.h>
#include "geometry.h"


class GeometryInfo
{
 public:
    // Ensure that models with different centers get resolved to different objects by
    // encoding the center, scale and normalization state in the key.
    struct ResourceKey
    {
        fs::path resolvedPath;
        Eigen::Vector3f center;
        float scale;
        bool isNormalized;
        bool resolvedToPath;

        ResourceKey(fs::path&& _resolvedPath,
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

 private:
    fs::path source;
    fs::path path;
    Eigen::Vector3f center;
    float scale;
    bool isNormalized;

    friend bool operator<(const GeometryInfo&, const GeometryInfo&);

 public:
    using ResourceType = Geometry;

    GeometryInfo(const fs::path& _source,
                 const fs::path& _path = "") :
        source(_source),
        path(_path),
        center(Eigen::Vector3f::Zero()),
        scale(1.0f),
        isNormalized(true)
        {}

    GeometryInfo(const fs::path& _source,
                 const fs::path& _path,
                 const Eigen::Vector3f& _center,
                 float _scale,
                 bool _isNormalized) :
        source(_source),
        path(_path),
        center(_center),
        scale(_scale),
        isNormalized(_isNormalized)
        {}

    ResourceKey resolve(const fs::path&) const;
    std::unique_ptr<Geometry> load(const ResourceKey&) const;
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

typedef ResourceManager<GeometryInfo> GeometryManager;

extern GeometryManager* GetGeometryManager();
