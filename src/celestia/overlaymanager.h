// overlaymanager.h
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include <celengine/imageoverlay.h>
#ifdef USE_FFMPEG
#include <celengine/videooverlay.h>
#endif

namespace celestia
{

// Small owning container for objects that carry their own Id. Hands out a
// fresh, monotonically-increasing id on each insertion (0 is reserved as
// "no id"); supports linear lookup, id-based removal, and predicate-based
// pruning. Used as an implementation detail of OverlayManager.
template <typename T>
class IdentifiedList
{
public:
    using Id = typename T::Id;

    Id add(std::unique_ptr<T>&& item)
    {
        if (item == nullptr) return 0;
        ++m_nextId;
        item->setId(m_nextId);
        m_items.push_back(std::move(item));
        return m_nextId;
    }

    bool remove(Id id)
    {
        if (id == 0) return false;
        auto before = m_items.size();
        m_items.erase(
            std::remove_if(m_items.begin(), m_items.end(),
                           [id](const auto& it) { return it->id() == id; }),
            m_items.end());
        return m_items.size() != before;
    }

    void clear() noexcept { m_items.clear(); }

    T* find(Id id) const
    {
        if (id == 0) return nullptr;
        for (const auto& it : m_items)
        {
            if (it->id() == id)
                return it.get();
        }
        return nullptr;
    }

    template <typename Pred>
    void eraseIf(Pred&& pred)
    {
        m_items.erase(
            std::remove_if(m_items.begin(), m_items.end(), std::forward<Pred>(pred)),
            m_items.end());
    }

    bool empty() const noexcept { return m_items.empty(); }

    auto begin() const noexcept { return m_items.begin(); }
    auto end()   const noexcept { return m_items.end(); }

private:
    std::vector<std::unique_ptr<T>> m_items;
    Id m_nextId{ 0 };
};

// Owns the per-script ImageOverlay and VideoOverlay stacks that Hud used
// to manage inline. Hud holds one of these and forwards script-facing
// add/remove/clear calls to it; render() drives both stacks during
// Hud::renderOverlay.
class OverlayManager
{
public:
    OverlayManager() = default;
    ~OverlayManager() = default;
    OverlayManager(const OverlayManager&) = delete;
    OverlayManager& operator=(const OverlayManager&) = delete;

    // Append an image to the stack and stamp it with `currentTime` as its
    // start. Returns the freshly-assigned id; 0 means the image was null.
    ImageOverlay::Id addImage(std::unique_ptr<ImageOverlay>&&, double currentTime);
    bool removeImage(ImageOverlay::Id);
    bool setImageSize(ImageOverlay::Id, float width, float height) const;
    bool setImageOffset(ImageOverlay::Id, float x, float y) const;
    void clearImages() noexcept;

#ifdef USE_FFMPEG
    VideoOverlay::Id addVideo(std::unique_ptr<VideoOverlay>&&);
    bool removeVideo(VideoOverlay::Id);
    bool seekVideo(VideoOverlay::Id, double seconds) const;
    bool pauseVideo(VideoOverlay::Id) const;
    bool resumeVideo(VideoOverlay::Id) const;
    bool setVideoSize(VideoOverlay::Id, float width, float height) const;
    bool setVideoOffset(VideoOverlay::Id, float x, float y) const;
    void clearVideos() noexcept;
#endif

    // Drive the image + video stacks for one frame. When `enabled` is
    // false (the showOverlayImage preference is off, or no script is
    // running), both stacks are cleared instead of rendered, matching
    // the gating Hud used to do inline.
    void render(int viewportWidth, int viewportHeight, double currentTime, bool enabled);

private:
    IdentifiedList<ImageOverlay> m_images;
#ifdef USE_FFMPEG
    IdentifiedList<VideoOverlay> m_videos;
#endif
};

} // namespace celestia
