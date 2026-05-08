// virtualtex.cpp
//
// Copyright (C) 2003, Chris Laurel
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.


#include "virtualtex.h"

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <optional>
#include <queue>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

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

// Per-VirtualTexture upload budget. Bounded GL upload work performed at the
// start of beginUsage(). Tune here if uploads become a frame-time spike.
constexpr unsigned int kMaxUploadsPerFrame = 4;

constexpr bool
isPow2(int x)
{
    return ((x & (x - 1)) == 0);
}

// Async tile loader: a single worker thread services every VirtualTexture in
// the process. Render threads call enqueue() to schedule a disk read + image
// decode; results are delivered back via a per-owner result queue keyed by id
// so a destroyed VirtualTexture can never receive a stale callback.

// We carry the owner-side Tile* through the request/result pair as an opaque
// pointer. The worker never dereferences it; only the render-thread code in
// VirtualTexture (which knows the real type) does.
using TileHandle = void*;

struct TileRequest
{
    std::uint64_t          ownerId{ 0 };
    TileHandle             tile{ nullptr };
    unsigned int           lod{ 0 };
    std::filesystem::path  path;
};

struct TileResult
{
    TileHandle             tile{ nullptr };
    unsigned int           lod{ 0 };
    std::unique_ptr<Image> image; // null on failure
};

struct ResultQueueImpl
{
    std::mutex             mutex;
    std::queue<TileResult> queue;
};

class TileLoader
{
public:
    static TileLoader& instance()
    {
        // Meyers singleton: lazy init, joined at static teardown via dtor.
        // First use is always a VirtualTexture constructor calling
        // registerOwner(); the singleton is therefore guaranteed to outlive
        // every VirtualTexture whose construction triggered it. (Static
        // destruction order: function-local statics are destroyed in reverse
        // of their construction order.)
        static TileLoader loader;
        return loader;
    }

    // Set true in the destructor so any VirtualTexture that somehow outlives
    // the singleton (e.g. a static container) can short-circuit its
    // unregisterOwner call. Defensive: the path described above keeps this
    // unreachable in normal use.
    static std::atomic<bool>& shutdownFlag()
    {
        static std::atomic flag{ false };
        return flag;
    }

    TileLoader(const TileLoader&) = delete;
    TileLoader& operator=(const TileLoader&) = delete;
    TileLoader(TileLoader&&) = delete;
    TileLoader& operator=(TileLoader&&) = delete;

    std::uint64_t registerOwner(ResultQueueImpl* rq)
    {
        std::scoped_lock lock(ownersMutex);
        std::uint64_t id = nextId;
        ++nextId;
        owners.emplace(id, rq);
        return id;
    }

    void unregisterOwner(std::uint64_t id)
    {
        std::scoped_lock lock(ownersMutex);
        owners.erase(id);
    }

    void enqueue(TileRequest req)
    {
        {
            std::scoped_lock lock(requestsMutex);
            requests.push(std::move(req));
        }
        requestsCv.notify_one();
    }

private:
    TileLoader() :
        worker([this] { workerLoop(); })
    {
    }

    ~TileLoader()
    {
        {
            std::scoped_lock lock(requestsMutex);
            exitRequested = true;
        }
        requestsCv.notify_all();
        if (worker.joinable())
            worker.join();
        shutdownFlag().store(true);
    }

    void workerLoop()
    {
        for (;;)
        {
            TileRequest req;
            {
                std::unique_lock lock(requestsMutex);
                requestsCv.wait(lock, [this] {
                    return exitRequested || !requests.empty();
                });
                if (exitRequested)
                    return;
                req = std::move(requests.front());
                requests.pop();
            }

            // CPU-only work: file open + image decode. No GL.
            std::unique_ptr<Image> img = Image::load(req.path);

            // Deliver back to the owner if it is still alive. Held under the
            // owners mutex so that an unregisterOwner() racing with delivery
            // either sees the entry (and we deliver) or removes it before we
            // look it up (and we drop). The owner cannot be destroyed while
            // we hold this mutex, because unregisterOwner blocks on it.
            std::scoped_lock lock(ownersMutex);
            auto it = owners.find(req.ownerId);
            if (it == owners.end())
                continue;

            ResultQueueImpl* rq = it->second;
            std::scoped_lock rqLock(rq->mutex);
            rq->queue.push(TileResult{ req.tile, req.lod, std::move(img) });
        }
    }

    std::mutex                                          requestsMutex;
    std::condition_variable                             requestsCv;
    std::queue<TileRequest>                             requests;
    bool                                                exitRequested{ false };

    std::mutex                                          ownersMutex;
    std::unordered_map<std::uint64_t, ResultQueueImpl*> owners;
    std::uint64_t                                       nextId{ 1 };

    // worker is constructed last so the other members are fully alive when
    // workerLoop() starts running.
    std::thread                                         worker;
};

std::unique_ptr<VirtualTexture>
CreateVirtualTexture(const util::AssociativeArray* texParams,
                     const std::filesystem::path& path)
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
                                            tileType);
}


std::unique_ptr<VirtualTexture>
LoadVirtualTexture(std::istream& in, const std::filesystem::path& path)
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

    return CreateVirtualTexture(texParams, path);
}

std::filesystem::path
buildTilePath(const std::filesystem::path& tilePath,
              const std::filesystem::path& tileExt,
              const std::string& tilePrefix,
              unsigned int lod,
              unsigned int u,
              unsigned int v,
              unsigned int baseSplit)
{
    unsigned int level = lod >> baseSplit;
    assert(level < (unsigned)MaxResolutionLevels);

    auto filename = std::filesystem::u8path(fmt::format("{}{}_{}", tilePrefix, u, v));
    filename += tileExt;

    return tilePath / fmt::format("level{:d}", level) / filename;
}

} // end unnamed namespace


// Make TileResultQueue a typedef of the internal type so the unique_ptr in the
// header has a complete type at this translation unit's destruction site.
struct VirtualTexture::TileResultQueue : public ResultQueueImpl {};


// Virtual textures are composed of tiles that are loaded from the hard drive
// as they become visible. File I/O and image decoding happen on a shared
// worker thread (TileLoader, in the anonymous namespace above); the render
// thread performs only the GL upload, with a per-frame budget enforced in
// beginUsage(). Hidden tiles may eventually be evicted from graphics memory to
// make room for other tiles when they become visible -- eviction is tracked by
// issue #2447 and is not implemented in this revision.
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
                               const std::string& _tileType) :
    Texture(_tileSize << (_baseSplit + 1), _tileSize << _baseSplit),
    tilePath(_tilePath),
    tilePrefix(_tilePrefix),
    baseSplit(_baseSplit),
    // resultQueue cannot use an in-class initializer because TileResultQueue
    // is forward-declared in the header.
    resultQueue(std::make_unique<TileResultQueue>())
{
    assert(_tileSize != 0 && isPow2(_tileSize));
    tileExt = fmt::format(".{:s}", _tileType);
    populateTileTree();

    if (DetermineFileType(tileExt, true) == ContentType::DXT5NormalMap)
        setFormatOptions(Texture::DXT5NormalMap);

    // Register only after the tile tree exists so any delivery from the
    // worker can find a valid destination queue.
    loaderId = TileLoader::instance().registerOwner(resultQueue.get());
}


VirtualTexture::~VirtualTexture()
{
    // Unregister first so that no in-flight worker result lands in a result
    // queue that is about to be destroyed. After unregisterOwner() returns,
    // the worker can still hold a TileRequest with our id, but on completion
    // it will fail to find us in the registry and drop the result.
    //
    // The shutdown-flag check guards against the (unlikely) case of a
    // VirtualTexture outliving the loader at static teardown.
    if (loaderId != 0 && !TileLoader::shutdownFlag().load())
        TileLoader::instance().unregisterOwner(loaderId);
}


TextureTile
VirtualTexture::getTile(int lod, int u, int v)
{
    tilesRequested++;

    lod += baseSplit;

    if (lod < 0 || (unsigned int) lod >= nResolutionLevels ||
        u < 0 || u >= (2 << lod) ||
        v < 0 || v >= (1 << lod))
    {
        return TextureTile(0);
    }

    const TileQuadtreeNode* node = &tileTree[u >> lod];
    const Tile* readyTile = nullptr;
    Tile* requestedTile = nullptr;
    unsigned int readyLOD = 0;
    unsigned int requestedLOD = 0;

    // Track the deepest Ready ancestor (for the returned subrect) and the
    // deepest existing Tile along the descent path (the "requested" tile we
    // may need to enqueue).
    if (node->tile != nullptr)
    {
        requestedTile = node->tile.get();
        requestedLOD = 0;
        if (node->tile->state == Tile::State::Ready)
        {
            readyTile = node->tile.get();
            readyLOD = 0;
        }
    }

    for (int n = 0; n < lod; n++)
    {
        unsigned int mask = 1 << (lod - n - 1);
        unsigned int child = (((v & mask) << 1) | (u & mask)) >> (lod - n - 1);
        if (!node->children[child])
            break;

        node = node->children[child].get();
        if (node->tile != nullptr)
        {
            requestedTile = node->tile.get();
            requestedLOD = n + 1;
            if (node->tile->state == Tile::State::Ready)
            {
                readyTile = node->tile.get();
                readyLOD = n + 1;
            }
        }
    }

    // If the deepest existing tile along the path is not yet Ready, ask the
    // loader to fetch it. The function is a no-op for tiles that are already
    // Queued, Ready, or Failed.
    if (requestedTile != nullptr && requestedTile->state == Tile::State::NotLoaded)
    {
        unsigned int reqU = u >> (lod - requestedLOD);
        unsigned int reqV = v >> (lod - requestedLOD);
        requestLoad(requestedTile, requestedLOD, reqU, reqV);
    }

    // No Ready tile is available yet -- caller sees a null name. Falls back to
    // whatever is already on the GPU once the worker delivers.
    if (readyTile == nullptr)
        return TextureTile(0);

    // Set up the texture subrect to be the entire texture.
    float texU = 0.0f;
    float texV = 0.0f;
    float texDU = 1.0f;
    float texDV = 1.0f;

    // If the tile came from a lower LOD than the requested one, we'll only
    // use a subsection of it.
    unsigned int lodDiff = lod - readyLOD;
    texDU = texDV = 1.0f / (float) (1 << lodDiff);
    texU = (u & ((1 << lodDiff) - 1)) * texDU;
    texV = (v & ((1 << lodDiff) - 1)) * texDV;

    return TextureTile(readyTile->tex->getName(), texU, texV, texDU, texDV);
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

    // Drain everything the worker has delivered into a local vector so that
    // GL work happens with the result-queue mutex released.
    std::vector<TileResult> drained;
    {
        std::scoped_lock lock(resultQueue->mutex);
        while (!resultQueue->queue.empty())
        {
            drained.push_back(std::move(resultQueue->queue.front()));
            resultQueue->queue.pop();
        }
    }

    unsigned int uploadsThisFrame = 0;
    std::size_t i = 0;
    while (i < drained.size() && uploadsThisFrame < kMaxUploadsPerFrame)
    {
        auto& result = drained[i];
        auto* tile = static_cast<Tile*>(result.tile);
        ++i;

        if (result.image == nullptr)
        {
            tile->state = Tile::State::Failed;
            continue;
        }

        // Mirrors the original loadTileTexture mip-mode logic: only level 0
        // file tiles get a generated mipmap; higher LODs already act as the
        // mip of their coarser ancestors. Preserve the exact >>= shift the
        // original used.
        unsigned int fileLevel = result.lod >> baseSplit;
        MipMapMode mipMapMode = fileLevel == 0 ? DefaultMipMaps : NoMipMaps;

        if (isPow2(result.image->getWidth()) && isPow2(result.image->getHeight()))
        {
            tile->tex = std::make_unique<ImageTexture>(*result.image, EdgeClamp, mipMapMode);
            tile->state = Tile::State::Ready;
        }
        else
        {
            tile->state = Tile::State::Failed;
        }

        ++uploadsThisFrame;
    }

    // Anything we didn't process this frame goes back on the queue, ahead of
    // anything the worker may have delivered in the meantime. The order is
    // simply "what the worker delivered, in delivery order"; per-tile request
    // order is not preserved (loads complete out-of-order anyway), and that
    // is fine since the render thread treats per-frame results as a set.
    if (i < drained.size())
    {
        std::scoped_lock lock(resultQueue->mutex);
        std::queue<TileResult> rebuilt;
        for (; i < drained.size(); ++i)
            rebuilt.push(std::move(drained[i]));
        while (!resultQueue->queue.empty())
        {
            rebuilt.push(std::move(resultQueue->queue.front()));
            resultQueue->queue.pop();
        }
        resultQueue->queue = std::move(rebuilt);
    }
}


void
VirtualTexture::endUsage()
{
    // Reserved for the eviction task (issue #2447).
}


void
VirtualTexture::requestLoad(Tile* tile, unsigned int lod, unsigned int u, unsigned int v) const
{
    if (tile->state != Tile::State::NotLoaded)
        return;

    tile->state = Tile::State::Queued;

    auto path = buildTilePath(tilePath, tileExt, tilePrefix, lod, u, v, baseSplit);

    TileRequest req;
    req.ownerId = loaderId;
    req.tile    = tile; // implicit Tile* -> void* (TileHandle)
    req.lod     = lod;
    req.path    = std::move(path);
    TileLoader::instance().enqueue(std::move(req));
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
LoadVirtualTexture(const std::filesystem::path& filename)
{
    std::ifstream in(filename, std::ios::in);

    if (!in.good())
    {
        GetLogger()->error("Error opening virtual texture file: {}\n", filename);
        return nullptr;
    }

    return LoadVirtualTexture(in, filename.parent_path());
}
