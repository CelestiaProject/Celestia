// asyncresourcecache.h
//
// Copyright (C) 2026-present, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <utility>

#include <celutil/classops.h>
#include "resourcesystem.h"

namespace celestia::engine
{

// Entry lifecycle. Every transition runs on the render thread; workers only
// post results to the ready queue and never touch the state.
//   NotLoaded     -> Queued          (find() miss)
//   Queued        -> Loaded | Failed (drainReady)
//   Loaded/Failed -> NotLoaded       (purgeStale; Failed lets find() retry)
enum class ResourceState : std::uint8_t
{
    NotLoaded,
    Queued,
    Loaded,
    Failed,
};

// Templated cache for an asynchronously-loaded resource type.
//
// Traits must provide the following member typedefs:
//   Handle      - opaque identifier (typically an enum class)
//   Info        - sufficient input for decode() (path, flags, ...)
//   CpuData     - output of decode(), input to upload()
//   GpuResource - the type returned to callers
//
// and the following member functions (on a Traits instance):
//   bool                         getInfo(Handle, Info&) const;
//   std::optional<CpuData>       decode(const Info&) const;     // worker
//   std::unique_ptr<GpuResource> upload(CpuData&&) const;       // render
//   std::size_t                  gpuBytes(const GpuResource&) const;
//   GpuResource*                 placeholder() const noexcept;  // optional fallback
//
// All cache methods run on the render (GL) thread. Workers only touch
// the cache through the ready queue drained in drainReady().
template <typename Traits>
class AsyncResourceCache : public ResourceCacheBase, private util::NoCopy
{
public:
    using Handle      = typename Traits::Handle;
    using Info        = typename Traits::Info;
    using CpuData     = typename Traits::CpuData;
    using GpuResource = typename Traits::GpuResource;

    AsyncResourceCache(ResourceSystem& system, Traits traits) :
        m_system(&system),
        m_traits(std::move(traits))
    {
        m_system->registerCache(this);
    }

    ~AsyncResourceCache() override
    {
        // ResourceSystem::shutdown() joins all workers before any cache is
        // destroyed, so no decode is in flight here.
        m_system->unregisterCache(this);
    }

    // Returns the loaded resource, the placeholder while it loads, or nullptr.
    // Stamps lastUsedFrame so the entry counts as used this frame.
    GpuResource* find(Handle handle)
    {
        if (!isValid(handle))
            return nullptr;

        const std::uint64_t frame = m_system->currentFrame();
        auto [it, inserted] = m_entries.try_emplace(handle);
        Entry& e = it->second;
        e.lastUsedFrame = frame;

        if (inserted)
        {
            Info info;
            if (!m_traits.getInfo(handle, info))
            {
                e.state = ResourceState::Failed;
                return nullptr;
            }
            enqueueDecode(handle, std::move(info));
        }

        if (e.state == ResourceState::Loaded)
            return e.gpu.get();

        return m_traits.placeholder();
    }

    void pin(Handle handle)
    {
        if (!isValid(handle))
            return;
        auto& e = m_entries[handle];
        ++e.pinCount;
        e.lastUsedFrame = m_system->currentFrame();
    }

    void unpin(Handle handle) noexcept
    {
        if (!isValid(handle))
            return;
        auto it = m_entries.find(handle);
        if (it == m_entries.end() || it->second.pinCount == 0)
            return;
        --it->second.pinCount;
    }

    std::size_t drainReady(std::size_t byteBudget) override
    {
        std::size_t uploaded = 0;
        while (uploaded < byteBudget)
        {
            ReadyItem item;
            {
                std::scoped_lock lock(m_readyMutex);
                if (m_ready.empty())
                    break;
                item = std::move(m_ready.front());
                m_ready.pop_front();
            }

            auto it = m_entries.find(item.handle);
            if (it == m_entries.end() || it->second.state != ResourceState::Queued
                || it->second.generation != item.generation)
            {
                // Evicted, replaced, or decoded under a stale generation: drop.
                continue;
            }

            Entry& e = it->second;
            if (!item.cpu.has_value())
            {
                e.state = ResourceState::Failed;
                continue;
            }

            auto gpu = m_traits.upload(std::move(*item.cpu));
            if (!gpu)
            {
                e.state = ResourceState::Failed;
                continue;
            }

            const std::size_t bytes = m_traits.gpuBytes(*gpu);
            e.gpu   = std::move(gpu);
            e.state = ResourceState::Loaded;
            uploaded += bytes;
        }
        return uploaded;
    }

    void purgeStale(std::uint64_t graceFrames) override
    {
        const std::uint64_t now = m_system->currentFrame();
        for (auto it = m_entries.begin(); it != m_entries.end(); )
        {
            const Entry& e = it->second;
            // Only terminal entries are evictable; Queued entries still have a
            // worker task in flight.
            const bool terminal =
                e.state == ResourceState::Loaded ||
                e.state == ResourceState::Failed;
            const bool evictable =
                terminal &&
                e.pinCount == 0 &&
                (now - e.lastUsedFrame) > graceFrames;
            it = evictable ? m_entries.erase(it) : std::next(it);
        }
    }

    // Drop every entry. In-flight decodes finish but their payloads are
    // dropped in drainReady() since the handle is gone. Render thread only.
    void clear()
    {
        m_entries.clear();
        // Invalidate in-flight decodes so their results are dropped rather
        // than uploaded against a re-queued entry.
        ++m_generation;
    }

    // Diagnostic helpers — render thread only.
    std::size_t size() const noexcept { return m_entries.size(); }
    std::size_t pendingReady() const
    {
        std::scoped_lock lock(m_readyMutex);
        return m_ready.size();
    }

private:
    struct Entry
    {
        ResourceState                state{ ResourceState::NotLoaded };
        std::unique_ptr<GpuResource> gpu;
        std::uint64_t                lastUsedFrame{ 0 };
        std::uint32_t                pinCount{ 0 };
        std::uint64_t                generation{ 0 };
    };

    struct ReadyItem
    {
        Handle                 handle{};
        std::optional<CpuData> cpu{};
        std::uint64_t          generation{ 0 };
    };

    static bool isValid(Handle h) noexcept
    {
        // Handle types declare an Invalid enumerator; treat it as a no-op so
        // misuse can't insert junk into the map.
        return h != Handle::Invalid;
    }

    void enqueueDecode(Handle handle, Info info)
    {
        Entry& entry = m_entries[handle];
        entry.state = ResourceState::Queued;
        entry.generation = m_generation;
        const std::uint64_t generation = m_generation;

        // The worker decodes and pushes the result onto m_ready; it never
        // touches m_entries (render thread only). drainReady() drops the
        // payload if the entry was evicted, replaced, or invalidated meanwhile.
        // ~ResourceSystem joins every worker, so `this` outlives the task.
        m_system->submit([this, handle, generation, info = std::move(info)]() mutable
        {
            ReadyItem item;
            item.handle     = handle;
            item.generation = generation;
            item.cpu        = m_traits.decode(info);

            std::scoped_lock lock(m_readyMutex);
            m_ready.push_back(std::move(item));
        });
    }

    ResourceSystem*                  m_system;
    Traits                           m_traits;
    std::unordered_map<Handle, Entry> m_entries;
    std::uint64_t                     m_generation{ 0 };

    mutable std::mutex      m_readyMutex;
    std::deque<ReadyItem>   m_ready;
};

} // namespace celestia::engine
