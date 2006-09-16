// avicapture.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _AVICAPTURE_H_
#define _AVICAPTURE_H_

#include <windowsx.h>
#include <vfw.h>
#include "moviecapture.h"


class AVICapture : public MovieCapture
{
 public:
    AVICapture();
    virtual ~AVICapture();

    bool start(const std::string& filename, int w, int h, float fps);
    bool end();
    bool captureFrame();

    int getWidth() const;
    int getHeight() const;
    float getFrameRate() const;
    int getFrameCount() const;

    // These are unused for now:
    virtual void setAspectRatio(int, int) {};
    virtual void setQuality(float) {};
    virtual void recordingStatus(bool) {};

 private:
    void cleanup();

 private:
    int width;
    int height;
    float frameRate;
    int frameCounter;
    bool capturing;
    PAVIFILE aviFile;
    PAVISTREAM aviStream;
    PAVISTREAM compAviStream;
    unsigned char* image;
};

#endif // _AVICAPTURE_H_
