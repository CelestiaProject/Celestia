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

namespace celestia::engine
{
namespace
{

// All rows are padded to a size that's a multiple of 4 bytes
std::int32_t
pad(std::int32_t n)
{
    return (n + INT32_C(3)) & ~INT32_C(0x3);
}

std::int32_t
formatComponents(PixelFormat fmt)
{
    switch (fmt)
    {
    case PixelFormat::RGBA:
    case PixelFormat::BGRA:
    case PixelFormat::sRGBA:
        return 4;
    case PixelFormat::RGB:
    case PixelFormat::BGR:
    case PixelFormat::sRGB:
        return 3;
    case PixelFormat::LumAlpha:
    case PixelFormat::sLumAlpha:
        return 2;
    case PixelFormat::Alpha:
    case PixelFormat::Luminance:
    case PixelFormat::sLuminance:
        return 1;

    // Compressed formats
    case PixelFormat::DXT1:
    case PixelFormat::DXT1_sRGBA:
        return 3;
    case PixelFormat::DXT3:
    case PixelFormat::DXT3_sRGBA:
    case PixelFormat::DXT5:
    case PixelFormat::DXT5_sRGBA:
        return 4;

    // Unknown format
    default:
        return 0;
    }
}

int
calcMipLevelSize(PixelFormat fmt, std::int32_t w, std::int32_t h, std::int32_t mip)
{
    w = std::max(w >> mip, INT32_C(1));
    h = std::max(h >> mip, INT32_C(1));

    switch (fmt)
    {
    case PixelFormat::DXT1:
    case PixelFormat::DXT1_sRGBA:
        // 4x4 blocks, 8 bytes per block
        return ((w + 3) / 4) * ((h + 3) / 4) * 8;
    case PixelFormat::DXT3:
    case PixelFormat::DXT3_sRGBA:
    case PixelFormat::DXT5:
    case PixelFormat::DXT5_sRGBA:
        // 4x4 blocks, 16 bytes per block
        return ((w + 3) / 4) * ((h + 3) / 4) * 16;
    default:
        assert(formatComponents(fmt) != 0);
        return h * pad(w * formatComponents(fmt));
    }
}

int
calcTotalMipSize(PixelFormat fmt, std::int32_t w, std::int32_t h, std::int32_t mipLevels)
{
    std::int32_t size = 1;
    for (std::int32_t i = 0; i < mipLevels; i++)
        size += calcMipLevelSize(fmt, w, h, i);
    return size;
}

PixelFormat
getLinearFormat(PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::sRGBA:
        return PixelFormat::RGBA;
    case PixelFormat::sRGB:
        return PixelFormat::RGB;
    case PixelFormat::sRGBA8:
        return PixelFormat::RGBA8;
    case PixelFormat::sRGB8:
        return PixelFormat::RGB8;
    case PixelFormat::sLuminance:
        return PixelFormat::Luminance;
    case PixelFormat::sLumAlpha:
        return PixelFormat::LumAlpha;
    case PixelFormat::DXT1_sRGBA:
        return PixelFormat::DXT1;
    case PixelFormat::DXT3_sRGBA:
        return PixelFormat::DXT3;
    case PixelFormat::DXT5_sRGBA:
        return PixelFormat::DXT5;
    default:
        return format;
    }
}

inline std::tuple<std::int32_t, std::int32_t>
handleEdge(std::int32_t i, std::int32_t size, bool wrap)
{
    assert(i >= 0 && size > 0);
    if (i > 0)
        return {i, i - 1};
    if (wrap)
        return {0, size - 1};
    return {1, 0};
}

} // anonymous namespace

Image::Image(PixelFormat fmt, std::int32_t w, std::int32_t h, std::int32_t mip) :
    width(w),
    height(h),
    mipLevels(mip),
    components(formatComponents(fmt)),
    format(fmt),
    size(calcTotalMipSize(fmt, w, h, mip))
{
    assert(components != 0);
    assert(width > 0 && width <= MAX_DIMENSION);
    assert(height > 0 && height <= MAX_DIMENSION);

    pitch = pad(w * components);
    pixels = std::make_unique<std::uint8_t[]>(size);
}

bool
Image::isValid() const noexcept
{
    return pixels != nullptr;
}

std::int32_t
Image::getWidth() const
{
    return width;
}

std::int32_t
Image::getHeight() const
{
    return height;
}

std::int32_t
Image::getPitch() const
{
    return pitch;
}

std::int32_t
Image::getMipLevelCount() const
{
    return mipLevels;
}

std::int32_t
Image::getSize() const
{
    return size;
}

PixelFormat
Image::getFormat() const
{
    return format;
}

std::int32_t
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
Image::getPixelRow(std::int32_t mip, std::int32_t row)
{
    if (mip >= mipLevels || row >= std::max(height >> mip, 1))
        return nullptr;

    // Row addressing of compressed textures is not allowed
    if (isCompressed())
        return nullptr;

    return getMipLevel(mip) + row * pitch;
}

std::uint8_t*
Image::getPixelRow(std::int32_t row)
{
    return getPixelRow(0, row);
}

std::uint8_t*
Image::getMipLevel(std::int32_t mip)
{
    if (mip >= mipLevels)
        return nullptr;

    std::int32_t offset = 0;
    for (std::int32_t i = 0; i < mip; i++)
        offset += calcMipLevelSize(format, width, height, i);

    return pixels.get() + offset;
}

const std::uint8_t*
Image::getMipLevel(int mip) const
{
    if (mip >= mipLevels)
        return nullptr;

    std::int32_t offset = 0;
    for (std::int32_t i = 0; i < mip; ++i)
        offset += calcMipLevelSize(format, width, height, i);

    return pixels.get() + offset;
}

std::int32_t
Image::getMipLevelSize(std::int32_t mip) const
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
    case PixelFormat::DXT1_sRGBA:
    case PixelFormat::DXT3_sRGBA:
    case PixelFormat::DXT5_sRGBA:
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
    case PixelFormat::DXT3_sRGBA:
    case PixelFormat::DXT5:
    case PixelFormat::DXT5_sRGBA:
    case PixelFormat::RGBA:
    case PixelFormat::BGRA:
    case PixelFormat::LumAlpha:
    case PixelFormat::sLumAlpha:
    case PixelFormat::Alpha:
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
    std::int32_t nmPitch = normalMap->getPitch();

    // Compute normals using differences between adjacent texels.
    for (std::int32_t i = 0; i < height; i++)
    {
        const auto rowBase = i * nmPitch;
        const auto [i0, i1] = handleEdge(i, height, wrap);
        for (std::int32_t j = 0; j < width; j++)
        {
            const auto [j0, j1] = handleEdge(j, width, wrap);

            auto h00 = static_cast<int>(pixels[i0 * pitch + j0 * components]);
            auto h10 = static_cast<int>(pixels[i0 * pitch + j1 * components]);
            auto h01 = static_cast<int>(pixels[i1 * pitch + j0 * components]);

            auto dx = static_cast<float>(h10 - h00) * (1.0f / 255.0f) * scale;
            auto dy = static_cast<float>(h01 - h00) * (1.0f / 255.0f) * scale;

            float rmag = 1.0f / std::sqrt(dx * dx + dy * dy + 1.0f);

            std::int32_t n = rowBase + j * 4;
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

bool Image::canSave(ContentType type)
{
    return type == ContentType::PNG || type == ContentType::JPEG;
}

bool Image::save(const fs::path &path, ContentType type) const
{
    switch (type)
    {
    case ContentType::PNG:
        return SavePNGImage(path, *this);
    case ContentType::JPEG:
        return SaveJPEGImage(path, *this);
    default:
        return false;
    }
}

std::unique_ptr<Image> Image::load(const fs::path& filename)
{
    ContentType type = DetermineFileType(filename);

    util::GetLogger()->verbose(_("Loading image from file {}\n"), filename);
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
        util::GetLogger()->error(_("{}: unrecognized or unsupported image file type.\n"), filename);
        break;
    }

    return std::unique_ptr<Image>(img);
}

} // namespace celestia::engine
