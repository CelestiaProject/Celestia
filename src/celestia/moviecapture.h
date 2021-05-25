#pragma once

#define __STDC_CONSTANT_MACROS
extern "C"
{
#include <libavformat/avformat.h>
}

namespace celestia
{
class MovieCapturePrivate;
class MovieCapture
{
 public:
    MovieCapture(const Renderer *r);
    ~MovieCapture();

    bool start(const fs::path&, int, int, float);
    bool end();
    bool captureFrame();

    int getFrameCount() const;
    int getWidth() const;
    int getHeight() const;
    float getFrameRate() const;

    void setVideoCodec(AVCodecID);
    void setBitRate(int64_t);
    void setEncoderOptions(const std::string&);

 private:
    MovieCapturePrivate *d{ nullptr };
};
}
