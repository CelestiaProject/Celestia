// qtcapture.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _QTCAPTURE_H_
#define _QTCAPTURE_H_

// #include <vfw.h>
#include "moviecapture.h"


class QTCapture : public MovieCapture
{
 public:
    QTCapture();
    virtual ~QTCapture();

    bool start(const std::string& filename, int w, int h, float fps);
    bool end();
    bool captureFrame();

    int getWidth() const;
    int getHeight() const;
    float getFrameRate() const;
    int getFrameCount() const;

    void setAspectRatio(int aspectNumerator, int aspectDenominator);
    void setQuality(float);
    void recordingStatus(bool started);

 private:
    void cleanup();

 private:
    int width;
    int height;
    float frameRate;
    int frameCounter;
    bool capturing;
};

#endif // _QTCAPTURE_H_
