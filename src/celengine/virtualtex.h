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
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

#include <celengine/texture.h>

class VirtualTexture : public Texture
{
public:
    VirtualTexture(const std::filesystem::path& _tilePath,
                   unsigned int _baseSplit,
                   unsigned int _tileSize,
                   const std::string& _tilePrefix,
                   const std::string& _tileType);
    ~VirtualTexture() override;

    TextureTile getTile(int lod, int u, int v) override;
    void bind() override;

    int getLODCount() const override;
    int getUTileCount(int lod) const override;
    int getVTileCount(int lod) const override;
    void beginUsage() override;
    void endUsage() override;

private:
    struct Tile
    {
        // State of the tile in the async load pipeline. Read AND written
        // exclusively on the render thread (in getTile / beginUsage /
        // requestLoad). The worker thread treats Tile* as an opaque cookie
        // and never dereferences it, so no atomic is needed.
        enum class State : std::uint8_t
        {
            NotLoaded,
            Queued,
            Ready,
            Failed,
        };

        Tile() = default;
        std::unique_ptr<ImageTexture> tex{ nullptr };
        State state{ State::NotLoaded };
    };

    struct TileQuadtreeNode
    {
        TileQuadtreeNode() = default;
        std::unique_ptr<Tile> tile{ nullptr };
        std::array<std::unique_ptr<TileQuadtreeNode>, 4> children{ nullptr, nullptr, nullptr, nullptr };
    };

    struct TileResultQueue;

    void populateTileTree();
    void addTileToTree(std::unique_ptr<Tile> tile, unsigned int lod, unsigned int u, unsigned int v);
    void requestLoad(Tile* tile, unsigned int lod, unsigned int u, unsigned int v) const;

private:
    std::filesystem::path tilePath;
    std::filesystem::path tileExt;
    std::string tilePrefix;
    unsigned int baseSplit{ 0 };
    // ticks / tilesRequested are reserved for the upcoming eviction task
    // (issue #2447); kept here to avoid churn when eviction lands.
    unsigned int ticks{ 0 };
    unsigned int tilesRequested{ 0 };
    unsigned int nResolutionLevels{ 0 };
    std::uint64_t loaderId{ 0 };
    std::unique_ptr<TileResultQueue> resultQueue;

    std::array<TileQuadtreeNode, 2> tileTree{};
};

std::unique_ptr<VirtualTexture>
LoadVirtualTexture(const std::filesystem::path& filename);
