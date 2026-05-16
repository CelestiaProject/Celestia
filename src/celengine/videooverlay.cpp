// videooverlay.cpp
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "videooverlay.h"

#ifdef USE_FFMPEG

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <celutil/logger.h>
#include "glsupport.h"
#include "rectangle.h"
#include "render.h"
#include "texture.h"

using celestia::util::GetLogger;

namespace
{

class VideoFrameTexture : public Texture
{
public:
    VideoFrameTexture(GLuint glName, int w, int h)
        : Texture(w, h, /*alpha=*/true), m_glName(glName) {}

    TextureTile getTile(int /*lod*/, int /*u*/, int /*v*/) override
    {
        return TextureTile(m_glName, 0.0f, 0.0f, 1.0f, 1.0f);
    }

    void bind() override
    {
        glBindTexture(GL_TEXTURE_2D, m_glName);
    }

private:
    GLuint m_glName;
};

// Number of decoded RGBA frames kept in the ring buffer.  3 gives the decoder
// one slot to write into while the render thread holds one for display and one
// is free as a spare.
constexpr int RING_SIZE = 3;

struct DecodedFrame
{
    std::vector<uint8_t> pixels;
    double pts    { -1.0 };
    bool   ready  { false };
    // Set on the first frame decoded after seeking back to the start so the
    // render thread knows to reset its playback clock.
    bool   looped { false };
};

} // namespace

struct VideoOverlay::Impl
{
    // FFmpeg resources — owned and used exclusively by the decoder thread
    // after startDecoder() is called.
    AVFormatContext* fmtCtx    { nullptr };
    AVCodecContext*  codecCtx  { nullptr };
    AVFrame*         frame     { nullptr };
    AVFrame*         rgbaFrame { nullptr };
    AVPacket*        packet    { nullptr };
    SwsContext*      swsCtx    { nullptr };
    int        videoStream { -1 };
    int        videoWidth  { 0 };
    int        videoHeight { 0 };
    AVRational timeBase    {};

    // Ring buffer shared between the decoder thread (writer) and the render
    // thread (reader).  Access is serialised by `mutex`.
    std::array<DecodedFrame, RING_SIZE> ring;
    // Scratch buffer used by the render thread to copy a frame out of the ring
    // before handing it to GL.  Owned exclusively by the render thread.
    std::vector<uint8_t>    uploadBuffer;
    std::mutex              mutex;
    std::condition_variable slotFree;   // decoder waits here when ring is full

    // GL resources — render thread only.
    GLuint glName { 0 };
    std::unique_ptr<VideoFrameTexture> texture;

    // Decoder thread.
    std::thread       decoderThread;
    std::atomic<bool> stop { false };
    // Pending seek target in seconds; -1 means "no seek requested". The
    // render thread writes, the decoder thread reads-and-clears with
    // exchange(). Wake the decoder via slotFree after a write.
    std::atomic<double> seekRequest { -1.0 };
    // When false (the default), the decoder stops on EOF instead of looping
    // back to the start.  Writable from any thread; the decoder reads it on
    // each EOF.
    std::atomic<bool>   loop { false };
    // True once a non-looping video has reached EOF and the decoder is
    // parked.  Cleared by a seek.  The render thread polls this to decide
    // when playback has actually ended.
    std::atomic<bool>   atEof { false };

    ~Impl();

    bool open(const std::filesystem::path& path);
    void startDecoder();
    void decoderLoop();
    // Decode one video frame into rgbaFrame; returns false at EOF/error.
    // Called from the decoder thread only.
    bool decodeOneFrame(double& outPts);

    // Render thread helpers.
    void ensureTexture();
    void uploadToGL();   // uploads uploadBuffer into the GL texture
};

// ---------------------------------------------------------------------------
// Impl lifecycle
// ---------------------------------------------------------------------------

VideoOverlay::Impl::~Impl()
{
    // Signal the decoder thread to exit and wait for it.
    stop.store(true, std::memory_order_relaxed);
    slotFree.notify_all();
    if (decoderThread.joinable())
        decoderThread.join();

    // GL cleanup (render thread is the caller of ~VideoOverlay).
    texture.reset();
    if (glName != 0)
        glDeleteTextures(1, &glName);

    // FFmpeg cleanup.
    if (swsCtx)   sws_freeContext(swsCtx);
    if (rgbaFrame)
    {
        av_freep(&rgbaFrame->data[0]);
        av_frame_free(&rgbaFrame);
    }
    if (frame)  av_frame_free(&frame);
    if (packet) av_packet_free(&packet);
    if (codecCtx) avcodec_free_context(&codecCtx);
    if (fmtCtx)   avformat_close_input(&fmtCtx);
}

bool VideoOverlay::Impl::open(const std::filesystem::path& path)
{
    if (avformat_open_input(&fmtCtx, path.string().c_str(), nullptr, nullptr) < 0)
    {
        GetLogger()->error("VideoOverlay: failed to open '{}'\n", path.string());
        return false;
    }

    if (avformat_find_stream_info(fmtCtx, nullptr) < 0)
    {
        GetLogger()->error("VideoOverlay: no stream info in '{}'\n", path.string());
        return false;
    }

    videoStream = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (videoStream < 0)
    {
        GetLogger()->error("VideoOverlay: no video stream in '{}'\n", path.string());
        return false;
    }

    AVStream* stream = fmtCtx->streams[videoStream];
    timeBase = stream->time_base;

    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec)
    {
        GetLogger()->error("VideoOverlay: no decoder for codec {}\n",
                           static_cast<int>(stream->codecpar->codec_id));
        return false;
    }

    codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) return false;

    if (avcodec_parameters_to_context(codecCtx, stream->codecpar) < 0) return false;

    if (avcodec_open2(codecCtx, codec, nullptr) < 0)
    {
        GetLogger()->error("VideoOverlay: avcodec_open2 failed\n");
        return false;
    }

    videoWidth  = codecCtx->width;
    videoHeight = codecCtx->height;

    frame     = av_frame_alloc();
    rgbaFrame = av_frame_alloc();
    packet    = av_packet_alloc();
    if (!frame || !rgbaFrame || !packet) return false;

    int bufSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, videoWidth, videoHeight, 1);
    auto* buf = static_cast<uint8_t*>(av_malloc(static_cast<size_t>(bufSize)));
    if (!buf) return false;

    av_image_fill_arrays(rgbaFrame->data, rgbaFrame->linesize,
                         buf, AV_PIX_FMT_RGBA, videoWidth, videoHeight, 1);

    swsCtx = sws_getContext(videoWidth, videoHeight, codecCtx->pix_fmt,
                            videoWidth, videoHeight, AV_PIX_FMT_RGBA,
                            SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!swsCtx)
    {
        GetLogger()->error("VideoOverlay: sws_getContext failed\n");
        return false;
    }

    // Pre-allocate ring-buffer pixel storage and the render-thread upload
    // buffer now that we know the frame size.
    auto frameBytes = static_cast<std::size_t>(videoWidth * videoHeight * 4);
    for (auto& f : ring)
        f.pixels.resize(frameBytes);
    uploadBuffer.resize(frameBytes);

    return true;
}

void VideoOverlay::Impl::startDecoder()
{
    decoderThread = std::thread(&Impl::decoderLoop, this);
}

// ---------------------------------------------------------------------------
// Decoder thread
// ---------------------------------------------------------------------------

bool VideoOverlay::Impl::decodeOneFrame(double& outPts)
{
    while (true)
    {
        int ret = av_read_frame(fmtCtx, packet);
        if (ret < 0)
            return false;  // EOF or read error

        if (packet->stream_index != videoStream)
        {
            av_packet_unref(packet);
            continue;
        }

        ret = avcodec_send_packet(codecCtx, packet);
        av_packet_unref(packet);
        if (ret < 0) continue;

        ret = avcodec_receive_frame(codecCtx, frame);
        if (ret == AVERROR(EAGAIN)) continue;
        if (ret < 0) return false;

        sws_scale(swsCtx,
                  frame->data, frame->linesize, 0, videoHeight,
                  rgbaFrame->data, rgbaFrame->linesize);

        outPts = (frame->pts != AV_NOPTS_VALUE)
            ? frame->pts * av_q2d(timeBase)
            : outPts + av_q2d(timeBase);  // fallback: advance by one time-base unit

        return true;
    }
}

void VideoOverlay::Impl::decoderLoop()
{
    // The very first frame is not considered a "loop" — the render thread
    // initialises its clock on the first render() call independently.
    bool afterSeek = false;
    double lastPts = 0.0;

    while (!stop.load(std::memory_order_relaxed))
    {
        // Handle a pending user-initiated seek before touching the ring.
        // EOF wraparound is handled below at the decode site.
        double pending = seekRequest.exchange(-1.0, std::memory_order_acq_rel);
        if (pending >= 0.0)
        {
            int64_t ts = av_q2d(timeBase) > 0.0
                ? static_cast<int64_t>(pending / av_q2d(timeBase))
                : 0;
            av_seek_frame(fmtCtx, videoStream, ts, AVSEEK_FLAG_BACKWARD);
            avcodec_flush_buffers(codecCtx);
            // Invalidate any frames already in the ring — they belong to the
            // old timeline.  Holding the lock here lets the render thread see
            // a consistent post-seek state on its next pass.
            {
                std::lock_guard<std::mutex> lock(mutex);
                for (auto& f : ring) f.ready = false;
            }
            lastPts   = pending;
            afterSeek = true;
            atEof.store(false, std::memory_order_release);
        }

        // Non-looping video that's hit EOF: park here until a seek arrives
        // or the overlay is being torn down.  No CPU spin, no extra slots
        // consumed.
        if (atEof.load(std::memory_order_acquire))
        {
            std::unique_lock<std::mutex> lock(mutex);
            slotFree.wait(lock, [&]
            {
                return stop.load() ||
                       seekRequest.load(std::memory_order_acquire) >= 0.0;
            });
            continue;
        }

        // Wait until a free slot is available in the ring buffer, or a seek
        // is requested, or we're being torn down.
        DecodedFrame* slot = nullptr;
        {
            std::unique_lock<std::mutex> lock(mutex);
            slotFree.wait(lock, [&]
            {
                if (stop.load()) return true;
                if (seekRequest.load(std::memory_order_acquire) >= 0.0) return true;
                for (auto& f : ring)
                    if (!f.ready) { slot = &f; return true; }
                return false;
            });
        }

        if (stop.load()) break;
        if (slot == nullptr) continue;  // woken by a seek — go process it

        // Decode one frame outside the lock — all FFmpeg resources are
        // exclusively owned by this thread.
        double pts = lastPts;
        if (!decodeOneFrame(pts))
        {
            if (loop.load(std::memory_order_acquire))
            {
                // EOF — seek back to the beginning and flag the next frame.
                av_seek_frame(fmtCtx, videoStream, 0, AVSEEK_FLAG_BACKWARD);
                avcodec_flush_buffers(codecCtx);
                lastPts   = 0.0;
                afterSeek = true;
            }
            else
            {
                // Non-looping: stop producing frames.  The render thread
                // keeps drawing the last uploaded frame until it observes
                // this flag and marks the overlay finished for auto-remove.
                atEof.store(true, std::memory_order_release);
            }
            continue;
        }
        lastPts = pts;

        // Copy the decoded RGBA data into the ring slot and mark it ready.
        // If a seek arrived while we were decoding, drop the just-decoded
        // frame instead of publishing it — the next loop iteration will
        // service the seek.
        if (seekRequest.load(std::memory_order_acquire) >= 0.0)
            continue;
        {
            std::lock_guard<std::mutex> lock(mutex);
            std::memcpy(slot->pixels.data(), rgbaFrame->data[0], slot->pixels.size());
            slot->pts    = pts;
            slot->looped = afterSeek;
            slot->ready  = true;
        }
        afterSeek = false;
    }
}

// ---------------------------------------------------------------------------
// Render thread helpers
// ---------------------------------------------------------------------------

void VideoOverlay::Impl::ensureTexture()
{
    if (glName != 0) return;

    glGenTextures(1, &glName);
    glBindTexture(GL_TEXTURE_2D, glName);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, videoWidth, videoHeight,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, uploadBuffer.data());

    texture = std::make_unique<VideoFrameTexture>(glName, videoWidth, videoHeight);
}

void VideoOverlay::Impl::uploadToGL()
{
    if (glName == 0)
    {
        ensureTexture();
        return;
    }
    glBindTexture(GL_TEXTURE_2D, glName);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, videoWidth, videoHeight,
                    GL_RGBA, GL_UNSIGNED_BYTE, uploadBuffer.data());
}

// ---------------------------------------------------------------------------
// VideoOverlay public API
// ---------------------------------------------------------------------------

VideoOverlay::VideoOverlay(const std::filesystem::path& path, Renderer* renderer)
    : m_impl(std::make_unique<Impl>())
    , m_renderer(renderer)
{
    m_valid = m_impl->open(path);
    if (m_valid)
        m_impl->startDecoder();
}

VideoOverlay::~VideoOverlay() = default;

void VideoOverlay::seek(double seconds)
{
    if (!m_valid) return;
    if (seconds < 0.0) seconds = 0.0;
    m_impl->seekRequest.store(seconds, std::memory_order_release);
    m_impl->slotFree.notify_all();
    // Force the render thread to realign its playback clock on the next
    // frame even if decode finishes before that frame arrives.  Seeking
    // also revives a previously-finished overlay.
    m_startTime = -1.0;
    m_finished  = false;
}

void VideoOverlay::setLoop(bool b)
{
    if (!m_valid) return;
    m_impl->loop.store(b, std::memory_order_release);
    // If we just turned looping back on while parked at EOF, the decoder
    // needs a wake — issue a seek-to-0 which also flushes the EOF state.
    if (b)
        seek(0.0);
}

void VideoOverlay::pause()
{
    if (!m_valid) return;
    m_paused = true;
}

void VideoOverlay::resume()
{
    if (!m_valid) return;
    m_paused       = false;
    m_pauseElapsed = -1.0;
}

void VideoOverlay::render(double currentTime, int vpWidth, int vpHeight)
{
    if (!m_valid || m_renderer == nullptr) return;

    if (m_startTime < 0.0)
        m_startTime = currentTime;

    // While paused, hold the playback clock at the elapsed value it had
    // when pause() was first observed.  Frame selection below still runs
    // so that seek-while-paused can land on the new frame, but with
    // elapsed frozen no untouched-by-seek future frames will match.
    if (m_paused)
    {
        if (m_pauseElapsed < 0.0) m_pauseElapsed = currentTime - m_startTime;
        m_startTime = currentTime - m_pauseElapsed;
    }
    else
    {
        m_pauseElapsed = -1.0;
    }

    double elapsed = currentTime - m_startTime;

    // Find the ready frame with the highest PTS that does not exceed elapsed.
    // Copy its pixels out of the ring buffer so the decoder thread can reuse
    // the slot immediately without contending with the GL upload.
    bool haveFrame = false;
    bool ringEmpty = true;
    {
        std::lock_guard<std::mutex> lock(m_impl->mutex);

        DecodedFrame* best = nullptr;
        for (auto& f : m_impl->ring)
        {
            if (!f.ready) continue;
            ringEmpty = false;
            if (f.pts > elapsed && !f.looped) continue;
            if (best == nullptr || f.pts > best->pts)
                best = &f;
        }

        if (best != nullptr)
        {
            if (best->looped)
            {
                // Align the playback clock so that PTS=0 maps to now.
                m_startTime = currentTime - best->pts;
                // If a seek landed while we're paused, refresh the freeze
                // point so resume() continues from the seeked-to position
                // rather than the old pre-seek one.
                if (m_paused) m_pauseElapsed = best->pts;
            }

            std::memcpy(m_impl->uploadBuffer.data(),
                        best->pixels.data(),
                        best->pixels.size());

            // Free this slot and every older one so the decoder can proceed.
            double uploadedPts = best->pts;
            for (auto& f : m_impl->ring)
                if (f.ready && f.pts <= uploadedPts)
                    f.ready = false;

            haveFrame = true;
        }
    }

    if (haveFrame)
    {
        m_impl->slotFree.notify_all();
        m_impl->ensureTexture();
        m_impl->uploadToGL();
    }

    // A non-looping video is "finished" once the decoder has parked at EOF,
    // the ring is empty, and we've at least uploaded one frame.  Hud uses
    // this to schedule auto-removal of the overlay after EOF.
    if (!m_paused &&
        m_impl->atEof.load(std::memory_order_acquire) &&
        ringEmpty &&
        m_impl->glName != 0)
    {
        m_finished = true;
    }

    if (m_impl->glName == 0) return;

    float xSize = (m_displayWidth  > 0.0f) ? m_displayWidth  : static_cast<float>(m_impl->videoWidth);
    float ySize = (m_displayHeight > 0.0f) ? m_displayHeight : static_cast<float>(m_impl->videoHeight);

    float left   = (static_cast<float>(vpWidth)  * (1.0f + m_offsetX) - xSize) / 2.0f;
    float bottom = (static_cast<float>(vpHeight) * (1.0f + m_offsetY) - ySize) / 2.0f;

    celestia::Rect r(left, bottom, xSize, ySize);
    r.tex = m_impl->texture.get();
    r.colors.fill(Color::White);
    r.hasColors = true;
    m_renderer->drawRectangle(r, FisheyeOverrideMode::Disabled,
                              m_renderer->getOrthoProjectionMatrix());
}

#endif // USE_FFMPEG
