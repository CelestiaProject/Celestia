// overlaymanager.cpp
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "overlaymanager.h"

namespace celestia
{

ImageOverlay::Id
OverlayManager::addImage(std::unique_ptr<ImageOverlay>&& image, double currentTime)
{
    if (image == nullptr) return 0;
    image->setStartTime(static_cast<float>(currentTime));
    return m_images.add(std::move(image));
}

bool
OverlayManager::removeImage(ImageOverlay::Id id)
{
    return m_images.remove(id);
}

bool
OverlayManager::setImageSize(ImageOverlay::Id id, float width, float height) const
{
    if (auto* img = m_images.find(id); img != nullptr)
    {
        img->setSize(width, height);
        return true;
    }
    return false;
}

bool
OverlayManager::setImageOffset(ImageOverlay::Id id, float x, float y) const
{
    if (auto* img = m_images.find(id); img != nullptr)
    {
        img->setOffset(x, y);
        return true;
    }
    return false;
}

void
OverlayManager::clearImages() noexcept
{
    m_images.clear();
}

#ifdef USE_FFMPEG

VideoOverlay::Id
OverlayManager::addVideo(std::unique_ptr<VideoOverlay>&& video)
{
    return m_videos.add(std::move(video));
}

bool
OverlayManager::removeVideo(VideoOverlay::Id id)
{
    return m_videos.remove(id);
}

bool
OverlayManager::seekVideo(VideoOverlay::Id id, double seconds) const
{
    if (auto* v = m_videos.find(id); v != nullptr)
    {
        v->seek(seconds);
        return true;
    }
    return false;
}

bool
OverlayManager::pauseVideo(VideoOverlay::Id id) const
{
    if (auto* v = m_videos.find(id); v != nullptr)
    {
        v->pause();
        return true;
    }
    return false;
}

bool
OverlayManager::resumeVideo(VideoOverlay::Id id) const
{
    if (auto* v = m_videos.find(id); v != nullptr)
    {
        v->resume();
        return true;
    }
    return false;
}

bool
OverlayManager::setVideoSize(VideoOverlay::Id id, float width, float height) const
{
    if (auto* v = m_videos.find(id); v != nullptr)
    {
        v->setSize(width, height);
        return true;
    }
    return false;
}

bool
OverlayManager::setVideoOffset(VideoOverlay::Id id, float x, float y) const
{
    if (auto* v = m_videos.find(id); v != nullptr)
    {
        v->setOffset(x, y);
        return true;
    }
    return false;
}

void
OverlayManager::clearVideos() noexcept
{
    m_videos.clear();
}

#endif // USE_FFMPEG

void
OverlayManager::render(int viewportWidth, int viewportHeight, double currentTime, bool enabled)
{
    if (!m_images.empty())
    {
        if (!enabled)
        {
            // Drop the queue rather than letting it accumulate silently
            // while the gate is off.
            m_images.clear();
        }
        else
        {
            auto now = static_cast<float>(currentTime);
            // Drop fully-faded images first so the loop doesn't pile up
            // state across many short-lived overlays; render the survivors
            // in insertion order (newest on top).
            m_images.eraseIf([now](const auto& img) { return img->isExpired(now); });
            for (const auto& img : m_images)
                img->render(now, viewportWidth, viewportHeight);
        }
    }

#ifdef USE_FFMPEG
    if (!m_videos.empty())
    {
        if (!enabled)
        {
            m_videos.clear();
        }
        else
        {
            for (const auto& v : m_videos)
                v->render(currentTime, viewportWidth, viewportHeight);
            // Drop overlays whose non-looping playback has finished.
            m_videos.eraseIf([](const auto& v) { return v->isFinished(); });
        }
    }
#endif
}

} // namespace celestia
