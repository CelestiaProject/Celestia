// bmp.cpp
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <istream>
#include <memory>
#include <type_traits>
#include <vector>

#include <celutil/array_view.h>
#include <celutil/binaryread.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include "image.h"

using celestia::util::GetLogger;

namespace celestia::engine
{
namespace
{

#pragma pack(push, 1)

// BMP file definitions--can't use windows.h because we might not be
// built on Windows!
struct BMPFileHeader
{
    BMPFileHeader() = delete;

    unsigned char magic[2];
    std::uint32_t size;
    std::uint16_t reserved1;
    std::uint16_t reserved2;
    std::uint32_t offset;
};

struct BMPInfoHeader
{
    BMPInfoHeader() = delete;

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
};

#pragma pack(pop)

static_assert(std::is_standard_layout_v<BMPFileHeader>);
static_assert(std::is_standard_layout_v<BMPInfoHeader>);

static_assert(sizeof(BMPInfoHeader) == 40);

// We do not support 16bpp images
constexpr std::array<std::uint32_t, 6> validBpps
{
    1, 2, 4, 8, 24, 32,
};

constexpr std::array<std::size_t, 5> validHeaderSizes
{
    40,  // BITMAPINFOHEADER
    52,  // BITMAPV2INFOHEADER (Adobe Photoshop)
    56,  // BITMAPV3INFOHEADER (Adobe Photoshop)
    108, // BITMAPV4HEADER (Windows NT 4.0, 95 or later)
    124, // BITMAPV5HEADER (GIMP)
};

struct PaletteEntry
{
    std::uint8_t blue;
    std::uint8_t green;
    std::uint8_t red;
    std::uint8_t reserved;
};

static_assert(std::is_trivially_copyable_v<PaletteEntry> && alignof(PaletteEntry) == 1);

struct BMPInfo
{
    std::uint32_t fileSize{ 0 };
    std::uint32_t offset{ 0 };
    std::uint32_t infoSize{ 0 };
    std::int32_t width{ 0 };
    std::int32_t height{ 0 };
    std::uint32_t bpp{ 0 };
    std::uint32_t rowStride{ 0 };
    std::uint32_t imageSize{ 0 };
    std::uint32_t paletteMax{ 0 };
    std::uint32_t paletteCount{ 0 };
    std::vector<PaletteEntry> palette;
};

bool
parseBMPFileHeader(const char* fileHeader, BMPInfo& info, const fs::path& filename)
{
    if (fileHeader[0] != 'B' || fileHeader[1] != 'M')
    {
        GetLogger()->error(_("BMP read failure '{}' - incorrect header bytes\n"), filename);
        return false;
    }

    info.fileSize = util::fromMemoryLE<std::uint32_t>(fileHeader + offsetof(BMPFileHeader, size));
    info.offset = util::fromMemoryLE<std::uint32_t>(fileHeader + offsetof(BMPFileHeader, offset));
    return true;
}

bool
parseBMPInfoHeader(const char* infoHeader,
                   BMPInfo& info,
                   const fs::path& filename)
{
    info.infoSize = util::fromMemoryLE<std::uint32_t>(infoHeader + offsetof(BMPInfoHeader, size));
    if (std::find(validHeaderSizes.begin(), validHeaderSizes.end(), info.infoSize) == validHeaderSizes.end())
    {
        GetLogger()->error(_("BMP read failure '{}' - unsupported header format\n"), filename);
        return false;
    }

    info.width = util::fromMemoryLE<std::int32_t>(infoHeader + offsetof(BMPInfoHeader, width));
    if (info.width <= 0 || info.width > Image::MAX_DIMENSION)
    {
        GetLogger()->error(_("BMP read failure '{}' - width out of range\n"), filename);
        return false;
    }

    info.height = util::fromMemoryLE<std::int32_t>(infoHeader + offsetof(BMPInfoHeader, height));
    if (info.height <= 0 || info.height > Image::MAX_DIMENSION)
    {
        GetLogger()->error(_("BMP read failure '{}' - height out of range\n"), filename);
        return false;
    }

    if (auto planes = util::fromMemoryLE<std::uint16_t>(infoHeader + offsetof(BMPInfoHeader, planes));
        planes != 1)
    {
        GetLogger()->error(_("BMP read failure '{}' - number of planes must be 1\n"), filename);
        return false;
    }

    info.bpp = util::fromMemoryLE<std::uint16_t>(infoHeader + offsetof(BMPInfoHeader, bpp));
    // We don't handle 1-, 2-, or 4-bpp images
    if (std::find(validBpps.begin(), validBpps.end(), info.bpp) == validBpps.end())
    {
        GetLogger()->error(_("BMP read failure '{}' - invalid bits per pixel {}\n"), filename, info.bpp);
        return false;
    }

    // We currently don't support compressed BMPs
    if (auto compression = util::fromMemoryLE<std::uint32_t>(infoHeader + offsetof(BMPInfoHeader, compression));
        compression != 0)
    {
        GetLogger()->error(_("BMP read failure '{}' - compressed images are not supported\n"), filename);
        return false;
    }

    if (info.bpp > 8)
    {
        std::uint32_t factor = info.bpp >> 3;
        info.rowStride = static_cast<std::uint32_t>(info.width) * factor;
    }
    else if (info.bpp == 8)
    {
        info.rowStride = static_cast<std::uint32_t>(info.width);
    }
    else
    {
        std::uint32_t factor = UINT32_C(8) / info.bpp;
        info.rowStride = (static_cast<std::uint32_t>(info.width) + factor - UINT32_C(1)) / factor;
    }

    if (info.rowStride > UINT32_MAX - 3)
    {
        GetLogger()->error(_("BMP read failure '{}' - image too wide\n"), filename);
        return false;
    }

    // round up to nearest DWORD (4 bytes)
    info.rowStride = (info.rowStride + UINT32_C(3)) & ~UINT32_C(3);

    info.imageSize = info.rowStride * static_cast<std::uint32_t>(info.height);
    // Uncompressed bitmaps can have a size of 0
    if (auto size = util::fromMemoryLE<std::uint32_t>(infoHeader + offsetof(BMPInfoHeader, imageSize));
        size != 0 && size != info.imageSize || (info.offset + info.imageSize) < info.fileSize)
    {
        GetLogger()->error(_("BMP read failure '{}' - size mismatch\n"), filename);
        return false;
    }

    if (info.bpp <= 8)
    {
        info.paletteMax = UINT32_C(1) << info.bpp;
        info.paletteCount = util::fromMemoryLE<std::uint32_t>(infoHeader + offsetof(BMPInfoHeader, colorsUsed));
        if (info.paletteCount == 0)
        {
            info.paletteCount = info.paletteMax;
        }
        else if (info.paletteCount > info.paletteMax)
        {
            GetLogger()->error(_("BMP read failure '{}' - palette too large\n"), filename);
            return false;
        }
    }

    return true;
}

bool
loadBMPHeaders(std::istream& in, BMPInfo& info, const fs::path& filename)
{
    std::array<char, sizeof(BMPFileHeader) + sizeof(BMPInfoHeader)> buffer;
    if (!in.read(buffer.data(), buffer.size())) /* Flawfinder: ignore */
    {
        GetLogger()->error(_("BMP read failure '{}' - could not read file headers\n"), filename);
        return false;
    }

    if (!parseBMPFileHeader(buffer.data(), info, filename))
        return false;

    if (!parseBMPInfoHeader(buffer.data() + sizeof(BMPFileHeader), info, filename))
        return false;

    if (info.bpp <= 8)
    {
        if (!in.seekg(sizeof(BMPFileHeader) + info.infoSize, std::ios_base::beg))
        {
            GetLogger()->error(_("BMP read failure '{}' - could not seek to palette\n"), filename);
            return false;
        }

        // Fill palette with magenta to highlight out-of-range indices in the
        // pixel data. We set the palette to the maximum size that can be
        // addressed with the available bits to avoid needing to do range
        // checks during processing.
        info.palette.resize(info.paletteMax, PaletteEntry{ 255, 0, 255, 0 });
        if (!in.read(reinterpret_cast<char*>(info.palette.data()), info.paletteCount * sizeof(PaletteEntry))) /* Flawfinder: ignore */
        {
            GetLogger()->error(_("BMP read failure '{}' - could not read palette\n"), filename);
            return false;
        }
    }

    return true;
}

void
processRow(const std::uint8_t* src,
           std::uint8_t* dst,
           std::int32_t width,
           std::size_t srcOffset)
{
    for (std::int32_t x = 0; x < width; ++x)
    {
        dst[0] = src[2];
        dst[1] = src[1];
        dst[2] = src[0];
        src += srcOffset;
        dst += 3;
    }
}

void
process8BppRow(const std::uint8_t* src,
               std::uint8_t* dst,
               std::int32_t width,
               util::array_view<PaletteEntry> palette)
{
    for (std::int32_t x = 0; x < width; ++x)
    {
        const PaletteEntry& color = palette[*src];
        dst[0] = color.red;
        dst[1] = color.green;
        dst[2] = color.blue;
        ++src;
        dst += 3;
    }
}

void
processLowBppRow(const std::uint8_t* src,
                 std::uint8_t* dst,
                 std::int32_t width,
                 std::uint32_t bpp,
                 util::array_view<PaletteEntry> palette)
{
    assert(bpp == 1 || bpp == 2 || bpp == 4);
    const unsigned int mask = (1U << bpp) - 1U;
    const unsigned int initalShift = 8U - bpp;
    unsigned int shift = initalShift;
    for (std::int32_t x = 0; x < width; ++x)
    {
        unsigned int idx = (*src >> shift) & mask;
        const PaletteEntry& color = palette[idx];
        dst[0] = color.red;
        dst[1] = color.green;
        dst[2] = color.blue;
        if (shift > 0)
        {
            shift -= bpp;
        }
        else
        {
            shift = initalShift;
            ++src;
        }

        dst += 3;
    }
}

std::unique_ptr<Image>
LoadBMPImage(std::istream& in, const fs::path& filename)
{
    BMPInfo info;
    if (!loadBMPHeaders(in, info, filename))
        return nullptr;

    if (!in.seekg(info.offset, std::ios_base::beg))
    {
        GetLogger()->error(_("BMP read failure '{}' - could not seek to image data\n"), filename);
        return nullptr;
    }

    // slurp the image data
    std::vector<std::uint8_t> pixels(info.imageSize);
    if (!in.read(reinterpret_cast<char*>(pixels.data()), info.imageSize)) /* Flawfinder: ignore */
    {
        GetLogger()->error(_("BMP read failure '{}' - could not read image data\n"), filename);
        return nullptr;
    }

    auto img = std::make_unique<Image>(PixelFormat::RGB, info.width, info.height);

    // Copy the image and perform any necessary conversions
    const std::uint8_t* src = pixels.data();
    for (std::int32_t y = info.height; y-- > 0;)
    {
        std::uint8_t* dst = img->getPixelRow(y);

        switch (info.bpp)
        {
        case 1:
        case 2:
        case 4:
            processLowBppRow(src, dst, info.width, info.bpp, info.palette);
            break;

        case 8:
            process8BppRow(src, dst, info.width, info.palette);
            break;

        case 24:
            processRow(src, dst, info.width, 3);
            break;

        case 32:
            processRow(src, dst, info.width, 4);
            break;

        default:
            assert(0);
            break;
        }

        src += info.rowStride;
    }

    return img;
}

} // anonymous namespace

Image*
LoadBMPImage(const fs::path& filename)
{
    std::ifstream bmpFile(filename, std::ios_base::in | std::ios_base::binary);
    if (!bmpFile)
    {
        GetLogger()->error(_("BMP read failure '{}' - could not open file\n"), filename);
        return nullptr;
    }

    return LoadBMPImage(bmpFile, filename).release();
}

} // namespace celestia::engine
