#pragma once

#include <cstdint>
#include <string>

extern "C"
{
#include <libavformat/avformat.h>
}

#include <celcompat/filesystem.h>
#include "moviecapture.h"

class FFMPEGCapturePrivate;
class Renderer;

class FFMPEGCapture : public MovieCapture
{
public:
    FFMPEGCapture(const Renderer *r);
    ~FFMPEGCapture() override;

    bool start(const fs::path&, int, int, float) override;
    bool end() override;
    bool captureFrame() override;

    int getFrameCount() const override;
    int getWidth() const override;
    int getHeight() const override;
    float getFrameRate() const override;

    void setAspectRatio(int, int) override {};
    void setQuality(float) override {};

    void setVideoCodec(AVCodecID);
    void setBitRate(std::int64_t);
    void setEncoderOptions(const std::string&);

protected:
    void recordingStatusUpdated(bool) override { /* no action necessary */ };

private:
    FFMPEGCapturePrivate *d{ nullptr };
};
