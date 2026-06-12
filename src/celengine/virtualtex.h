// virtualtex.h
//
// Copyright (C) 2003, Chris Laurel
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>

#include <celengine/resourcesystem.h>
#include <celengine/texture.h>
#include <celimage/image.h>

class VirtualTexture : public Texture, public celestia::engine::ResourceCacheBase
{
public:
    VirtualTexture(const std::filesystem::path& _tilePath,
                   unsigned int _baseSplit,
                   unsigned int _tileSize,
                   const std::string& _tilePrefix,
                   const std::string& _tileType,
                   Colorspace _colorspace);
    ~VirtualTexture() override;

    // Called on the render thread by TextureTraits::upload once the VT is in
    // the texture cache. Registers it so it gets per-frame ticks.
    void attachToResourceSystem(celestia::engine::ResourceSystem& system);

    TextureTile getTile(int lod, int u, int v) override;
    void bind() override;

    int getLODCount() const override;
    int getUTileCount(int lod) const override;
    int getVTileCount(int lod) const override;
    void beginUsage() override;
    void endUsage() override;

    // ResourceCacheBase — render thread only.
    std::size_t drainReady(std::size_t byteBudget) override;
    void purgeStale(std::uint64_t graceFrames) override;

private:
    enum class TileState : std::uint8_t
    {
        NotLoaded,
        Loading,
        Loaded,
        Failed,
    };

    struct Tile
    {
        Tile() = default;
        TileState                     state{ TileState::NotLoaded };
        std::uint64_t                 lastUsedFrame{ 0 };
        std::unique_ptr<ImageTexture> tex{ nullptr };
    };

    struct TileQuadtreeNode
    {
        TileQuadtreeNode() = default;
        std::unique_ptr<Tile> tile{ nullptr };
        std::array<std::unique_ptr<TileQuadtreeNode>, 4> children{ nullptr, nullptr, nullptr, nullptr };
    };

    // Decoded payload posted worker -> render. drainReady() turns the Image
    // into an ImageTexture on the render thread.
    struct ReadyTile
    {
        unsigned int                                  lod{ 0 };
        unsigned int                                  u{ 0 };
        unsigned int                                  v{ 0 };
        std::unique_ptr<celestia::engine::Image>      image;
        bool                                          decoded{ false };
    };

    // Shared cross-thread queue, captured by worker lambdas via shared_ptr so
    // an in-flight load can outlive the VirtualTexture.
    struct SharedQueue
    {
        std::mutex             mutex;
        std::deque<ReadyTile>  ready;
    };

    void populateTileTree();
    void addTileToTree(std::unique_ptr<Tile> tile, unsigned int lod, unsigned int u, unsigned int v);
    void requestTile(Tile* tile, unsigned int lod, unsigned int u, unsigned int v);
    Tile* findTile(unsigned int lod, unsigned int u, unsigned int v) const noexcept;
    static bool purgeTreeRecursive(TileQuadtreeNode& node,
                                   std::uint64_t now,
                                   std::uint64_t graceFrames);

private:
    std::filesystem::path tilePath;
    std::filesystem::path tileExt;
    std::string tilePrefix;
    unsigned int baseSplit{ 0 };
    unsigned int ticks{ 0 };
    unsigned int tilesRequested{ 0 };
    unsigned int nResolutionLevels{ 0 };
    Colorspace colorspace;

    celestia::engine::ResourceSystem* m_system{ nullptr };
    std::shared_ptr<SharedQueue>      m_queue{ std::make_shared<SharedQueue>() };

    std::array<TileQuadtreeNode, 2> tileTree{};
};

std::unique_ptr<VirtualTexture>
LoadVirtualTexture(const std::filesystem::path& filename, Texture::Colorspace colorspace);
