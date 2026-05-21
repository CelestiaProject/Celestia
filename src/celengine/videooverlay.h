// videooverlay.h
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#ifdef USE_FFMPEG

#include <cstdint>
#include <filesystem>
#include <memory>

class Renderer;

// VideoOverlay plays a video file as a 2D screen overlay using FFmpeg for
// decoding. Frames are decoded synchronously on demand during render() and
// uploaded to a GL texture via glTexSubImage2D. The video loops automatically
// when it reaches EOF. Audio is not handled here.
class VideoOverlay
{
public:
    // Opaque handle assigned by Hud::addVideoOverlay at insertion time.
    // 0 is reserved as "no id."
    using Id = std::uint64_t;

    VideoOverlay(const std::filesystem::path& path, Renderer* renderer);
    ~VideoOverlay();

    VideoOverlay()               = delete;
    VideoOverlay(VideoOverlay&)  = delete;
    VideoOverlay(VideoOverlay&&) = delete;

    // Advance playback to currentTime and draw the current frame. currentTime
    // is Celestia's real-time clock (seconds), matching what ImageOverlay
    // receives.
    void render(double currentTime, int vpWidth, int vpHeight);

    void setOffset(float x, float y) { m_offsetX = x; m_offsetY = y; }
    // Override the drawn size in pixels. 0 on either axis uses the video's
    // native dimension for that axis.
    void setSize(float w, float h) { m_displayWidth = w; m_displayHeight = h; }

    // Seek to `seconds` from the start of the video. Asynchronously signals
    // the decoder thread; the next frame to be displayed will be at or just
    // before the requested time, and the playback clock is then realigned so
    // playback continues from that point. Negative values clamp to 0.
    void seek(double seconds);

    // When true (the default) the video loops back to the beginning on EOF.
    // When false, decoding stops at EOF and the last decoded frame remains
    // on screen until the overlay is removed or seek() is called.
    void setLoop(bool b);

    // Pause/resume playback. While paused the currently-displayed frame stays
    // on screen; the decoder thread continues to fill the ring buffer until
    // it is full and then blocks naturally.  seek() still works while paused
    // and lands on the seeked-to frame without resuming playback.
    void pause();
    void resume();
    bool isPaused() const { return m_paused; }

    // True once a non-looping video has played all the way through and the
    // last frame has been displayed.  Hud uses this to auto-remove the
    // overlay after EOF.  Always false while looping is enabled.
    bool isFinished() const { return m_finished; }

    void setId(Id id) { m_id = id; }
    Id   id()    const { return m_id; }
    bool isValid() const { return m_valid; }

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    // render() helpers, split out to keep render() readable.
    void advanceClock(double currentTime);
    bool pickFrame(double currentTime, double elapsed, bool& ringEmpty);
    void drawRect(int vpWidth, int vpHeight) const;

    float  m_offsetX       { 0.0f };
    float  m_offsetY       { 0.0f };
    float  m_displayWidth  { 0.0f };
    float  m_displayHeight { 0.0f };
    double m_startTime     { -1.0 };  // currentTime of first render() call
    // Frozen elapsed value while paused.  -1 means "not currently paused or
    // pause hasn't been observed by render() yet."
    double m_pauseElapsed  { -1.0 };
    bool   m_paused        { false };
    // Set once by render() when a non-looping video has shown its last
    // frame.  Hud polls this to schedule auto-removal.
    bool   m_finished      { false };
    Id     m_id            { 0 };
    bool   m_valid         { false };
    Renderer* m_renderer   { nullptr };
};

#endif // USE_FFMPEG
