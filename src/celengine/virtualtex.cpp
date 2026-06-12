// virtualtex.cpp
//
// Copyright (C) 2003, Chris Laurel
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.


#include "virtualtex.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <optional>
#include <system_error>
#include <utility>

#include <fmt/format.h>

#include <celutil/filetype.h>
#include <celutil/logger.h>
#include <celutil/parser.h>
#include <celutil/tokenizer.h>

namespace util = celestia::util;

using celestia::util::GetLogger;
using celestia::engine::Image;

namespace
{

constexpr int MaxResolutionLevels = 13;

constexpr bool
isPow2(int x)
{
    return ((x & (x - 1)) == 0);
}

std::unique_ptr<VirtualTexture>
CreateVirtualTexture(const util::AssociativeArray* texParams,
                     const std::filesystem::path& path,
                     Texture::Colorspace colorspace)
{
    const std::string* imageDirectory = texParams->getString("ImageDirectory");
    if (imageDirectory == nullptr)
    {
        GetLogger()->error("ImageDirectory missing in virtual texture.\n");
        return nullptr;
    }

    std::optional<double> baseSplit = texParams->getNumber<double>("BaseSplit");
    if (!baseSplit.has_value() || *baseSplit < 0.0 || *baseSplit != floor(*baseSplit))
    {
        GetLogger()->error("BaseSplit in virtual texture missing or has bad value\n");
        return nullptr;
    }

    std::optional<double> tileSize = texParams->getNumber<double>("TileSize");
    if (!tileSize.has_value())
    {
        GetLogger()->error("TileSize is missing from virtual texture\n");
        return nullptr;
    }

    if (*tileSize != floor(*tileSize) ||
        *tileSize < 64.0 ||
        !isPow2((int) *tileSize))
    {
        GetLogger()->error("Virtual texture tile size must be a power of two >= 64\n");
        return nullptr;
    }

    std::string tileType = "dds";
    if (const std::string* tileTypeVal = texParams->getString("TileType"); tileTypeVal != nullptr)
    {
        tileType = *tileTypeVal;
    }

    std::string tilePrefix = "tx_";
    if (const std::string* tilePrefixVal = texParams->getString("TilePrefix"); tilePrefixVal != nullptr)
    {
        tilePrefix = *tilePrefixVal;
    }

    // if absolute directory notation for ImageDirectory used,
    // don't prepend the current add-on path.
    std::filesystem::path directory(*imageDirectory);

    if (directory.is_relative())
        directory = path / directory;
    return std::make_unique<VirtualTexture>(directory,
                                            (unsigned int) *baseSplit,
                                            (unsigned int) *tileSize,
                                            tilePrefix,
                                            tileType,
                                            colorspace);
}


std::unique_ptr<VirtualTexture>
LoadVirtualTexture(std::istream& in, const std::filesystem::path& path, Texture::Colorspace colorspace)
{
    util::Tokenizer tokenizer(in);
    util::Parser parser(&tokenizer);

    tokenizer.nextToken();
    if (auto tokenValue = tokenizer.getNameValue(); tokenValue != "VirtualTexture")
    {
        return nullptr;
    }

    const util::Value texParamsValue = parser.readValue();
    const util::AssociativeArray* texParams = texParamsValue.getHash();
    if (texParams == nullptr)
    {
        GetLogger()->error("Error parsing virtual texture\n");
        return nullptr;
    }

    return CreateVirtualTexture(texParams, path, colorspace);
}

} // end unnamed namespace


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

VirtualTexture::VirtualTexture(const std::filesystem::path& _tilePath,
                               unsigned int _baseSplit,
                               unsigned int _tileSize,
                               const std::string& _tilePrefix,
                               const std::string& _tileType,
                               Colorspace _colorspace) :
    Texture(_tileSize << (_baseSplit + 1), _tileSize << _baseSplit),
    tilePath(_tilePath),
    tilePrefix(_tilePrefix),
    baseSplit(_baseSplit),
    colorspace(_colorspace)
{
    assert(_tileSize != 0 && isPow2(_tileSize));
    tileExt = fmt::format(".{:s}", _tileType);
    populateTileTree();

    if (DetermineFileType(tileExt, true) == ContentType::DXT5NormalMap)
        setFormatOptions(Texture::DXT5NormalMap);
}

VirtualTexture::~VirtualTexture()
{
    // Unregister first so we stop getting per-frame ticks. In-flight workers
    // still hold m_queue via shared_ptr; their payloads are dropped once the
    // last reference goes away.
    if (m_system != nullptr)
        m_system->unregisterCache(this);
}

void
VirtualTexture::attachToResourceSystem(celestia::engine::ResourceSystem& system)
{
    assert(m_system == nullptr);
    m_system = &system;
    m_system->registerCache(this);
}


TextureTile
VirtualTexture::getTile(int lod, int u, int v)
{
    assert(m_system != nullptr);
    tilesRequested++;

    lod += baseSplit;

    if (lod < 0 || (unsigned int) lod >= nResolutionLevels ||
        u < 0 || u >= (2 << lod) ||
        v < 0 || v >= (1 << lod))
    {
        return TextureTile(0);
    }

    // Walk the quadtree toward (lod, u, v), tracking the deepest existing
    // tile and the deepest one that's already Loaded.
    const TileQuadtreeNode* node = &tileTree[u >> lod];
    Tile* deepestExisting = node->tile.get();
    unsigned int deepestExistingLOD = 0;
    Tile* bestResident = nullptr;
    unsigned int bestResidentLOD = 0;

    auto consider = [&](Tile* t, unsigned int n)
    {
        if (t == nullptr)
            return;
        deepestExisting    = t;
        deepestExistingLOD = n;
        if (t->state == TileState::Loaded)
        {
            bestResident    = t;
            bestResidentLOD = n;
        }
    };
    consider(deepestExisting, 0);

    for (int n = 0; n < lod; n++)
    {
        unsigned int mask = 1 << (lod - n - 1);
        unsigned int child = (((v & mask) << 1) | (u & mask)) >> (lod - n - 1);
        if (!node->children[child])
            break;

        node = node->children[child].get();
        consider(node->tile.get(), n + 1);
    }

    // No tile exists here at all - transparent.
    if (deepestExisting == nullptr)
        return TextureTile(0);

    // Kick off the async load if it hasn't been requested (or was evicted).
    if (deepestExisting->state == TileState::NotLoaded)
    {
        unsigned int tileU = u >> (lod - deepestExistingLOD);
        unsigned int tileV = v >> (lod - deepestExistingLOD);
        requestTile(deepestExisting, deepestExistingLOD, tileU, tileV);
    }

    // While it decodes, fall back to the coarsest resident ancestor.
    if (bestResident == nullptr)
        return TextureTile(0);

    bestResident->lastUsedFrame = m_system->currentFrame();

    float texU = 0.0f;
    float texV = 0.0f;
    float texDU = 1.0f;
    float texDV = 1.0f;

    unsigned int lodDiff = lod - bestResidentLOD;
    texDU = texDV = 1.0f / (float) (1 << lodDiff);
    texU = (u & ((1 << lodDiff) - 1)) * texDU;
    texV = (v & ((1 << lodDiff) - 1)) * texDV;

    return TextureTile(bestResident->tex->getName(), texU, texV, texDU, texDV);
}


void
VirtualTexture::bind()
{
    // Treating a virtual texture like an ordinary one will not work; this is a
    // weakness in the class hierarchy.
}


int
VirtualTexture::getLODCount() const
{
    return nResolutionLevels - baseSplit;
}


int
VirtualTexture::getUTileCount(int lod) const
{
    return 2 << (lod + baseSplit);
}


int
VirtualTexture::getVTileCount(int lod) const
{
    return 1 << (lod + baseSplit);
}


void
VirtualTexture::beginUsage()
{
    ticks++;
    tilesRequested = 0;
}


void
VirtualTexture::endUsage()
{
}


void
VirtualTexture::requestTile(Tile* tile, unsigned int lod, unsigned int u, unsigned int v)
{
    assert(tile != nullptr);
    assert(tile->state == TileState::NotLoaded);
    assert(m_system != nullptr);

    tile->state = TileState::Loading;

    unsigned int diskLod = lod >> baseSplit;
    auto filename = std::filesystem::u8path(fmt::format("{}{}_{}", tilePrefix, u, v));
    filename += tileExt;
    auto path = tilePath / fmt::format("level{:d}", diskLod) / filename;

    // Capture the queue by shared_ptr so an in-flight worker can outlive this
    // VirtualTexture; no raw VirtualTexture* is captured.
    auto queue = m_queue;
    Colorspace cs = colorspace;
    m_system->submit([queue, path = std::move(path), lod, u, v, cs]()
    {
        ReadyTile ready;
        ready.lod = lod;
        ready.u = u;
        ready.v = v;
        ready.image = celestia::engine::Image::load(path);
        ready.decoded = ready.image != nullptr;
        if (ready.decoded && cs == Texture::Colorspace::LinearColorspace)
            ready.image->forceLinear();

        std::scoped_lock lock(queue->mutex);
        queue->ready.push_back(std::move(ready));
    });
}


VirtualTexture::Tile*
VirtualTexture::findTile(unsigned int lod, unsigned int u, unsigned int v) const noexcept
{
    const TileQuadtreeNode* node = &tileTree[u >> lod];
    for (unsigned int i = 0; i < lod; i++)
    {
        unsigned int mask = 1 << (lod - i - 1);
        unsigned int child = (((v & mask) << 1) | (u & mask)) >> (lod - i - 1);
        if (!node->children[child])
            return nullptr;
        node = node->children[child].get();
    }
    return node->tile.get();
}


std::size_t
VirtualTexture::drainReady(std::size_t byteBudget)
{
    assert(m_system != nullptr);

    std::size_t bytes = 0;
    while (bytes < byteBudget)
    {
        ReadyTile ready;
        {
            std::scoped_lock lock(m_queue->mutex);
            if (m_queue->ready.empty())
                break;
            ready = std::move(m_queue->ready.front());
            m_queue->ready.pop_front();
        }

        Tile* tile = findTile(ready.lod, ready.u, ready.v);
        // Drop the result if the tile was evicted or the node is gone.
        if (tile == nullptr || tile->state != TileState::Loading)
            continue;

        if (!ready.decoded || !ready.image)
        {
            tile->state = TileState::Failed;
            continue;
        }

        if (!isPow2(ready.image->getWidth()) || !isPow2(ready.image->getHeight()))
        {
            tile->state = TileState::Failed;
            continue;
        }

        unsigned int diskLod = ready.lod >> baseSplit;
        MipMapMode mipMapMode = diskLod == 0 ? DefaultMipMaps : NoMipMaps;
        tile->tex = std::make_unique<ImageTexture>(*ready.image, EdgeClamp, mipMapMode);
        tile->state = TileState::Loaded;
        tile->lastUsedFrame = m_system->currentFrame();
        bytes += static_cast<std::size_t>(ready.image->getSize());
    }
    return bytes;
}


bool
VirtualTexture::purgeTreeRecursive(TileQuadtreeNode& node,
                                   std::uint64_t now,
                                   std::uint64_t graceFrames)
{
    // Recurse first so we know whether any finer descendant is still resident.
    bool subtreeLoaded = false;
    for (const auto& child : node.children)
    {
        if (child)
            subtreeLoaded |= purgeTreeRecursive(*child, now, graceFrames);
    }

    if (node.tile && node.tile->state == TileState::Loaded)
    {
        // Keep a coarser tile while a finer descendant is still loaded: it's a
        // cheap fallback and avoids blank tiles when zooming back out.
        if (!subtreeLoaded && (now - node.tile->lastUsedFrame) > graceFrames)
        {
            // Drop the GPU texture; reset so a later getTile() reloads from disk.
            node.tile->tex.reset();
            node.tile->state = TileState::NotLoaded;
        }
        else
        {
            subtreeLoaded = true;
        }
    }

    return subtreeLoaded;
}


void
VirtualTexture::purgeStale(std::uint64_t graceFrames)
{
    assert(m_system != nullptr);
    std::uint64_t now = m_system->currentFrame();
    for (auto& root : tileTree)
        purgeTreeRecursive(root, now, graceFrames);
}


void VirtualTexture::populateTileTree()
{
    // Count the number of resolution levels present
    unsigned int maxLevel = 0;

    for (int i = 0; i < MaxResolutionLevels; i++)
    {
        std::filesystem::path path = tilePath / fmt::format("level{:d}", i);
        std::error_code ec;
        if (!std::filesystem::is_directory(path, ec))
            continue;

        maxLevel = i + baseSplit;
        int uLimit = 2 << maxLevel;
        int vLimit = 1 << maxLevel;

        for (const auto& d : std::filesystem::directory_iterator(path, ec))
        {
            if (!std::filesystem::is_regular_file(d, ec))
                continue;

            int u = -1;
            int v = -1;
            if (auto filename = d.path().filename().string();
                filename.size() < tilePrefix.size()
                || std::string_view{filename.data(), tilePrefix.size()} != tilePrefix
                || std::sscanf(filename.c_str() + tilePrefix.size(), "%d_%d.", &u, &v) != 2
                || u < 0 || u >= uLimit
                || v < 0 || v >= vLimit)
                continue;

            // Found a tile, so add it to the quadtree
            addTileToTree(std::make_unique<Tile>(), maxLevel, (unsigned int) u, (unsigned int) v);
        }
    }

    nResolutionLevels = maxLevel + 1;
}


void
VirtualTexture::addTileToTree(std::unique_ptr<Tile> tile, unsigned int lod, unsigned int u, unsigned int v)
{
    TileQuadtreeNode* node = &tileTree[u >> lod];

    for (unsigned int i = 0; i < lod; i++)
    {
        unsigned int mask = 1 << (lod - i - 1);
        unsigned int child = (((v & mask) << 1) | (u & mask)) >> (lod - i - 1);
        if (!node->children[child])
            node->children[child] = std::make_unique<TileQuadtreeNode>();
        node = node->children[child].get();
    }

    // Verify that the tile doesn't already exist
    if (!node->tile)
        node->tile = std::move(tile);
}


std::unique_ptr<VirtualTexture>
LoadVirtualTexture(const std::filesystem::path& filename, Texture::Colorspace colorspace)
{
    std::ifstream in(filename, std::ios::in);

    if (!in.good())
    {
        GetLogger()->error("Error opening virtual texture file: {}\n", filename);
        return nullptr;
    }

    return LoadVirtualTexture(in, filename.parent_path(), colorspace);
}
