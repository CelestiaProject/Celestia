// virtualtex.h
//
// Copyright (C) 2003, Chris Laurel
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_VIRTUALTEX_H_
#define _CELENGINE_VIRTUALTEX_H_

#include <string>
#include "celutil/basictypes.h"
#include <celengine/texture.h>


class VirtualTexture : public Texture
{
 public:
    VirtualTexture(const std::string& _tilePath,
                   unsigned int _baseSplit,
                   unsigned int _tileSize,
                   const std::string& tilePrefix,
                   const std::string& tileType);
    ~VirtualTexture();

    virtual const TextureTile getTile(int lod, int u, int v);
    virtual void bind();

    virtual int getLODCount() const;
    virtual int getUTileCount(int lod) const;
    virtual int getVTileCount(int lod) const;
    virtual void beginUsage();
    virtual void endUsage();

 private:
    struct Tile
    {
        Tile() : lastUsed(0), tex(NULL), loadFailed(false) {};
        unsigned int lastUsed;
        ImageTexture* tex;
        bool loadFailed;
    };

    struct TileQuadtreeNode
    {
        TileQuadtreeNode() :
            tile(NULL)
        {
            for (int i = 0; i < 4; i++)
                children[i] = NULL;
        };
                
        Tile* tile;
        TileQuadtreeNode* children[4];
    };

    void populateTileTree();
    void addTileToTree(Tile* tile, uint lod, uint v, uint u);
    void makeResident(Tile* tile, uint lod, uint v, uint u);
    ImageTexture* loadTileTexture(uint lod, uint u, uint v);

    Tile* tiles;
    Tile* findTile(unsigned int lod,
                   unsigned int u, unsigned int v);

 private:
    std::string tilePath;
    std::string tileExt;
    std::string tilePrefix;
    unsigned int baseSplit;
    unsigned int tileSize;
    unsigned int ticks;
    unsigned int tilesRequested;
    unsigned int nResolutionLevels;

    enum {
        TileNotLoaded  = -1,
        TileLoadFailed = -2,
    };

    TileQuadtreeNode* tileTree[2];
};


VirtualTexture* LoadVirtualTexture(const std::string& filename);

#endif // _CELENGINE_VIRTUALTEX_H_
