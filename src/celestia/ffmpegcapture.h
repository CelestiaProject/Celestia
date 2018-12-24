#pragma once

#include "moviecapture.h"
#define __STDC_CONSTANT_MACROS
extern "C"
{
#include <libavformat/avformat.h>
}

#include <celengine/hash.h>

class FFMPEGCapturePrivate;

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
    void recordingStatus(bool) override {};

    void setVideoCodec(AVCodecID);
    void setBitRate(int64_t);
    void setEncoderOptions(const std::string&);

 private:
    FFMPEGCapturePrivate *d{ nullptr };
};
