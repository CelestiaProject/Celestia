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
    {
        std::lock_guard<std::mutex> lock(m_workMutex);
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
ResourceSystem::submit(DecodeTask task, int priority)
{
    if (!task)
        return;

    {
        std::lock_guard<std::mutex> lock(m_workMutex);
        m_work.push(WorkItem{ priority, ++m_workSeq, std::move(task) });
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
    if (budget == 0 || m_caches.empty())
        return;

    // Round-robin caches so no single type starves the others when the
    // budget is tight. Caches share the budget for the frame.
    std::size_t remaining = budget;
    for (auto* cache : m_caches)
    {
        if (remaining == 0)
            break;
        const std::size_t uploaded = cache->drainReady(remaining);
        remaining = uploaded >= remaining ? 0 : remaining - uploaded;
    }
}

void
ResourceSystem::purgeIfDue()
{
    if (m_purgeIntervalFrames == 0)
        return;
    if (m_currentFrame == 0 || (m_currentFrame % m_purgeIntervalFrames) != 0)
        return;

    for (auto* cache : m_caches)
        cache->purgeStale(m_graceFrames);
}

void
ResourceSystem::registerCache(ResourceCacheBase* cache)
{
    if (cache == nullptr)
        return;
    if (std::find(m_caches.begin(), m_caches.end(), cache) == m_caches.end())
        m_caches.push_back(cache);
}

void
ResourceSystem::unregisterCache(ResourceCacheBase* cache) noexcept
{
    m_caches.erase(std::remove(m_caches.begin(), m_caches.end(), cache), m_caches.end());
}

void
ResourceSystem::workerLoop()
{
    for (;;)
    {
        WorkItem item;
        {
            std::unique_lock<std::mutex> lock(m_workMutex);
            m_workCv.wait(lock, [this]() noexcept { return m_stop || !m_work.empty(); });
            if (m_stop && m_work.empty())
                return;

            // priority_queue::top() returns const&; we need to move the task
            // out before pop(), so const_cast the held DecodeTask.
            item.priority = m_work.top().priority;
            item.sequence = m_work.top().sequence;
            item.task     = std::move(const_cast<DecodeTask&>(m_work.top().task));
            m_work.pop();
        }

        // Run the task outside the lock so other workers can pull in parallel.
        // Tasks must not throw — exceptions are disabled project-wide.
        item.task();
    }
}

} // namespace celestia::engine
