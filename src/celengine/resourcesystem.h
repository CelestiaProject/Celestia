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

// Interface implemented by per-resource-type caches (textures, geometry, ...).
// The ResourceSystem owns the worker thread pool and the per-frame tick; the
// caches own the per-handle entries and the cross-thread ready queue.
//
// All methods on this interface run on the main (render/GL) thread.
class ResourceCacheBase
{
public:
    virtual ~ResourceCacheBase() = default;

    // Drain ready-to-upload entries, performing GL uploads, until at least
    // `byteBudget` bytes have been uploaded or the ready queue is empty.
    // Returns the number of bytes actually uploaded.
    virtual std::size_t drainReady(std::size_t byteBudget) = 0;

    // Evict entries whose last access frame is older than (currentFrame -
    // graceFrames). Implementations must not evict entries that are pinned
    // or currently in-flight on a worker thread.
    virtual void purgeStale(std::uint64_t graceFrames) = 0;
};

// Coordinator for asynchronous resource loading.
//
// Owns a worker thread pool that runs CPU-bound decode tasks. Provides
// per-frame lifecycle hooks that the renderer drives:
//   1. beginFrame()        — bumps the frame counter (call at top of frame)
//   2. drainCaches()       — main-thread GL uploads, bounded by upload budget
//   3. purgeIfDue()        — periodic LRU eviction across all caches
//
// The ResourceSystem itself does no GL work; uploads run inside the caches
// via the ResourceCacheBase interface.
class ResourceSystem : private util::NoCopy
{
public:
    using DecodeTask = std::function<void()>;

    // numWorkers == 0 selects max(1, hardware_concurrency - 1).
    explicit ResourceSystem(unsigned numWorkers = 0);
    ~ResourceSystem();

    // Submit a CPU-bound task to the worker pool. The task is invoked on a
    // worker thread; it is responsible for posting any result back to the
    // owning cache's ready queue. Higher priority values run first; within
    // the same priority, submission order is preserved (FIFO).
    void submit(DecodeTask task, int priority = 0);

    // Per-frame lifecycle. Call beginFrame() once per render frame, before
    // any cache find() calls. Call drainCaches() and purgeIfDue() after the
    // frame's logical work; they are safe to call back-to-back at the top
    // of the next frame as well.
    void beginFrame() noexcept;
    void drainCaches();
    void purgeIfDue();

    std::uint64_t currentFrame() const noexcept { return m_currentFrame; }

    // Cache registration. Caches must call unregisterCache() in their
    // destructor (before ~ResourceSystem) to avoid dangling references.
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
    struct WorkItem
    {
        int           priority;
        std::uint64_t sequence;
        DecodeTask    task;

        // std::priority_queue is a max-heap on operator<. We want higher
        // priority first, then lower sequence (FIFO) within a priority.
        bool operator<(const WorkItem& other) const noexcept
        {
            if (priority != other.priority)
                return priority < other.priority;
            return sequence > other.sequence;
        }
    };

    void workerLoop();

    // Worker thread pool + work queue.
    mutable std::mutex            m_workMutex;
    std::condition_variable       m_workCv;
    std::priority_queue<WorkItem> m_work;
    std::uint64_t                 m_workSeq{ 0 };
    std::vector<std::thread>      m_workers;
    bool                          m_stop{ false };

    // Caches participating in per-frame lifecycle. Touched only from the
    // main thread; no lock needed.
    std::vector<ResourceCacheBase*> m_caches;

    // Per-frame state. m_currentFrame is read by worker threads (e.g., for
    // timestamp metadata) but written only on the main thread; atomic-ish
    // semantics via plain std::uint64_t are acceptable because workers only
    // use the value as a hint.
    std::uint64_t m_currentFrame{ 0 };

    // Tunables — defaults sized for desktop. Mobile callers should tighten.
    std::size_t   m_uploadBudgetBytes{ 8 * 1024 * 1024 }; // 8 MiB per frame
    std::uint64_t m_purgeIntervalFrames{ 600 };           // ~10s at 60 fps
    std::uint64_t m_graceFrames{ 1800 };                  // ~30s at 60 fps
};

} // namespace celestia::engine
