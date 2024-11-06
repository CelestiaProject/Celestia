// dds.cpp
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include <cstring>
#include <fstream>
#include <algorithm>
#include <memory>
#include <celengine/glsupport.h>
#include <celutil/logger.h>
#include <celutil/bytes.h>
#include "dds_decompress.h"
#include "image.h"

namespace celestia::engine
{
namespace
{

struct DDPixelFormat
{
    std::uint32_t size;
    std::uint32_t flags;
    std::uint32_t fourCC;
    std::uint32_t bpp;
    std::uint32_t redMask;
    std::uint32_t greenMask;
    std::uint32_t blueMask;
    std::uint32_t alphaMask;
};

struct DDSCaps
{
    std::uint32_t caps;
    std::uint32_t caps2;
    std::uint32_t caps3;
    std::uint32_t caps4;
};

struct DDColorKey
{
    std::uint32_t lowVal;
    std::uint32_t highVal;
};

struct DDSurfaceDesc
{
    std::uint32_t size;
    std::uint32_t flags;
    std::uint32_t height;
    std::uint32_t width;
    std::uint32_t pitch;
    std::uint32_t depth;
    std::uint32_t mipMapLevels;
    std::uint32_t alphaBitDepth;
    std::uint32_t reserved;
    std::uint32_t surface;

    DDColorKey ckDestOverlay;
    DDColorKey ckDestBlt;
    DDColorKey ckSrcOverlay;
    DDColorKey ckSrcBlt;

    DDPixelFormat format;
    DDSCaps caps;

    std::uint32_t textureStage;
};

constexpr std::size_t DDS_MAX_BLOCK_SIZE = 16;

constexpr std::uint32_t FourCC(const char *s)
{
    return static_cast<std::uint32_t>(s[3]) << 24 |
           static_cast<std::uint32_t>(s[2]) << 16 |
           static_cast<std::uint32_t>(s[1]) << 8 |
           static_cast<std::uint32_t>(s[0]);
}

constexpr bool
IsCompressedFormat(PixelFormat format)
{
    return format == PixelFormat::DXT1 ||
           format == PixelFormat::DXT3 ||
           format == PixelFormat::DXT5;
}

PixelFormat
GetUncompressedFormat(const DDSurfaceDesc& ddsd)
{
    util::GetLogger()->debug("DDS Format: {:08x}\n", ddsd.format.fourCC);
    switch (ddsd.format.bpp)
    {
    case 24:
        if (ddsd.format.redMask   == 0x000000ff &&
            ddsd.format.greenMask == 0x0000ff00 &&
            ddsd.format.blueMask  == 0x00ff0000)
        {
            return PixelFormat::RGB8;
        }

#ifndef GL_ES
        if (ddsd.format.redMask           == 0x00ff0000 &&
                    ddsd.format.greenMask == 0x0000ff00 &&
                    ddsd.format.blueMask  == 0x000000ff)
        {
            return PixelFormat::BGR8;
        }
#endif

        break;

    case 32:
        if (ddsd.format.redMask   == 0x00ff0000 &&
            ddsd.format.greenMask == 0x0000ff00 &&
            ddsd.format.blueMask  == 0x000000ff &&
            ddsd.format.alphaMask == 0xff000000)
        {
            return PixelFormat::BGRA8;
        }

        if (ddsd.format.redMask           == 0x000000ff &&
                    ddsd.format.greenMask == 0x0000ff00 &&
                    ddsd.format.blueMask  == 0x00ff0000 &&
                    ddsd.format.alphaMask == 0xff000000)
        {
            return PixelFormat::RGBA8;
        }

        break;

    default:
        break;
    }

    return PixelFormat::Invalid;
}

PixelFormat
GetFormat(const DDSurfaceDesc& ddsd)
{
    switch (ddsd.format.fourCC)
    {
    case 0:
        return GetUncompressedFormat(ddsd);

    case FourCC("DXT1"):
        return PixelFormat::DXT1;

    case FourCC("DXT3"):
        return PixelFormat::DXT3;

    case FourCC("DXT5"):
        return PixelFormat::DXT5;

    default:
        util::GetLogger()->error("Unknown FourCC in DDS file: {:08x}\n", ddsd.format.fourCC);
        return PixelFormat::Invalid;
    }
}

// decompress a DXTc texture to a RGBA texture, taken from https://github.com/ptitSeb/gl4es
std::unique_ptr<std::uint32_t[]>
DecompressDXTc(std::uint32_t width, std::uint32_t height, PixelFormat format, bool transparent0, std::istream& in)
{
    // TODO: check with the size of the input data stream if the stream is in fact decompressed
    // alloc memory
    int blocksize = 0;
    switch (format)
    {
    case PixelFormat::DXT1:
        blocksize = 8;
        break;
    case PixelFormat::DXT3:
    case PixelFormat::DXT5:
        blocksize = 16;
        break;
    default:
        assert(0);
        return nullptr;
    }
    auto pixels = std::make_unique<std::uint32_t[]>(((width + 3) & ~3) * ((height + 3) & ~3));
    std::array<std::uint8_t, DDS_MAX_BLOCK_SIZE> block; // enough to hold DXT1/3/5 blocks
    for (std::uint32_t y = 0; y < height; y += 4)
    {
        for (std::uint32_t x = 0; x < width; x += 4)
        {
            if (!in.good())
                return nullptr;

            in.read(reinterpret_cast<char*>(block.data()), blocksize); /* Flawfinder: ignore */
            switch (format)
            {
            case PixelFormat::DXT1:
                DecompressBlockDXT1(x, y, width, block.data(), transparent0, pixels.get());
                break;
            case PixelFormat::DXT3:
                DecompressBlockDXT3(x, y, width, block.data(), transparent0, pixels.get());
                break;
            case PixelFormat::DXT5:
                DecompressBlockDXT5(x, y, width, block.data(), transparent0, pixels.get());
                break;
            default:
                assert(0);
                return nullptr;
            }
        }
    }
    return pixels;
}

std::unique_ptr<Image>
CreateDecompressedImage(const DDSurfaceDesc& ddsd, PixelFormat format, std::istream& in, const fs::path& filename)
{
    // DXTc texture not supported, decompress DXTc to RGB/RGBA
    std::unique_ptr<std::uint32_t[]>pixels = nullptr;
    bool transparent0 = format == PixelFormat::DXT1;
    if ((ddsd.width & 3) != 0 || (ddsd.height & 3) != 0)
    {
        std::uint32_t nw = std::max(ddsd.width, 4u);
        std::uint32_t nh = std::max(ddsd.height, 4u);
        auto tmp = DecompressDXTc(nw, nh, format, transparent0, in);
        if (tmp != nullptr)
        {
            pixels = std::make_unique<std::uint32_t[]>(ddsd.width * ddsd.height);
            // crop
            for (std::uint32_t y = 0; y < ddsd.height; y++)
            {
                std::memcpy(pixels.get() + y * ddsd.width, tmp.get() + y * nw, ddsd.width * 4);
            }
        }
    }
    else
    {
        pixels = DecompressDXTc(ddsd.width, ddsd.height, format, transparent0, in);
    }

    if (pixels == nullptr)
    {
        util::GetLogger()->error("Failed to decompress DDS texture file {}.\n", filename);
        return nullptr;
    }

    if (transparent0)
    {
        // Remove the alpha channel for DXT1 since DXT1 textures
        // are deemed not to contain alpha values in Celestia
        // https://github.com/CelestiaProject/Celestia/pull/1086
        char *ptr = reinterpret_cast<char*>(pixels.get());
        std::uint32_t numberOfPixels = ddsd.width * ddsd.height;
        for (std::uint32_t index = 0; index < numberOfPixels; ++index)
        {
            std::memcpy(&ptr[3 * index], &ptr[4 * index], sizeof(char) * 3);
        }
    }

    auto img = std::make_unique<Image>(transparent0 ? PixelFormat::RGB : PixelFormat::RGBA, ddsd.width, ddsd.height);
    std::memcpy(img->getPixels(), pixels.get(), (transparent0 ? 3 : 4) * ddsd.width * ddsd.height);
    return img;
}

} // anonymous namespace

Image* LoadDDSImage(const fs::path& filename)
{
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (!in.good())
    {
        util::GetLogger()->error("Error opening DDS texture file {}.\n", filename);
        return nullptr;
    }

    char header[4];
    if (!in.read(header, sizeof(header)).good() /* Flawfinder: ignore */
        || header[0] != 'D' || header[1] != 'D'
        || header[2] != 'S' || header[3] != ' ')
    {
        util::GetLogger()->error("DDS texture file {} has bad header.\n", filename);
        return nullptr;
    }

    DDSurfaceDesc ddsd;
    if (!in.read(reinterpret_cast<char*>(&ddsd), sizeof ddsd).good()) /* Flawfinder: ignore */
    {
        util::GetLogger()->error("DDS file {} has bad surface desc.\n", filename);
        return nullptr;
    }
    LE_TO_CPU_INT32(ddsd.size, ddsd.size);
    LE_TO_CPU_INT32(ddsd.pitch, ddsd.pitch);
    LE_TO_CPU_INT32(ddsd.width, ddsd.width);
    LE_TO_CPU_INT32(ddsd.height, ddsd.height);
    LE_TO_CPU_INT32(ddsd.mipMapLevels, ddsd.mipMapLevels);
    LE_TO_CPU_INT32(ddsd.format.flags, ddsd.format.flags);
    LE_TO_CPU_INT32(ddsd.format.redMask, ddsd.format.redMask);
    LE_TO_CPU_INT32(ddsd.format.greenMask, ddsd.format.greenMask);
    LE_TO_CPU_INT32(ddsd.format.blueMask, ddsd.format.blueMask);
    LE_TO_CPU_INT32(ddsd.format.alphaMask, ddsd.format.alphaMask);
    LE_TO_CPU_INT32(ddsd.format.bpp, ddsd.format.bpp);
    LE_TO_CPU_INT32(ddsd.format.fourCC, ddsd.format.fourCC);

    if (ddsd.width == 0 || ddsd.width > Image::MAX_DIMENSION ||
        ddsd.height == 0 || ddsd.height > Image::MAX_DIMENSION)
    {
        util::GetLogger()->error("DDS file {} size out of range.\n", filename);
        return nullptr;
    }

    PixelFormat format = GetFormat(ddsd);

    if (format == PixelFormat::Invalid)
    {
        util::GetLogger()->error("Unsupported format for DDS texture file {}.\n", filename);
        return nullptr;
    }

    // Check if the platform supports compressed DTXc textures
    if (IsCompressedFormat(format) && !gl::EXT_texture_compression_s3tc)
        return CreateDecompressedImage(ddsd, format, in, filename).release();

    // TODO: Verify that the reported texture size matches the amount of
    // data expected.
    auto img = std::make_unique<Image>(format,
                                       static_cast<std::int32_t>(ddsd.width),
                                       static_cast<std::int32_t>(ddsd.height),
                                       std::max(static_cast<std::int32_t>(ddsd.mipMapLevels), INT32_C(1)));
    if (!in.read(reinterpret_cast<char*>(img->getPixels()), img->getSize())) /* Flawfinder: ignore */
    {
        util::GetLogger()->error("Failed reading data from DDS texture file {}.\n", filename);
        return nullptr;
    }

    return img.release();
}

} // namespace celestia::engine
