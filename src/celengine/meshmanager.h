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
#include "resourcesystem.h"
#include "texmanager.h"

namespace celestia::engine
{

class ResourceSystem;
class GeometryTraits;
template <typename Traits> class AsyncResourceCache;

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
    GeometryManager(std::shared_ptr<const GeometryPaths>,
                    std::shared_ptr<TexturePaths>,
                    ResourceSystem&);
    ~GeometryManager();

    // Returns the parsed Geometry, or nullptr while the decode is still in
    // flight (callers skip the body's mesh that frame). GeometryHandle::Empty
    // always returns the EmptyGeometry sentinel.
    const Geometry* find(GeometryHandle);

private:
    std::shared_ptr<const GeometryPaths> m_geometryPaths;
    std::shared_ptr<TexturePaths> m_texturePaths;
    std::unique_ptr<Geometry> m_emptyGeometry{ std::make_unique<EmptyGeometry>() };
    std::unique_ptr<AsyncResourceCache<GeometryTraits>> m_cache;
};

// Caches the GL-side RenderGeometry (VBOs/VAOs) built from the CPU-side
// Geometry. Registers with the ResourceSystem so idle geometry is evicted
// like any other GPU resource; a later find() rebuilds it on demand.
class RenderGeometryManager : public ResourceCacheBase, private util::NoCopy
{
public:
    RenderGeometryManager(std::shared_ptr<GeometryManager>, ResourceSystem&);
    ~RenderGeometryManager() override;

    GeometryManager* geometryManager() const noexcept { return m_geometryManager.get(); }
    RenderGeometry* find(GeometryHandle);

    // ResourceCacheBase. Geometry is built lazily in find() rather than from a
    // ready queue, so drainReady has nothing to upload.
    std::size_t drainReady(std::size_t) override { return 0; }
    void purgeStale(std::uint64_t graceFrames) override;

private:
    struct Entry
    {
        std::unique_ptr<RenderGeometry> geometry;
        std::uint64_t lastUsedFrame{ 0 };
    };

    std::shared_ptr<GeometryManager> m_geometryManager;
    ResourceSystem* m_system;
    std::unordered_map<GeometryHandle, Entry> m_geometry;
};

} // end namespace celestia::engine
