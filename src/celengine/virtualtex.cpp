// virtualtex.cpp
//
// Copyright (C) 2003, Chris Laurel
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <string>
#include <iostream>
#include <fstream>
#include <cmath>
#include <cassert>
#include <utility>
#include <fmt/printf.h>
#include "celutil/debug.h"
#include "celutil/directory.h"
#include "celutil/filetype.h"
#include "virtualtex.h"
#include <GL/glew.h>
#include "parser.h"

using namespace std;

static const int MaxResolutionLevels = 13;


// Virtual textures are composed of tiles that are loaded from the hard drive
// as they become visible.  Hidden tiles may be evicted from graphics memory
// to make room for other tiles when they become visible.
//
// The virtual texture consists of one or more levels of detail.  Each level
// of detail is twice as wide and twice as high as the previous one, therefore
// having four times as many tiles.  The height and width of each LOD must be
// a power of two, with width = 2 * height.  The baseSplit determines the
// number of tiles at the lowest LOD.  It is the log base 2 of the width in
// tiles of LOD zero.  Though it's not required

static bool isPow2(int x)
{
    return ((x & (x - 1)) == 0);
}


#if 0
// Useful if we want to use a packed quadtree to store tiles instead of
// the currently implemented tree structure.
static inline unsigned int lodOffset(unsigned int lod)
{
    return ((1 << (lod << 1)) - 1) & 0xaaaaaaaa;
}
#endif


VirtualTexture::VirtualTexture(string  _tilePath,
                               unsigned int _baseSplit,
                               unsigned int _tileSize,
                               string  _tilePrefix,
                               const string& _tileType) :
    Texture(_tileSize << (_baseSplit + 1), _tileSize << _baseSplit),
    tilePath(std::move(_tilePath)),
    tilePrefix(std::move(_tilePrefix)),
    baseSplit(_baseSplit),
    tileSize(_tileSize),
    ticks(0),
    nResolutionLevels(0)
{
    assert(tileSize != 0 && isPow2(tileSize));
    tileTree[0] = new TileQuadtreeNode();
    tileTree[1] = new TileQuadtreeNode();
    tileExt = string(".") + _tileType;
    populateTileTree();

    if (DetermineFileType(tileExt) == Content_DXT5NormalMap)
        setFormatOptions(Texture::DXT5NormalMap);
}


const TextureTile VirtualTexture::getTile(int lod, int u, int v)
{
    tilesRequested++;
#if 0
    cout << "getTile(" << lod << ", " << u << ", " << v << ")\n";
#endif

    lod += baseSplit;

    if (lod < 0 || (unsigned int) lod >= nResolutionLevels ||
        u < 0 || u >= (2 << lod) ||
        v < 0 || v >= (1 << lod))
    {
        return TextureTile(0);
    }

    TileQuadtreeNode* node = tileTree[u >> lod];
    Tile* tile = node->tile;
    unsigned int tileLOD = 0;

    for (int n = 0; n < lod; n++)
    {
        unsigned int mask = 1 << (lod - n - 1);
        unsigned int child = (((v & mask) << 1) | (u & mask)) >> (lod - n - 1);
        //int child = (((v << 1) | u) >> (lod - n - 1)) & 3;
        if (!node->children[child])
            break;

        node = node->children[child];
        if (node->tile != nullptr)
        {
            tile = node->tile;
            tileLOD = n + 1;
        }
    }

    // No tile was found at all--not even the base texture was found
    if (!tile)
        return TextureTile(0);

    // Make the tile resident.
    unsigned int tileU = u >> (lod - tileLOD);
    unsigned int tileV = v >> (lod - tileLOD);
    makeResident(tile, tileLOD, tileU, tileV);

    // It's possible that we failed to make the tile resident, either
    // because the texture file was bad, or there was an unresolvable
    // out of memory situation.  In that case there is nothing else to
    // do but return a texture tile with a null texture name.
    if (!tile->tex)
        return TextureTile(0);

    // Set up the texture subrect to be the entire texture
    float texU = 0.0f;
    float texV = 0.0f;
    float texDU = 1.0f;
    float texDV = 1.0f;

    // If the tile came from a lower LOD than the requested one,
    // we'll only use a subsection of it.
    unsigned int lodDiff = lod - tileLOD;
    texDU = texDV = 1.0f / (float) (1 << lodDiff);
    texU = (u & ((1 << lodDiff) - 1)) * texDU;
    texV = (v & ((1 << lodDiff) - 1)) * texDV;

#if 0
    cout << "Tile: " << tile->tex->getName() << ", " <<
        texU << ", " << texV << ", " << texDU << ", " << texDV << '\n';
#endif
    return TextureTile(tile->tex->getName(), texU, texV, texDU, texDV);
}


void VirtualTexture::bind()
{
    // Treating a virtual texture like an ordinary one will not work; this is a
    // weakness in the class hierarchy.
}


int VirtualTexture::getLODCount() const
{
    return nResolutionLevels - baseSplit;
}


int VirtualTexture::getUTileCount(int lod) const
{
    return 2 << (lod + baseSplit);
}


int VirtualTexture::getVTileCount(int lod) const
{
    return 1 << (lod + baseSplit);
}


void VirtualTexture::beginUsage()
{
    ticks++;
    tilesRequested = 0;
}


void VirtualTexture::endUsage()
{
}


#if 0
unsigned int VirtualTexture::tileIndex(unsigned int lod,
                               unsigned int u, unsigned int v)
{
    unsigned int lodBase = lodOffset(lod + baseSplit) - lodOffset(baseSplit);
    return lodBase + (v << (lod << 1)) + u;
}
#endif


ImageTexture* VirtualTexture::loadTileTexture(unsigned int lod, unsigned int u, unsigned int v)
{
    lod >>= baseSplit;

    assert(lod < (unsigned)MaxResolutionLevels);

    string path;
    path = fmt::sprintf("%slevel%d/%s%d_%d%s", tilePath, lod, tilePrefix.c_str(), u, v, tileExt);

    Image* img = LoadImageFromFile(path);
    if (img == nullptr)
        return nullptr;

    ImageTexture* tex = nullptr;

    // Only use mip maps for the LOD 0; for higher LODs, the function of mip
    // mapping is built into the texture.
    MipMapMode mipMapMode = lod == 0 ? DefaultMipMaps : NoMipMaps;

    if (isPow2(img->getWidth()) && isPow2(img->getHeight()))
        tex = new ImageTexture(*img, EdgeClamp, mipMapMode);

    // TODO: Virtual textures can have tiles in different formats, some
    // compressed and some not. The compression flag doesn't make much
    // sense for them.
    compressed = img->isCompressed();

    delete img;

    return tex;
}


void VirtualTexture::makeResident(Tile* tile, unsigned int lod, unsigned int u, unsigned int v)
{
    if (tile->tex == nullptr && !tile->loadFailed)
    {
        // Potentially evict other tiles in order to make this one fit
        tile->tex = loadTileTexture(lod, u, v);
        if (tile->tex == nullptr)
        {
            // cout << "Texture load failed!\n";
            tile->loadFailed = true;
        }
    }
}


void VirtualTexture::populateTileTree()
{
    // Count the number of resolution levels present
    unsigned int maxLevel = 0;

    // Crash potential if the tile prefix contains a %, so disallow it
    string pattern;
    if (tilePrefix.find('%') == string::npos)
        pattern = tilePrefix + "%d_%d.";

    for (int i = 0; i < MaxResolutionLevels; i++)
    {
        string path = fmt::sprintf("%slevel%d", tilePath, i);
        if (IsDirectory(path))
        {
            Directory* dir = OpenDirectory(path);
            if (dir)
            {
                maxLevel = i + baseSplit;
                int uLimit = 2 << maxLevel;
                int vLimit = 1 << maxLevel;

                string filename;
                while (dir->nextFile(filename))
                {
                    int u = -1, v = -1;
                    if (sscanf(filename.c_str(), pattern.c_str(), &u, &v) == 2)
                    {
                        if (u >= 0 && v >= 0 && u < uLimit && v < vLimit)
                        {
                            // Found a tile, so add it to the quadtree
                            Tile* tile = new Tile();
                            addTileToTree(tile, maxLevel, (unsigned int) u, (unsigned int) v);
                        }
                    }
                }
                delete dir;
            }
        }
    }

    nResolutionLevels = maxLevel + 1;
}


void VirtualTexture::addTileToTree(Tile* tile, unsigned int lod, unsigned int u, unsigned int v)
{
    TileQuadtreeNode* node = tileTree[u >> lod];

    for (unsigned int i = 0; i < lod; i++)
    {
        unsigned int mask = 1 << (lod - i - 1);
        unsigned int child = (((v & mask) << 1) | (u & mask)) >> (lod - i - 1);
        if (!node->children[child])
            node->children[child] = new TileQuadtreeNode();
        node = node->children[child];
    }
#if 0
    clog << "addTileToTree: " << node << ", " << lod << ", " << u << ", " << v << '\n';
#endif

    // Verify that the tile doesn't already exist
    if (!node->tile)
        node->tile = tile;
}


static VirtualTexture* CreateVirtualTexture(Hash* texParams,
                                            const string& path)
{
    string imageDirectory;
    if (!texParams->getString("ImageDirectory", imageDirectory))
    {
        DPRINTF(0, "ImageDirectory missing in virtual texture.\n");
        return nullptr;
    }

    double baseSplit = 0.0;
    if (!texParams->getNumber("BaseSplit", baseSplit) ||
        baseSplit < 0.0 || baseSplit != floor(baseSplit))
    {
        DPRINTF(0, "BaseSplit in virtual texture missing or has bad value\n");
        return nullptr;
    }

    double tileSize = 0.0;
    if (!texParams->getNumber("TileSize", tileSize))
    {
        DPRINTF(0, "TileSize is missing from virtual texture\n");
        return nullptr;
    }

    if (tileSize != floor(tileSize) ||
        tileSize < 64.0 ||
        !isPow2((int) tileSize))
    {
        DPRINTF(0, "Virtual texture tile size must be a power of two >= 64\n");
        return nullptr;
    }

    string tileType = "dds";
    texParams->getString("TileType", tileType);

    string tilePrefix = "tx_";
    texParams->getString("TilePrefix", tilePrefix);

    // if absolute directory notation for ImageDirectory used,
    // don't prepend the current add-on path.
    string directory = imageDirectory + "/";
    if (directory[0] != '/' && directory[1] != ':')
    {
        directory = path + "/" + directory;
    }
    return new VirtualTexture(directory,
                              (unsigned int) baseSplit,
                              (unsigned int) tileSize,
                              tilePrefix,
                              tileType);
}


static VirtualTexture* LoadVirtualTexture(istream& in, const string& path)
{
    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);

    if (tokenizer.nextToken() != Tokenizer::TokenName)
        return nullptr;

    string virtTexString = tokenizer.getNameValue();
    if (virtTexString != "VirtualTexture")
        return nullptr;

    Value* texParamsValue = parser.readValue();
    if (texParamsValue == nullptr || texParamsValue->getType() != Value::HashType)
    {
        DPRINTF(0, "Error parsing virtual texture\n");
        delete texParamsValue;
        return nullptr;
    }

    Hash* texParams = texParamsValue->getHash();

    VirtualTexture* virtualTex  = CreateVirtualTexture(texParams, path);
    delete texParamsValue;

    return virtualTex;
}


VirtualTexture* LoadVirtualTexture(const string& filename)
{
    ifstream in(filename, ios::in);

    if (!in.good())
    {
        //DPRINTF(0, "Error opening virtual texture file: %s\n", filename.c_str());
        return nullptr;
    }

    // Strip off every character including and after the final slash to get
    // the pathname.
    string::size_type pos = filename.rfind('/');
    string path = pos != string::npos ? filename.substr(0, pos) : ".";

    return LoadVirtualTexture(in, path);
}
