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
// decode() runs on a worker: it parses the model (.cmod/.3ds/.cms), does
// the post-load conditioning (uniquify materials, sort meshes, opacity,
// normalize) and returns the prepared cmod::Model.
//
// upload() runs on the render thread but does no GL — it just wraps the
// model in a ModelGeometry. VBO/VAO creation is deferred to
// RenderGeometryManager::find() on first render.
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
        // No GL allocations at this layer (those live in
        // RenderGeometryManager), so geometry uses none of the upload budget.
        return 0;
    }

    GpuResource* placeholder() const noexcept;

private:
    std::shared_ptr<const GeometryPaths> m_geometryPaths;
    std::shared_ptr<TexturePaths>        m_texturePaths;
};

} // namespace celestia::engine
