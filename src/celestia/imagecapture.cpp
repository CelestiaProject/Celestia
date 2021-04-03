// imagecapture.cpp
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include <memory>
#include <fmt/format.h>
#include <celengine/render.h>
#include <celimage/imageformats.h>
#include <celutil/gettext.h>
#include "imagecapture.h"

using fmt::print;
using std::cerr;
using std::unique_ptr;

bool CaptureBufferToFile(const fs::path& filename,
                         int x, int y,
                         int width, int height,
                         const Renderer *renderer,
                         ContentType type)
{
    if (type != Content_JPEG && type != Content_PNG)
    {
        print(cerr, "Unsupported image type: {}!\n", filename.string());
        return false;
    }

#ifdef GL_ES
    int rowStride = width * 4;
    int imageSize = height * rowStride;
    Renderer::PixelFormat format = Renderer::PixelFormat::RGBA;
#else
    int rowStride = (width * 3 + 3) & ~0x3;
    int imageSize = height * rowStride;
    Renderer::PixelFormat format = Renderer::PixelFormat::RGB;
#endif
    auto pixels = unique_ptr<unsigned char[]>(new unsigned char[imageSize]);

    if (!renderer->captureFrame(x, y, width, height,
                                format, pixels.get(), true))
    {
        print(cerr, _("Unable to capture a frame!\n"));
        return false;
    }

    bool removeAlpha = format == Renderer::PixelFormat::RGBA;

    switch (type)
    {
    case Content_JPEG:
        return SaveJPEGImage(filename, width, height, rowStride, pixels.get(), removeAlpha);
    case Content_PNG:
        return SavePNGImage(filename, width, height, rowStride, pixels.get(), removeAlpha);
    default:
        break;
    }
    return false;
}
