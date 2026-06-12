// texturetraits.h
//
// Copyright (C) 2026-present, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <utility>

#include <celimage/image.h>
#include <celutil/texhandle.h>

#include "texmanager.h"
#include "texture.h"
#include "virtualtex.h"

namespace celestia::engine
{

// Decoded representation of a texture. Produced off-thread by
// TextureTraits::decode() and consumed on the main thread by
// TextureTraits::upload(). One of {image, virtualTexture} is populated;
// the other is null.
struct DecodedTexture
{
    // Raster path: a CPU-side Image plus the parameters needed to build
    // an ImageTexture / TiledTexture from it on the main thread.
    std::unique_ptr<Image>           image;
    Texture::AddressMode             addressMode  = Texture::EdgeClamp;
    Texture::MipMapMode              mipMode      = Texture::DefaultMipMaps;
    bool                             dxt5NormalMap = false;

    // Virtual-texture path: the VirtualTexture object can be constructed
    // off-thread because its constructor performs only file parsing and
    // bookkeeping (tile uploads happen lazily on bind, on the main thread).
    std::unique_ptr<Texture>         virtualTexture;
};

// Traits adapter that plugs Celestia's texture loader into the generic
// AsyncResourceCache.
class TextureTraits
{
public:
    using Handle      = util::TextureHandle;
    using Info        = TextureInfo;
    using CpuData     = DecodedTexture;
    using GpuResource = Texture;

    // `resolutionPtr` (when non-null) lets the cache always read the
    // owning TextureManager's current resolution; passing null falls back
    // to `fixedResolution` (used by the shadow-texture cache, which is
    // pinned at medres).
    TextureTraits(std::shared_ptr<const TexturePaths> paths,
                  const TextureResolution* resolutionPtr,
                  TextureResolution fixedResolution) :
        m_paths(std::move(paths)),
        m_resolutionPtr(resolutionPtr),
        m_fixedResolution(fixedResolution)
    {
    }

    bool getInfo(Handle handle, Info& out) const
    {
        return m_paths->getInfo(handle, effectiveResolution(), out);
    }

    // Runs on a worker thread. Must not touch GL.
    std::optional<CpuData> decode(const Info& info) const;

    // Runs on the main (render) thread; performs GL uploads.
    std::unique_ptr<GpuResource> upload(CpuData&& cpu) const;

    std::size_t gpuBytes(const GpuResource& tex) const noexcept
    {
        // Texture does not currently expose a precise mip-chain byte total,
        // so estimate from base dimensions assuming 32 bpp + mip chain
        // (~33% overhead). The value is consumed by the ResourceSystem's
        // per-frame upload budget — an estimate is sufficient.
        const std::size_t pixels = static_cast<std::size_t>(tex.getWidth()) *
                                   static_cast<std::size_t>(tex.getHeight());
        return (pixels * 4u * 4u) / 3u;
    }

    GpuResource* placeholder() const noexcept { return nullptr; }

    TextureResolution effectiveResolution() const noexcept
    {
        return m_resolutionPtr != nullptr ? *m_resolutionPtr : m_fixedResolution;
    }

private:
    std::shared_ptr<const TexturePaths> m_paths;
    const TextureResolution*            m_resolutionPtr;
    TextureResolution                   m_fixedResolution;
};

} // namespace celestia::engine
