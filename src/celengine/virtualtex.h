// virtualtex.h
//
// Copyright (C) 2003, Chris Laurel
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>
#include <string>

#include <celcompat/filesystem.h>
#include <celengine/texture.h>


class VirtualTexture : public Texture
{
 public:
    VirtualTexture(const fs::path& _tilePath,
                   unsigned int _baseSplit,
                   unsigned int _tileSize,
                   const std::string& _tilePrefix,
                   const std::string& _tileType);
    ~VirtualTexture() = default;

    const TextureTile getTile(int lod, int u, int v) override;
    void bind() override;

    int getLODCount() const override;
    int getUTileCount(int lod) const override;
    int getVTileCount(int lod) const override;
    void beginUsage() override;
    void endUsage() override;

 private:
    struct Tile
    {
        Tile() = default;
        unsigned int lastUsed{ 0 };
        ImageTexture* tex{ nullptr };
        bool loadFailed{ false };
    };

    struct TileQuadtreeNode
    {
        TileQuadtreeNode() = default;
        Tile* tile{ nullptr };
        TileQuadtreeNode* children[4]{ nullptr, nullptr, nullptr, nullptr};
    };

    void populateTileTree();
    void addTileToTree(Tile* tile, unsigned int lod, unsigned int u, unsigned int v);
    void makeResident(Tile* tile, unsigned int lod, unsigned int u, unsigned int v);
    ImageTexture* loadTileTexture(unsigned int lod, unsigned int u, unsigned int v);

    Tile* tiles{ nullptr };
    Tile* findTile(unsigned int lod,
                   unsigned int u, unsigned int v);

 private:
    fs::path tilePath;
    fs::path tileExt;
    std::string tilePrefix;
    unsigned int baseSplit{ 0 };
    unsigned int tileSize{ 0 };
    unsigned int ticks{ 0 };
    unsigned int tilesRequested{ 0 };
    unsigned int nResolutionLevels{ 0 };

    enum
    {
        TileNotLoaded  = -1,
        TileLoadFailed = -2,
    };

    TileQuadtreeNode* tileTree[2];
};


std::unique_ptr<VirtualTexture>
LoadVirtualTexture(const fs::path& filename);
