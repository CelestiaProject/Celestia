// image.cpp
//
// Copyright (C) 2001, Chris Laurel
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "image.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <tuple>

#include <celutil/filetype.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include "imageformats.h"

using celestia::PixelFormat;
using celestia::util::GetLogger;

namespace
{
// All rows are padded to a size that's a multiple of 4 bytes
int
pad(int n)
{
    return (n + 3) & ~0x3;
}

int
formatComponents(PixelFormat fmt)
{
    switch (fmt)
    {
    case PixelFormat::RGBA:
    case PixelFormat::BGRA:
    case PixelFormat::SRGBA:
        return 4;
    case PixelFormat::RGB:
    case PixelFormat::BGR:
    case PixelFormat::SRGB:
        return 3;
    case PixelFormat::LUM_ALPHA:
    case PixelFormat::SLUM_ALPHA:
        return 2;
    case PixelFormat::ALPHA:
    case PixelFormat::LUMINANCE:
    case PixelFormat::SLUMINANCE:
        return 1;

    // Compressed formats
    case PixelFormat::DXT1:
    case PixelFormat::DXT1_SRGBA:
        return 3;
    case PixelFormat::DXT3:
    case PixelFormat::DXT3_SRGBA:
    case PixelFormat::DXT5:
    case PixelFormat::DXT5_SRGBA:
        return 4;

    // Unknown format
    default:
        return 0;
    }
}

int
calcMipLevelSize(PixelFormat fmt, int w, int h, int mip)
{
    w = std::max(w >> mip, 1);
    h = std::max(h >> mip, 1);

    switch (fmt)
    {
    case PixelFormat::DXT1:
    case PixelFormat::DXT1_SRGBA:
        // 4x4 blocks, 8 bytes per block
        return ((w + 3) / 4) * ((h + 3) / 4) * 8;
    case PixelFormat::DXT3:
    case PixelFormat::DXT3_SRGBA:
    case PixelFormat::DXT5:
    case PixelFormat::DXT5_SRGBA:
        // 4x4 blocks, 16 bytes per block
        return ((w + 3) / 4) * ((h + 3) / 4) * 16;
    default:
        assert(formatComponents(fmt) != 0);
        return h * pad(w * formatComponents(fmt));
    }
}

int
calcTotalMipSize(PixelFormat fmt, int w, int h, int mipLevels)
{
    int size = 1;
    for (int i = 0; i < mipLevels; i++)
        size += calcMipLevelSize(fmt, w, h, i);
    return size;
}

celestia::PixelFormat
getLinearFormat(celestia::PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::SRGBA:
        return PixelFormat::RGBA;
    case PixelFormat::SRGB:
        return PixelFormat::RGB;
    case PixelFormat::SLUM_ALPHA:
        return PixelFormat::LUM_ALPHA;
    case PixelFormat::SLUMINANCE:
        return PixelFormat::LUMINANCE;
    case PixelFormat::DXT1_SRGBA:
        return PixelFormat::DXT1;
    case PixelFormat::DXT3_SRGBA:
        return PixelFormat::DXT3;
    case PixelFormat::DXT5_SRGBA:
        return PixelFormat::DXT5;
    default:
        return format;
    }
}

inline std::tuple<int, int>
handleEdge(int i, int size, bool wrap)
{
    assert(i >= 0 && size > 0);
    if (i > 0)
        return {i, i - 1};
    if (wrap)
        return {0, size - 1};
    return {1, 0};
}

} // anonymous namespace

Image::Image(PixelFormat fmt, int w, int h, int mip) :
    width(w),
    height(h),
    mipLevels(mip),
    components(formatComponents(fmt)),
    format(fmt),
    size(calcTotalMipSize(fmt, w, h, mip))
{
    assert(components != 0);

    pitch = pad(w * components);
    pixels = std::make_unique<std::uint8_t[]>(size);
}

bool
Image::isValid() const noexcept
{
    return pixels != nullptr;
}

int
Image::getWidth() const
{
    return width;
}

int
Image::getHeight() const
{
    return height;
}

int
Image::getPitch() const
{
    return pitch;
}

int
Image::getMipLevelCount() const
{
    return mipLevels;
}

int
Image::getSize() const
{
    return size;
}

PixelFormat
Image::getFormat() const
{
    return format;
}

int
Image::getComponents() const
{
    return components;
}

std::uint8_t*
Image::getPixels()
{
    return pixels.get();
}

const std::uint8_t*
Image::getPixels() const
{
    return pixels.get();
}

std::uint8_t*
Image::getPixelRow(int mip, int row)
{
    if (mip >= mipLevels || row >= std::max(height >> mip, 1))
        return nullptr;

    // Row addressing of compressed textures is not allowed
    if (isCompressed())
        return nullptr;

    return getMipLevel(mip) + row * pitch;
}

std::uint8_t*
Image::getPixelRow(int row)
{
    return getPixelRow(0, row);
}

std::uint8_t*
Image::getMipLevel(int mip)
{
    if (mip >= mipLevels)
        return nullptr;

    int offset = 0;
    for (int i = 0; i < mip; i++)
        offset += calcMipLevelSize(format, width, height, i);

    return pixels.get() + offset;
}

const std::uint8_t*
Image::getMipLevel(int mip) const
{
    if (mip >= mipLevels)
        return nullptr;

    int offset = 0;
    for (int i = 0; i < mip; ++i)
        offset += calcMipLevelSize(format, width, height, i);

    return pixels.get() + offset;
}

int
Image::getMipLevelSize(int mip) const
{
    if (mip >= mipLevels)
        return 0;

    return calcMipLevelSize(format, width, height, mip);
}

bool
Image::isCompressed() const
{
    switch (format)
    {
    case PixelFormat::DXT1:
    case PixelFormat::DXT3:
    case PixelFormat::DXT5:
    case PixelFormat::DXT1_SRGBA:
    case PixelFormat::DXT3_SRGBA:
    case PixelFormat::DXT5_SRGBA:
        return true;
    default:
        return false;
    }
}

bool
Image::hasAlpha() const
{
    switch (format)
    {
    case PixelFormat::DXT3:
    case PixelFormat::DXT3_SRGBA:
    case PixelFormat::DXT5:
    case PixelFormat::DXT5_SRGBA:
    case PixelFormat::RGBA:
    case PixelFormat::BGRA:
    case PixelFormat::LUM_ALPHA:
    case PixelFormat::SLUM_ALPHA:
    case PixelFormat::ALPHA:
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
std::unique_ptr<Image>
Image::computeNormalMap(float scale, bool wrap) const
{
    // Can't do anything with compressed input; there are probably some other
    // formats that should be rejected as well . . .
    if (isCompressed())
        return nullptr;

    auto normalMap = std::make_unique<Image>(PixelFormat::RGBA, width, height);

    std::uint8_t* nmPixels = normalMap->getPixels();
    int nmPitch = normalMap->getPitch();

    // Compute normals using differences between adjacent texels.
    for (int i = 0; i < height; i++)
    {
        const auto rowBase = i * nmPitch;
        const auto [i0, i1] = handleEdge(i, height, wrap);
        for (int j = 0; j < width; j++)
        {
            const auto [j0, j1] = handleEdge(j, width, wrap);

            auto h00 = static_cast<int>(pixels[i0 * pitch + j0 * components]);
            auto h10 = static_cast<int>(pixels[i0 * pitch + j1 * components]);
            auto h01 = static_cast<int>(pixels[i1 * pitch + j0 * components]);

            auto dx = static_cast<float>(h10 - h00) * (1.0f / 255.0f) * scale;
            auto dy = static_cast<float>(h01 - h00) * (1.0f / 255.0f) * scale;

            float rmag = 1.0f / std::sqrt(dx * dx + dy * dy + 1.0f);

            int n = rowBase + j * 4;
            nmPixels[n]     = static_cast<std::uint8_t>(128 + 127 * dx * rmag);
            nmPixels[n + 1] = static_cast<std::uint8_t>(128 + 127 * dy * rmag);
            nmPixels[n + 2] = static_cast<std::uint8_t>(128 + 127 * rmag);
            nmPixels[n + 3] = 255;
        }
    }

    return normalMap;
}

void Image::forceLinear()
{
    format = getLinearFormat(format);
}

std::unique_ptr<Image> LoadImageFromFile(const fs::path& filename)
{
    ContentType type = DetermineFileType(filename);

    GetLogger()->verbose(_("Loading image from file {}\n"), filename);
    Image* img = nullptr;

    switch (type)
    {
    case ContentType::JPEG:
        img = LoadJPEGImage(filename);
        break;
    case ContentType::BMP:
        img = LoadBMPImage(filename);
        break;
    case ContentType::PNG:
        img = LoadPNGImage(filename);
        break;
#ifdef USE_LIBAVIF
    case ContentType::AVIF:
        img = LoadAVIFImage(filename);
        break;
#endif
    case ContentType::DDS:
    case ContentType::DXT5NormalMap:
        img = LoadDDSImage(filename);
        break;
    default:
        GetLogger()->error("{}: unrecognized or unsupported image file type.\n", filename);
        break;
    }

    return std::unique_ptr<Image>(img);
}
