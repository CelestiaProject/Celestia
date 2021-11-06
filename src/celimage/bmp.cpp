// bmp.cpp
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstdint>
#include <fstream>  // ifstream
#include <iostream> // ios
#include <memory>
#include <vector>

#include <celengine/image.h>
#include <celutil/binaryread.h>
#include <celutil/debug.h>

namespace celutil = celestia::util;

namespace
{
// BMP file definitions--can't use windows.h because we might not be
// built on Windows!
typedef struct
{
    unsigned char magic[2];
    std::uint32_t size;
    std::uint32_t reserved;
    std::uint32_t offset;
} BMPFileHeader;

typedef struct
{
    std::uint32_t size;
    std::int32_t width;
    std::int32_t height;
    std::uint16_t planes;
    std::uint16_t bpp;
    std::uint32_t compression;
    std::uint32_t imageSize;
    std::int32_t widthPPM;
    std::int32_t heightPPM;
    std::uint32_t colorsUsed;
    std::uint32_t colorsImportant;
} BMPImageHeader;


Image* LoadBMPImage(std::istream& in)
{
    BMPFileHeader fileHeader;
    BMPImageHeader imageHeader;

    if (!celutil::readLE<unsigned char>(in, fileHeader.magic[0])
        || fileHeader.magic[0] != 'B'
        || !celutil::readLE<unsigned char>(in, fileHeader.magic[1])
        || fileHeader.magic[1] != 'M'
        || !celutil::readLE<std::uint32_t>(in, fileHeader.size)
        || !celutil::readLE<std::uint32_t>(in, fileHeader.reserved)
        || !celutil::readLE<std::uint32_t>(in, fileHeader.offset)
        || !celutil::readLE<std::uint32_t>(in, imageHeader.size)
        || !celutil::readLE<std::int32_t>(in, imageHeader.width)
        || imageHeader.width <= 0
        || !celutil::readLE<std::int32_t>(in, imageHeader.height)
        || imageHeader.height <= 0
        || !celutil::readLE<std::uint16_t>(in, imageHeader.planes)
        || !celutil::readLE<std::uint16_t>(in, imageHeader.bpp)
        // We don't handle 1-, 2-, or 4-bpp images
        || (imageHeader.bpp != 8 && imageHeader.bpp != 24 && imageHeader.bpp != 32)
        || !celutil::readLE<std::uint32_t>(in, imageHeader.compression)
        // We currently don't support compressed BMPs
        || imageHeader.compression != 0
        || !celutil::readLE<std::uint32_t>(in, imageHeader.imageSize)
        || !celutil::readLE<std::int32_t>(in, imageHeader.widthPPM)
        || !celutil::readLE<std::int32_t>(in, imageHeader.heightPPM)
        || !celutil::readLE<std::uint32_t>(in, imageHeader.colorsUsed)
        || !celutil::readLE<std::uint32_t>(in, imageHeader.colorsImportant))
    {
        return nullptr;
    }

    std::vector<uint8_t> palette;
    if (imageHeader.bpp == 8)
    {
        DPRINTF(LOG_LEVEL_DEBUG, "Reading %u color palette\n", imageHeader.colorsUsed);
        palette.resize(imageHeader.colorsUsed * 4);
        if (!in.read(reinterpret_cast<char*>(palette.data()), imageHeader.colorsUsed * 4).good())
        {
            return nullptr;
        }
    }

    if (!in.seekg(fileHeader.offset, std::ios::beg)) { return nullptr; }

    std::size_t bytesPerRow =
        (imageHeader.width * imageHeader.bpp / 8 + 1) & ~1;
    std::size_t imageBytes = bytesPerRow * imageHeader.height;

    // slurp the image data
    std::vector<std::uint8_t> pixels(imageBytes);
    if (!in.read(reinterpret_cast<char*>(pixels.data()), imageBytes).good())
    {
        return nullptr;
    }

    // check for truncated file

    auto img = std::make_unique<Image>(celestia::PixelFormat::RGB, imageHeader.width, imageHeader.height);

    // Copy the image and perform any necessary conversions
    for (std::int32_t y = 0; y < imageHeader.height; y++)
    {
        const std::uint8_t* src = pixels.data() + y * bytesPerRow;
        uint8_t* dst = img->getPixelRow(y);

        switch (imageHeader.bpp)
        {
        case 8:
            for (std::int32_t x = 0; x < imageHeader.width; x++)
            {
                const uint8_t* color = palette.data() + (*src << 2);
                dst[0] = color[2];
                dst[1] = color[1];
                dst[2] = color[0];
                src++;
                dst += 3;
            }
            break;
        case 24:
            for (std::int32_t x = 0; x < imageHeader.width; x++)
            {
                dst[0] = src[2];
                dst[1] = src[1];
                dst[2] = src[0];
                src += 3;
                dst += 3;
            }
            break;
        case 32:
            for (std::int32_t x = 0; x < imageHeader.width; x++)
            {
                dst[0] = src[2];
                dst[1] = src[1];
                dst[2] = src[0];
                src += 4;
                dst += 3;
            }
            break;
        }
    }

    return img.release();
}
} // anonymous namespace

Image* LoadBMPImage(const fs::path& filename)
{
    std::ifstream bmpFile(filename.string(), std::ios::in | std::ios::binary);

    if (bmpFile.good())
    {
        Image* img = LoadBMPImage(bmpFile);
        bmpFile.close();
        return img;
    }

    return nullptr;
}
