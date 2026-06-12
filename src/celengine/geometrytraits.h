// geometrytraits.h
//
// Copyright (C) 2026-present, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstddef>
#include <memory>
#include <optional>

#include "geometry.h"
#include "meshmanager.h"

namespace cmod { class Model; }

namespace celestia::engine
{

// Traits adapter that plugs cmod model loading into AsyncResourceCache.
//
// decode() runs on a worker thread: it parses the model file (.cmod /
// .3ds / .cms), runs the post-load conditioning (uniquifyMaterials,
// sortMeshes, determineOpacity, normalize/transform) and returns the
// fully-prepared cmod::Model.
//
// upload() runs on the render thread but performs no GL work — it just
// wraps the model in a ModelGeometry. The actual GL-side construction
// (VBOs/VAOs) is deferred to RenderGeometryManager::find() which calls
// Geometry::createRenderGeometry() the first time the body is rendered.
class GeometryTraits
{
public:
    using Handle      = GeometryHandle;
    using Info        = GeometryInfo;
    using CpuData     = std::unique_ptr<cmod::Model>;
    using GpuResource = Geometry;

    GeometryTraits(std::shared_ptr<const GeometryPaths> geometryPaths,
                   std::shared_ptr<TexturePaths> texturePaths);

    bool getInfo(Handle handle, Info& out) const;
    std::optional<CpuData> decode(const Info& info) const;
    std::unique_ptr<GpuResource> upload(CpuData&& model) const;

    std::size_t gpuBytes(const GpuResource& /*geom*/) const noexcept
    {
        // No GL allocations happen at this layer (those live in
        // RenderGeometryManager / ModelRenderGeometry). Returning zero
        // means GeometryManager doesn't consume any of the per-frame
        // upload budget, which is correct.
        return 0;
    }

    GpuResource* placeholder() const noexcept;

private:
    std::shared_ptr<const GeometryPaths> m_geometryPaths;
    std::shared_ptr<TexturePaths>        m_texturePaths;
};

} // namespace celestia::engine
