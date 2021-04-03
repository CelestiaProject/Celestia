// image.cpp
//
// Copyright (C) 2001, Chris Laurel
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <celengine/glsupport.h>
#include <celutil/debug.h>
#include <celutil/filetype.h>
#include <celutil/gettext.h>
#include <celimage/imageformats.h>
#include "image.h"

using namespace std;

namespace
{
// All rows are padded to a size that's a multiple of 4 bytes
int pad(int n)
{
    return (n + 3) & ~0x3;
}

int formatComponents(int fmt)
{
    switch (fmt)
    {
    case GL_RGBA:
    case GL_BGRA_EXT:
        return 4;
    case GL_RGB:
#ifndef GL_ES
    case GL_BGR_EXT:
#endif
        return 3;
    case GL_LUMINANCE_ALPHA:
#ifndef GL_ES
    case GL_DSDT_NV:
#endif
        return 2;
    case GL_ALPHA:
    case GL_LUMINANCE:
        return 1;

    // Compressed formats
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        return 3;
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        return 4;

    // Unknown format
    default:
        return 0;
    }
}

int calcMipLevelSize(int fmt, int w, int h, int mip)
{
    w = max(w >> mip, 1);
    h = max(h >> mip, 1);

    switch (fmt)
    {
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        // 4x4 blocks, 8 bytes per block
        return ((w + 3) / 4) * ((h + 3) / 4) * 8;
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        // 4x4 blocks, 16 bytes per block
        return ((w + 3) / 4) * ((h + 3) / 4) * 16;
    default:
        return h * pad(w * formatComponents(fmt));
    }
}
} // anonymous namespace

Image::Image(int fmt, int w, int h, int mip) :
    width(w),
    height(h),
    mipLevels(mip),
    format(fmt)
{
    components = formatComponents(fmt);
    assert(components != 0);

    pitch = pad(w * components);

    size = 1;
    for (int i = 0; i < mipLevels; i++)
        size += calcMipLevelSize(fmt, w, h, i);
    pixels = new unsigned char[size];
}

Image::~Image()
{
    delete[] pixels;
}

int Image::getWidth() const
{
    return width;
}

int Image::getHeight() const
{
    return height;
}

int Image::getPitch() const
{
    return pitch;
}

int Image::getMipLevelCount() const
{
    return mipLevels;
}

int Image::getSize() const
{
    return size;
}

int Image::getFormat() const
{
    return format;
}

int Image::getComponents() const
{
    return components;
}

unsigned char* Image::getPixels()
{
    return pixels;
}

unsigned char* Image::getPixelRow(int mip, int row)
{
    /*int w = max(width >> mip, 1); Unused*/
    int h = max(height >> mip, 1);
    if (mip >= mipLevels || row >= h)
        return nullptr;

    // Row addressing of compressed textures is not allowed
    if (isCompressed())
        return nullptr;

    return getMipLevel(mip) + row * pitch;
}

unsigned char* Image::getPixelRow(int row)
{
    return getPixelRow(0, row);
}

unsigned char* Image::getMipLevel(int mip)
{
    if (mip >= mipLevels)
        return nullptr;

    int offset = 0;
    for (int i = 0; i < mip; i++)
        offset += calcMipLevelSize(format, width, height, i);

    return pixels + offset;
}

int Image::getMipLevelSize(int mip) const
{
    if (mip >= mipLevels)
        return 0;

    return calcMipLevelSize(format, width, height, mip);
}

bool Image::isCompressed() const
{
    switch (format)
    {
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        return true;
    default:
        return false;
    }
}

bool Image::hasAlpha() const
{
    switch (format)
    {
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
    case GL_RGBA:
    case GL_BGRA_EXT:
    case GL_LUMINANCE_ALPHA:
    case GL_ALPHA:
        return true;
    default:
        return false;
    }
}

/**
 * Convert an input height map to a normal map.  Ideally, a single channel
 * input should be used.  If not, the first color channel of the input image
 * is the one only one used when generating normals.  This produces the
 * expected results for grayscale values in RGB images.
 */
Image* Image::computeNormalMap(float scale, bool wrap) const
{
    // Can't do anything with compressed input; there are probably some other
    // formats that should be rejected as well . . .
    if (isCompressed())
        return nullptr;

    auto* normalMap = new Image(GL_RGBA, width, height);

    unsigned char* nmPixels = normalMap->getPixels();
    int nmPitch = normalMap->getPitch();

    // Compute normals using differences between adjacent texels.
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            int i0 = i;
            int j0 = j;
            int i1 = i - 1;
            int j1 = j - 1;
            if (i1 < 0)
            {
                if (wrap)
                {
                    i1 = height - 1;
                }
                else
                {
                    i0++;
                    i1++;
                }
            }
            if (j1 < 0)
            {
                if (wrap)
                {
                    j1 = width - 1;
                }
                else
                {
                    j0++;
                    j1++;
                }
            }

            auto h00 = (int) pixels[i0 * pitch + j0 * components];
            auto h10 = (int) pixels[i0 * pitch + j1 * components];
            auto h01 = (int) pixels[i1 * pitch + j0 * components];

            float dx = (float) (h10 - h00) * (1.0f / 255.0f) * scale;
            float dy = (float) (h01 - h00) * (1.0f / 255.0f) * scale;

            auto mag = (float) sqrt(dx * dx + dy * dy + 1.0f);
            float rmag = 1.0f / mag;

            int n = i * nmPitch + j * 4;
            nmPixels[n]     = (unsigned char) (128 + 127 * dx * rmag);
            nmPixels[n + 1] = (unsigned char) (128 + 127 * dy * rmag);
            nmPixels[n + 2] = (unsigned char) (128 + 127 * rmag);
            nmPixels[n + 3] = 255;
        }
    }

    return normalMap;
}

Image* LoadImageFromFile(const fs::path& filename)
{
    ContentType type = DetermineFileType(filename);
    Image* img = nullptr;

    fmt::fprintf(clog, _("Loading image from file %s\n"), filename.string());

    switch (type)
    {
    case Content_JPEG:
        img = LoadJPEGImage(filename);
        break;
    case Content_BMP:
        img = LoadBMPImage(filename);
        break;
    case Content_PNG:
        img = LoadPNGImage(filename);
        break;
    case Content_DDS:
    case Content_DXT5NormalMap:
        img = LoadDDSImage(filename);
        break;
    default:
        DPRINTF(LOG_LEVEL_ERROR, "%s: unrecognized or unsupported image file type.\n", filename.string());
        break;
    }

    return img;
}
