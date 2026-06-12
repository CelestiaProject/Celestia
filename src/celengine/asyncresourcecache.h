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

// State machine for a cache entry. Transitions occur on specific threads:
//   NotLoaded -> Queued           (main thread, find() miss)
//   Queued    -> Loaded | Failed  (main thread, drainReady)
//   Loaded    -> NotLoaded        (main thread, purgeStale)
//   Failed    -> NotLoaded        (main thread, purgeStale; allows retry on
//                                  the next find())
//
// Worker threads never mutate Entry::state — they only push results onto
// the cross-thread ready queue.
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
//   std::unique_ptr<GpuResource> upload(CpuData&&) const;       // main
//   std::size_t                  gpuBytes(const GpuResource&) const;
//   GpuResource*                 placeholder() const noexcept;  // optional fallback
//
// decode() must NOT throw (exceptions are disabled project-wide); signal
// failure by returning std::nullopt.
//
// All public methods on the cache (find/pin/unpin/drainReady/purgeStale)
// must be called from the main (render/GL) thread. Worker threads only
// touch the cache via the cross-thread ready queue inside drainReady().
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
        m_system->unregisterCache(this);
        // Any in-flight decode tasks may still call deliver() after this
        // returns; the captured weak state on the worker side must guard
        // against that. See enqueueDecode() for the lifetime model.
    }

    // Returns the loaded resource if available, the placeholder while
    // loading, or nullptr if no placeholder is configured and the resource
    // is not yet ready. Stamps lastUsedFrame on every call.
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
        for (;;)
        {
            ReadyItem item;
            {
                std::lock_guard<std::mutex> lock(m_readyMutex);
                if (m_ready.empty())
                    break;
                item = std::move(m_ready.front());
                m_ready.pop_front();
            }

            auto it = m_entries.find(item.handle);
            if (it == m_entries.end() || it->second.state != ResourceState::Queued)
            {
                // Evicted or replaced while in flight: drop the payload.
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

            if (uploaded >= byteBudget)
                break;
        }
        return uploaded;
    }

    void purgeStale(std::uint64_t graceFrames) override
    {
        const std::uint64_t now = m_system->currentFrame();
        for (auto it = m_entries.begin(); it != m_entries.end(); )
        {
            const Entry& e = it->second;
            // Only Loaded and Failed entries are candidates. Queued entries
            // have a worker task that will eventually deliver, and we must
            // not erase them while that pointer-equivalent (the handle) is
            // captured in the in-flight task.
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

    // Diagnostic helpers — main thread only.
    std::size_t size() const noexcept { return m_entries.size(); }
    std::size_t pendingReady() const
    {
        std::lock_guard<std::mutex> lock(m_readyMutex);
        return m_ready.size();
    }

private:
    struct Entry
    {
        ResourceState                state{ ResourceState::NotLoaded };
        std::unique_ptr<GpuResource> gpu;
        std::uint64_t                lastUsedFrame{ 0 };
        std::uint32_t                pinCount{ 0 };
    };

    struct ReadyItem
    {
        Handle                 handle{};
        std::optional<CpuData> cpu{};
    };

    static bool isValid(Handle h) noexcept
    {
        // Convention used throughout celestia::engine: handle types declare
        // an `Invalid` enumerator. Callers should not pass it; we treat it
        // defensively as a no-op so misuse can't insert junk in the map.
        return h != Handle::Invalid;
    }

    void enqueueDecode(Handle handle, Info info)
    {
        m_entries[handle].state = ResourceState::Queued;

        // Worker captures `this`, `handle`, and `info` by value. It does NOT
        // touch m_entries — only the main thread mutates that map. The worker
        // performs CPU decode and pushes the result onto m_ready under
        // m_readyMutex. drainReady() (main thread) reconciles by looking up
        // the entry; if it was evicted or replaced in the meantime, the
        // payload is silently dropped.
        //
        // Lifetime: Renderer owns both this cache and the ResourceSystem.
        // ~ResourceSystem joins all workers before returning, so the cache
        // pointer remains valid for the lifetime of any worker task.
        m_system->submit([this, handle, info = std::move(info)]() mutable
        {
            ReadyItem item;
            item.handle = handle;
            item.cpu    = m_traits.decode(info);  // nullopt on failure

            std::lock_guard<std::mutex> lock(m_readyMutex);
            m_ready.push_back(std::move(item));
        });
    }

    ResourceSystem*                  m_system;
    Traits                           m_traits;
    std::unordered_map<Handle, Entry> m_entries;

    mutable std::mutex      m_readyMutex;
    std::deque<ReadyItem>   m_ready;
};

} // namespace celestia::engine
