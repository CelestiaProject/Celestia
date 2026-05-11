// videooverlay.cpp
//
// Copyright (C) 2026, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "videooverlay.h"

#ifdef USE_VIDEO_OVERLAY

#include <cstdint>

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

// Minimal Texture subclass wrapping a caller-owned GL texture name.
// The GL texture is owned by VideoOverlay::Impl; this wrapper exists only
// to flow through Renderer::drawRectangle without any image data copy.
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

} // namespace

struct VideoOverlay::Impl
{
    AVFormatContext* fmtCtx    { nullptr };
    AVCodecContext*  codecCtx  { nullptr };
    AVFrame*         frame     { nullptr };
    AVFrame*         rgbaFrame { nullptr };
    AVPacket*        packet    { nullptr };
    SwsContext*      swsCtx    { nullptr };
    int         videoStream { -1 };
    int         videoWidth  { 0 };
    int         videoHeight { 0 };
    AVRational  timeBase    {};

    GLuint glName { 0 };
    std::unique_ptr<VideoFrameTexture> texture;

    double currentPts { -1.0 };  // PTS of last decoded frame, in seconds

    ~Impl();

    bool open(const std::filesystem::path& path);
    bool decodeNextFrame();
    void ensureTexture();
    void uploadFrame();
};

VideoOverlay::Impl::~Impl()
{
    texture.reset();
    if (glName != 0)
        glDeleteTextures(1, &glName);

    if (swsCtx)
        sws_freeContext(swsCtx);
    if (rgbaFrame)
    {
        av_freep(&rgbaFrame->data[0]);
        av_frame_free(&rgbaFrame);
    }
    if (frame)
        av_frame_free(&frame);
    if (packet)
        av_packet_free(&packet);
    if (codecCtx)
        avcodec_free_context(&codecCtx);
    if (fmtCtx)
        avformat_close_input(&fmtCtx);
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
    if (!codecCtx)
        return false;

    if (avcodec_parameters_to_context(codecCtx, stream->codecpar) < 0)
        return false;

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
    if (!frame || !rgbaFrame || !packet)
        return false;

    int bufSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, videoWidth, videoHeight, 1);
    auto* buf = static_cast<uint8_t*>(av_malloc(static_cast<size_t>(bufSize)));
    if (!buf)
        return false;

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

    return true;
}

bool VideoOverlay::Impl::decodeNextFrame()
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
        if (ret < 0)
            continue;

        ret = avcodec_receive_frame(codecCtx, frame);
        if (ret == AVERROR(EAGAIN))
            continue;
        if (ret < 0)
            return false;

        sws_scale(swsCtx,
                  frame->data, frame->linesize, 0, videoHeight,
                  rgbaFrame->data, rgbaFrame->linesize);

        if (frame->pts != AV_NOPTS_VALUE)
            currentPts = frame->pts * av_q2d(timeBase);
        else
            currentPts += av_q2d(timeBase);  // fallback: advance by one time-base unit

        return true;
    }
}

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
                 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaFrame->data[0]);

    texture = std::make_unique<VideoFrameTexture>(glName, videoWidth, videoHeight);
}

void VideoOverlay::Impl::uploadFrame()
{
    if (glName == 0)
    {
        ensureTexture();
        return;
    }
    glBindTexture(GL_TEXTURE_2D, glName);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, videoWidth, videoHeight,
                    GL_RGBA, GL_UNSIGNED_BYTE, rgbaFrame->data[0]);
}

VideoOverlay::VideoOverlay(const std::filesystem::path& path, Renderer* renderer)
    : m_impl(std::make_unique<Impl>())
    , m_renderer(renderer)
{
    m_valid = m_impl->open(path);
}

VideoOverlay::~VideoOverlay() = default;

void VideoOverlay::render(double currentTime, int vpWidth, int vpHeight)
{
    if (!m_valid || m_renderer == nullptr) return;

    if (m_startTime < 0.0)
        m_startTime = currentTime;

    double elapsed = currentTime - m_startTime;

    // Decode frames until the video has caught up with elapsed time.
    while (m_impl->currentPts < elapsed)
    {
        if (!m_impl->decodeNextFrame())
        {
            // EOF — seek back to the beginning and loop.
            av_seek_frame(m_impl->fmtCtx, m_impl->videoStream, 0, AVSEEK_FLAG_BACKWARD);
            avcodec_flush_buffers(m_impl->codecCtx);
            m_startTime    = currentTime;
            elapsed        = 0.0;
            m_impl->currentPts = -1.0;
            if (!m_impl->decodeNextFrame())
                return;
        }
        m_impl->uploadFrame();
    }

    if (m_impl->glName == 0) return;

    float xSize = (m_displayWidth  > 0.0f) ? m_displayWidth  : static_cast<float>(m_impl->videoWidth);
    float ySize = (m_displayHeight > 0.0f) ? m_displayHeight : static_cast<float>(m_impl->videoHeight);

    // Positioning mirrors OverlayImage: offset 0 centres, ±1 aligns to edges.
    float left   = (static_cast<float>(vpWidth)  * (1.0f + m_offsetX) - xSize) / 2.0f;
    float bottom = (static_cast<float>(vpHeight) * (1.0f + m_offsetY) - ySize) / 2.0f;

    celestia::Rect r(left, bottom, xSize, ySize);
    r.tex = m_impl->texture.get();
    r.colors.fill(Color::White);
    r.hasColors = true;
    m_renderer->drawRectangle(r, FisheyeOverrideMode::Disabled,
                               m_renderer->getOrthoProjectionMatrix());
}

#endif // USE_VIDEO_OVERLAY
