// avicapture.cpp
//
// Copyright (C) 2001-2008, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <windowsx.h>
#include <celengine/pixelformat.h>
#include <celengine/render.h>
#include <celutil/logger.h>
#include "avicapture.h"

using namespace std;
using namespace celestia;
using celestia::util::GetLogger;

AVICapture::AVICapture(const Renderer *r) :
    MovieCapture(r)
{
    AVIFileInit();
}


AVICapture::~AVICapture()
{
    cleanup();
    AVIFileExit();
}


bool AVICapture::start(const fs::path& filename,
                       int w, int h,
                       float fps)
{
    if (capturing)
        return false;

    width = w;
    height = h;
    frameRate = fps;

    if (HIWORD(VideoForWindowsVersion()) < 0x010a)
    {
        // We need to be running on version 1.1 or later
        return false;
    }

    // Compute the width of a row in bytes; pad so that rows are aligned on
    // 4 byte boundaries.
    int rowBytes = (width * 3 + 3) & ~0x3;
    image = new unsigned char[rowBytes * height];

    HRESULT hr = AVIFileOpenW(&aviFile,
                              filename.c_str(),
                              OF_WRITE | OF_CREATE,
                              nullptr);
    if (hr != AVIERR_OK)
    {
        GetLogger()->error("Erroring creating avi file for capture.\n");
        return false;
    }

    AVISTREAMINFO info;
    ZeroMemory(&info, sizeof info);
    info.fccType = streamtypeVIDEO;
    info.fccHandler = 0;
    info.dwScale = 1;
    info.dwRate = (DWORD) floor(frameRate + 0.5f);
    info.dwSuggestedBufferSize = rowBytes * height;
    SetRect(&info.rcFrame, 0, 0, width, height);
    hr = AVIFileCreateStream(aviFile, &aviStream, &info);
    if (hr != AVIERR_OK)
    {
        GetLogger()->error("Error {:08x} creating AVI stream.\n", hr);
        cleanup();
        return false;
    }

    // Display a dialog to allow the user to select compression options
    AVICOMPRESSOPTIONS options;
    AVICOMPRESSOPTIONS* arrOptions[1] = { &options };
    ZeroMemory(&options, sizeof options);

    if (!AVISaveOptions(nullptr, 0, 1, &aviStream,
                        (LPAVICOMPRESSOPTIONS*) &arrOptions))
    {
        // The user either clicked on cancel or there was an error
        cleanup();
        return false;
    }

    hr = AVIMakeCompressedStream(&compAviStream, aviStream, &options, nullptr);
    if (hr != AVIERR_OK)
    {
        GetLogger()->error("Error {:08x} creating compressed AVI stream.\n", hr);
        cleanup();
        return false;
    }

    BITMAPINFOHEADER bi;
    ZeroMemory(&bi, sizeof bi);
    bi.biSize = sizeof bi;
    bi.biWidth = width;
    bi.biHeight = height;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = rowBytes * height;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    hr = AVIStreamSetFormat(compAviStream, 0, &bi, sizeof bi);
    if (hr != AVIERR_OK)
    {
        GetLogger()->error("AVIStreamSetFormat failed: {:08x}\n", hr);
        cleanup();
        return false;
    }

    capturing = true;
    frameCounter = 0;

    return true;
}


bool AVICapture::end()
{
    capturing = false;
    cleanup();

    return true;
}


bool AVICapture::captureFrame()
{
    if (!capturing)
        return false;

    // Get the dimensions of the current viewport
    int x, y, w, h;
    renderer->getViewport(&x, &y, &w, &h);

    x += (w - width) / 2;
    y += (h - height) / 2;
    renderer->captureFrame(x, y, width, height,
                 PixelFormat::BGR,
                 image);

    int rowBytes = (width * 3 + 3) & ~0x3;

    LONG samplesWritten = 0;
    LONG bytesWritten = 0;
    HRESULT hr = AVIStreamWrite(compAviStream,
                                frameCounter,
                                1,
                                image,
                                rowBytes * height,
                                AVIIF_KEYFRAME,
                                &samplesWritten,
                                &bytesWritten);
    if (hr != AVIERR_OK)
    {
        GetLogger()->error("AVIStreamWrite failed on frame {}\n", frameCounter);
        return false;
    }

    // fmt::printf("Writing frame: %d  %d => %d bytes\n",
    //             frameCounter, rowBytes * height, bytesWritten);
    frameCounter++;

    return true;
}


void AVICapture::cleanup()
{
    if (aviStream != nullptr)
    {
        AVIStreamRelease(aviStream);
        aviStream = nullptr;
    }
    if (compAviStream != nullptr)
    {
        AVIStreamRelease(compAviStream);
        compAviStream = nullptr;
    }
    if (aviFile != nullptr)
    {
        AVIFileRelease(aviFile);
        aviFile = nullptr;
    }
    if (image != nullptr)
    {
        delete[] image;
        image = nullptr;
    }
}


int AVICapture::getWidth() const
{
    return width;
}

int AVICapture::getHeight() const
{
    return height;
}

float AVICapture::getFrameRate() const
{
    return frameRate;
}

int AVICapture::getFrameCount() const
{
    return frameCounter;
}
