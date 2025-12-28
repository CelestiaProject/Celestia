// meshmanager.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <vector>

#include <Eigen/Core>

#include <celutil/classops.h>
#include <celutil/fsutils.h>
#include "geometry.h"

namespace celestia::engine
{

enum class GeometryHandle : std::uint32_t
{
    Invalid = ~UINT32_C(0),
    Empty = Invalid - UINT32_C(1),
};

struct GeometryInfo
{
    std::filesystem::path path;
    std::filesystem::path directory;
    Eigen::Vector3f center{ Eigen::Vector3f::Zero() };
    bool isNormalized{ true };
};

class GeometryPaths : private util::NoCopy
{
public:
    GeometryHandle getHandle(const std::filesystem::path& filename,
                             const std::filesystem::path& directory,
                             const Eigen::Vector3f& center = Eigen::Vector3f::Zero(),
                             bool isNormalized = true);
    bool getInfo(GeometryHandle, GeometryInfo&) const;

private:
    enum class PathIndex : std::uint32_t
    {
        Root = 0,
        Invalid = ~UINT32_C(0),
    };

    struct Info
    {
        Info(PathIndex, PathIndex, const Eigen::Vector3f&, bool);

        PathIndex pathIndex;
        PathIndex directoryPathIndex;
        Eigen::Vector3f center;
        bool isNormalized;
    };

    struct Key
    {
        Key(PathIndex, const Eigen::Vector3f&, bool);

        PathIndex pathIndex;
        Eigen::Vector3f center;
        bool isNormalized;
    };

    struct KeyHash
    {
        std::size_t operator()(const Key&) const;
    };

    struct KeyEqual
    {
        bool operator()(const Key&, const Key&) const;
    };

    using DirectoryPaths = std::unordered_map<std::filesystem::path, PathIndex, util::PathHasher>;

    PathIndex getFileIndex(PathIndex&, const std::filesystem::path&);
    bool checkPath(PathIndex, const std::filesystem::path&, PathIndex&);
    PathIndex getPathIndex(const std::filesystem::path&);

    std::vector<std::filesystem::path> m_paths{ std::filesystem::path{} };
    std::vector<Info> m_info;

    std::unordered_map<std::filesystem::path, PathIndex, util::PathHasher> m_pathMap{ { std::filesystem::path{}, PathIndex::Root } };
    std::unordered_map<PathIndex, DirectoryPaths> m_dirPaths;
    std::unordered_map<Key, GeometryHandle, KeyHash, KeyEqual> m_handles;
};

class GeometryManager
{
public:
    explicit GeometryManager(std::shared_ptr<GeometryPaths>);

    const Geometry* find(GeometryHandle);

private:
    std::shared_ptr<GeometryPaths> m_paths;
    std::unordered_map<GeometryHandle, std::unique_ptr<const Geometry>> m_geometry;
};

class RenderGeometryManager
{
public:
    explicit RenderGeometryManager(std::shared_ptr<GeometryManager>);

    GeometryManager* geometryManager() const noexcept { return m_geometryManager.get(); }
    RenderGeometry* find(GeometryHandle);

private:
    std::shared_ptr<GeometryManager> m_geometryManager;
    std::unordered_map<GeometryHandle, std::unique_ptr<RenderGeometry>> m_geometry;
};

} // end namespace celestia::engine
