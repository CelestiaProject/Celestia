// resourcesystem.cpp
//
// Copyright (C) 2026-present, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "resourcesystem.h"

#include <algorithm>
#include <utility>

namespace celestia::engine
{

ResourceSystem::ResourceSystem(unsigned numWorkers)
{
    if (numWorkers == 0)
    {
        const unsigned hw = std::thread::hardware_concurrency();
        numWorkers = hw > 1 ? hw - 1 : 1;
    }

    m_workers.reserve(numWorkers);
    for (unsigned i = 0; i < numWorkers; ++i)
        m_workers.emplace_back(&ResourceSystem::workerLoop, this);
}

ResourceSystem::~ResourceSystem()
{
    shutdown();
}

void
ResourceSystem::shutdown() noexcept
{
    {
        std::scoped_lock lock(m_workMutex);
        if (m_stop)
            return;
        m_stop = true;
    }
    m_workCv.notify_all();
    for (auto& t : m_workers)
    {
        if (t.joinable())
            t.join();
    }
}

void
ResourceSystem::submit(DecodeTask task)
{
    if (!task)
        return;

    {
        std::scoped_lock lock(m_workMutex);
        m_work.push(std::move(task));
    }
    m_workCv.notify_one();
}

void
ResourceSystem::beginFrame() noexcept
{
    ++m_currentFrame;
}

void
ResourceSystem::drainCaches()
{
    const std::size_t budget = m_uploadBudgetBytes;
    const std::size_t count = m_caches.size();
    if (budget == 0 || count == 0)
        return;

    // Rotate the starting cache each frame so a perpetually-busy cache can't
    // monopolise the budget and starve the others. No cache is added or
    // removed during a drain (uploads neither attach nor evict), so the size
    // and indices stay stable across the loop.
    std::size_t remaining = budget;
    const std::size_t start = m_drainCursor % count;
    m_iterating = true;
    for (std::size_t k = 0; k < count; ++k)
    {
        if (remaining == 0)
            break;
        auto* cache = m_caches[(start + k) % count];
        if (cache == nullptr)
            continue;
        const std::size_t uploaded = cache->drainReady(remaining);
        remaining = uploaded >= remaining ? 0 : remaining - uploaded;
    }
    finishCacheIteration();
    ++m_drainCursor;
}

void
ResourceSystem::purgeIfDue()
{
    if (m_purgeIntervalFrames == 0 || m_currentFrame == 0 ||
        (m_currentFrame % m_purgeIntervalFrames) != 0)
        return;

    // Evicting an entry may destroy a resource that is itself a registered
    // cache (e.g. a VirtualTexture), which reentrantly unregisters. The
    // tombstone-and-defer scheme keeps this iteration valid.
    m_iterating = true;
    for (auto* cache : m_caches)
    {
        if (cache != nullptr)
            cache->purgeStale(m_graceFrames);
    }
    finishCacheIteration();
}

void
ResourceSystem::registerCache(ResourceCacheBase* cache)
{
    if (cache == nullptr)
        return;

    auto present = [cache](const std::vector<ResourceCacheBase*>& v)
    {
        return std::find(v.begin(), v.end(), cache) != v.end();
    };
    if (present(m_caches) || present(m_pendingCaches))
        return;

    if (m_iterating)
        m_pendingCaches.push_back(cache);
    else
        m_caches.push_back(cache);
}

void
ResourceSystem::unregisterCache(ResourceCacheBase* cache) noexcept
{
    // Tombstone rather than erase so an in-progress iteration skips the slot
    // instead of dereferencing a freed pointer; finishCacheIteration() compacts
    // the nulls afterwards. When not iterating, drop it outright.
    for (auto*& slot : m_caches)
    {
        if (slot == cache)
            slot = nullptr;
    }
    m_pendingCaches.erase(
        std::remove(m_pendingCaches.begin(), m_pendingCaches.end(), cache),
        m_pendingCaches.end());

    if (!m_iterating)
    {
        m_caches.erase(std::remove(m_caches.begin(), m_caches.end(), nullptr),
                       m_caches.end());
    }
}

void
ResourceSystem::finishCacheIteration()
{
    m_iterating = false;
    m_caches.erase(std::remove(m_caches.begin(), m_caches.end(), nullptr),
                   m_caches.end());
    for (auto* cache : m_pendingCaches)
    {
        if (std::find(m_caches.begin(), m_caches.end(), cache) == m_caches.end())
            m_caches.push_back(cache);
    }
    m_pendingCaches.clear();
}

void
ResourceSystem::workerLoop()
{
    for (;;)
    {
        DecodeTask item;
        {
            std::unique_lock lock(m_workMutex);
            m_workCv.wait(lock, [this]() noexcept { return m_stop || !m_work.empty(); });
            if (m_stop && m_work.empty())
                return;

            item = std::move(m_work.front());
            m_work.pop();
        }

        // Run outside the lock so other workers can pull in parallel.
        item();
    }
}

} // namespace celestia::engine
