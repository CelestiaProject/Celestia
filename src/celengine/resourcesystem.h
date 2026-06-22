// resourcesystem.h
//
// Copyright (C) 2026-present, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include <celutil/classops.h>

namespace celestia::engine
{

// Implemented by per-resource-type caches (textures, geometry, ...). The
// ResourceSystem owns the worker pool and frame tick; caches own their
// entries and ready queue. All methods run on the render (GL) thread.
class ResourceCacheBase
{
public:
    virtual ~ResourceCacheBase() = default;

    // Upload ready entries until at least `byteBudget` bytes are uploaded or
    // the ready queue drains. Returns bytes actually uploaded.
    virtual std::size_t drainReady(std::size_t byteBudget) = 0;

    // Evict entries untouched for more than `graceFrames`. Must skip pinned
    // or in-flight entries.
    virtual void purgeStale(std::uint64_t graceFrames) = 0;
};

// Coordinator for asynchronous resource loading.
//
// Owns a worker pool that runs CPU-bound decode tasks, plus the per-frame
// hooks the renderer drives:
//   beginFrame()  — bump the frame counter (top of frame)
//   drainCaches() — render-thread GL uploads, bounded by the upload budget
//   purgeIfDue()  — periodic LRU eviction across all caches
//
// The ResourceSystem does no GL itself; uploads happen in the caches.
class ResourceSystem : private util::NoCopy
{
public:
    using DecodeTask = std::function<void()>;

    // numWorkers == 0 selects max(1, hardware_concurrency - 1).
    explicit ResourceSystem(unsigned numWorkers = 0);
    ~ResourceSystem();

    // Stop and join all worker threads. Idempotent. Must be called (directly
    // or via the destructor) before any registered cache is destroyed, so an
    // in-flight decode can never post to a freed cache. Queued tasks still
    // drain against their (live) caches before the join completes.
    void shutdown() noexcept;

    // Submit a CPU-bound task to the worker pool. It runs on a worker thread
    // and is responsible for posting its result back to the owning cache.
    // Tasks run in submission order (FIFO).
    void submit(DecodeTask task);

    // Per-frame lifecycle. Call beginFrame() once per frame before any cache
    // find(); call drainCaches() and purgeIfDue() after the frame's work.
    void beginFrame() noexcept;
    void drainCaches();
    void purgeIfDue();

    std::uint64_t currentFrame() const noexcept { return m_currentFrame; }

    // Cache registration. Caches must unregister in their destructor (before
    // ~ResourceSystem) to avoid dangling pointers. Both are safe to call
    // reentrantly from drainCaches()/purgeIfDue() (e.g. a tile cache
    // attaching/detaching while it streams): adds are deferred and removals
    // tombstone the slot, so the in-progress iteration is never disturbed.
    void registerCache(ResourceCacheBase* cache);
    void unregisterCache(ResourceCacheBase* cache) noexcept;

    // Tunables (safe to call at any time).
    void setUploadBudgetBytes(std::size_t bytes) noexcept { m_uploadBudgetBytes = bytes; }
    void setPurgeIntervalFrames(std::uint64_t frames) noexcept { m_purgeIntervalFrames = frames; }
    void setGraceFrames(std::uint64_t frames) noexcept { m_graceFrames = frames; }

    std::size_t   uploadBudgetBytes() const noexcept { return m_uploadBudgetBytes; }
    std::uint64_t purgeIntervalFrames() const noexcept { return m_purgeIntervalFrames; }
    std::uint64_t graceFrames() const noexcept { return m_graceFrames; }

    unsigned workerCount() const noexcept { return static_cast<unsigned>(m_workers.size()); }

private:
    void workerLoop();

    // Apply deferred register/unregister and compact tombstones after an
    // iteration over m_caches completes.
    void finishCacheIteration();

    // Worker thread pool + work queue.
    mutable std::mutex            m_workMutex;
    std::condition_variable       m_workCv;
    std::queue<DecodeTask>        m_work;
    std::vector<std::thread>      m_workers;
    bool                          m_stop{ false };

    // Caches in the per-frame lifecycle. Render thread only, so no lock.
    // While m_iterating, registerCache() defers into m_pendingCaches and
    // unregisterCache() tombstones the slot; finishCacheIteration() reconciles
    // both so the live iteration is never invalidated.
    std::vector<ResourceCacheBase*> m_caches;
    std::vector<ResourceCacheBase*> m_pendingCaches;
    std::size_t                     m_drainCursor{ 0 };
    bool                            m_iterating{ false };

    // Bumped on the render thread, read by workers only as a hint, so a plain
    // counter suffices.
    std::uint64_t m_currentFrame{ 0 };

    // Tunables — defaults sized for desktop. Mobile callers should tighten.
    std::size_t   m_uploadBudgetBytes{ 8 * 1024 * 1024 }; // 8 MiB per frame
    std::uint64_t m_purgeIntervalFrames{ 600 };           // ~10s at 60 fps
    std::uint64_t m_graceFrames{ 1800 };                  // ~30s at 60 fps
};

} // namespace celestia::engine
