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
#include <celengine/glsupport.h>
#include <celengine/image.h>
#include <celutil/bytes.h>
#include <celutil/debug.h>

using std::ifstream;
using std::ios;

namespace
{
// BMP file definitions--can't use windows.h because we might not be
// built on Windows!
typedef struct
{
    unsigned char magic[2];
    uint32_t size;
    uint32_t reserved;
    uint32_t offset;
} BMPFileHeader;

typedef struct
{
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bpp;
    uint32_t compression;
    uint32_t imageSize;
    int32_t widthPPM;
    int32_t heightPPM;
    uint32_t colorsUsed;
    uint32_t colorsImportant;
} BMPImageHeader;


int32_t readInt32(ifstream& in)
{
    uint8_t b[4];
    in.read(reinterpret_cast<char*>(b), 4);
    int32_t val = ((int32_t) b[3] << 24) +
                  ((int32_t) b[2] << 16) +
                  ((int32_t) b[1] << 8) +
                  ((int32_t) b[0]);
    LE_TO_CPU_INT32(val, val);
    return val;
}

int16_t readInt16(ifstream& in)
{
    uint8_t b[2];
    in.read(reinterpret_cast<char*>(b), 2);
    int16_t val = ((int16_t) b[1] << 8) + (int16_t) b[0];
    LE_TO_CPU_INT16(val, val);
    return val;
}


Image* LoadBMPImage(ifstream& in)
{
    BMPFileHeader fileHeader;
    BMPImageHeader imageHeader;
    uint8_t* pixels;

    in >> fileHeader.magic[0];
    in >> fileHeader.magic[1];
    fileHeader.size = readInt32(in);
    fileHeader.reserved = readInt32(in);
    fileHeader.offset = readInt32(in);

    if (fileHeader.magic[0] != 'B' || fileHeader.magic[1] != 'M')
        return nullptr;

    imageHeader.size = readInt32(in);
    imageHeader.width = readInt32(in);
    imageHeader.height = readInt32(in);
    imageHeader.planes = readInt16(in);
    imageHeader.bpp = readInt16(in);
    imageHeader.compression = readInt32(in);
    imageHeader.imageSize = readInt32(in);
    imageHeader.widthPPM = readInt32(in);
    imageHeader.heightPPM = readInt32(in);
    imageHeader.colorsUsed = readInt32(in);
    imageHeader.colorsImportant = readInt32(in);

    if (imageHeader.width <= 0 || imageHeader.height <= 0)
        return nullptr;

    // We currently don't support compressed BMPs
    if (imageHeader.compression != 0)
        return nullptr;
    // We don't handle 1-, 2-, or 4-bpp images
    if (imageHeader.bpp != 8 && imageHeader.bpp != 24 && imageHeader.bpp != 32)
        return nullptr;

    uint8_t* palette = nullptr;
    if (imageHeader.bpp == 8)
    {
        DPRINTF(LOG_LEVEL_DEBUG, "Reading %u color palette\n", imageHeader.colorsUsed);
        palette = new uint8_t[imageHeader.colorsUsed * 4];
        in.read(reinterpret_cast<char*>(palette), imageHeader.colorsUsed * 4);
    }

    in.seekg(fileHeader.offset, ios::beg);

    uint32_t bytesPerRow =
        (imageHeader.width * imageHeader.bpp / 8 + 1) & ~1;
    uint32_t imageBytes = bytesPerRow * imageHeader.height;

    // slurp the image data
    pixels = new uint8_t[imageBytes];
    in.read(reinterpret_cast<char*>(pixels), imageBytes);

    // check for truncated file

    auto* img = new Image(GL_RGB, imageHeader.width, imageHeader.height);

    // Copy the image and perform any necessary conversions
    for (int32_t y = 0; y < imageHeader.height; y++)
    {
        uint8_t* src = &pixels[y * bytesPerRow];
        uint8_t* dst = img->getPixelRow(y);

        switch (imageHeader.bpp)
        {
        case 8:
            for (int32_t x = 0; x < imageHeader.width; x++)
            {
                uint8_t* color = palette + (*src << 2);
                dst[0] = color[2];
                dst[1] = color[1];
                dst[2] = color[0];
                src++;
                dst += 3;
            }
            break;
        case 24:
            for (int32_t x = 0; x < imageHeader.width; x++)
            {
                dst[0] = src[2];
                dst[1] = src[1];
                dst[2] = src[0];
                src += 3;
                dst += 3;
            }
            break;
        case 32:
            for (int32_t x = 0; x < imageHeader.width; x++)
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

    delete[] pixels;
    delete[] palette;

    return img;
}
} // anonymous namespace

Image* LoadBMPImage(const fs::path& filename)
{
    ifstream bmpFile(filename.string(), ios::in | ios::binary);

    if (bmpFile.good())
    {
        Image* img = LoadBMPImage(bmpFile);
        bmpFile.close();
        return img;
    }

    return nullptr;
}
