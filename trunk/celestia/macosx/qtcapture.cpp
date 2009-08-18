// qtcapture.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cmath>
#include <celutil/debug.h>
#include "qtcapture.h"
#include <GL/glew.h>

using namespace std;


QTCapture::QTCapture() :
    width(-1),
    height(-1),
    frameRate(30.0f),
    frameCounter(0),
    capturing(false)
{
}


QTCapture::~QTCapture()
{
    cleanup();
}


bool QTCapture::start(const string& filename,
                       int w, int h,
                       float fps)
{
    if (capturing)
        return false;

    width = w;
    height = h;
    frameRate = fps;

    capturing = true;
    frameCounter = 0;

    return true;
}


bool QTCapture::end()
{
    capturing = false;
    cleanup();

    return true;
}


bool QTCapture::captureFrame()
{
    if (!capturing)
        return false;

    // Get the dimensions of the current viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    int x = viewport[0] + (viewport[2] - width) / 2;
    int y = viewport[1] + (viewport[3] - height) / 2;

    frameCounter++;

    return true;
}


void QTCapture::cleanup()
{
}


int QTCapture::getWidth() const
{
    return width;
}

int QTCapture::getHeight() const
{
    return height;
}

float QTCapture::getFrameRate() const
{
    return frameRate;
}

int QTCapture::getFrameCount() const
{
    return frameCounter;
}

void QTCapture::setAspectRatio(int aspectNumerator, int aspectDenominator)
{
}

void QTCapture::setQuality(float)
{
}

void QTCapture::recordingStatus(bool started)
{
}
