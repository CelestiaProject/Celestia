// avif.cpp
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celengine/image.h>
#include <celutil/debug.h>

extern "C" {
#include <avif/avif.h>
}

using celestia::PixelFormat;

Image* LoadAVIFImage(const fs::path& filename)
{
    avifDecoder* decoder = avifDecoderCreate();
    avifResult result = avifDecoderSetIOFile(decoder, filename.string().c_str());
    if (result != AVIF_RESULT_OK)
    {
        DPRINTF(LOG_LEVEL_ERROR, "Cannot open file for read: '%s'\n", filename);
        avifDecoderDestroy(decoder);
        return nullptr;
    }

    result = avifDecoderParse(decoder);
    if (result != AVIF_RESULT_OK)
    {
        DPRINTF(LOG_LEVEL_ERROR, "Failed to decode image: %s\n", avifResultToString(result));
        avifDecoderDestroy(decoder);
        return nullptr;
    }

    if (avifDecoderNextImage(decoder) != AVIF_RESULT_OK)
    {
        DPRINTF(LOG_LEVEL_ERROR, "No image available: %s\n", filename);
        avifDecoderDestroy(decoder);
        return nullptr;
    }

    avifRGBImage rgb;
    rgb.format = AVIF_RGB_FORMAT_RGBA;
    avifRGBImageSetDefaults(&rgb, decoder->image);

    Image* image = new Image(PixelFormat::RGBA, rgb.width, rgb.height);
    rgb.pixels = image->getPixels();
    rgb.rowBytes = image->getWidth() * image->getComponents();

    if (avifImageYUVToRGB(decoder->image, &rgb) != AVIF_RESULT_OK)
    {
        DPRINTF(LOG_LEVEL_ERROR, "Conversion from YUV failed: %s\n", filename);
        delete image;
        avifDecoderDestroy(decoder);
        return nullptr;
    }

    avifDecoderDestroy(decoder);
    return image;
}
