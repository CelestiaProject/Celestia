// dds.cpp
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <array>
#include <cassert>
#include <cstddef>
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
    util::LE<std::uint32_t> size;
    util::LE<std::uint32_t> flags;
    util::LE<std::uint32_t> fourCC;
    util::LE<std::uint32_t> bpp;
    util::LE<std::uint32_t> redMask;
    util::LE<std::uint32_t> greenMask;
    util::LE<std::uint32_t> blueMask;
    util::LE<std::uint32_t> alphaMask;
};

struct DDSCaps
{
    util::LE<std::uint32_t> caps;
    util::LE<std::uint32_t> caps2;
    util::LE<std::uint32_t> caps3;
    util::LE<std::uint32_t> caps4;
};

struct DDColorKey
{
    util::LE<std::uint32_t> lowVal;
    util::LE<std::uint32_t> highVal;
};

struct DDSurfaceDesc
{
    util::LE<std::uint32_t> size;
    util::LE<std::uint32_t> flags;
    util::LE<std::uint32_t> height;
    util::LE<std::uint32_t> width;
    util::LE<std::uint32_t> pitch;
    util::LE<std::uint32_t> depth;
    util::LE<std::uint32_t> mipMapLevels;
    util::LE<std::uint32_t> alphaBitDepth;
    util::LE<std::uint32_t> reserved;
    util::LE<std::uint32_t> surface;

    DDColorKey ckDestOverlay;
    DDColorKey ckDestBlt;
    DDColorKey ckSrcOverlay;
    DDColorKey ckSrcBlt;

    DDPixelFormat format;
    DDSCaps caps;

    util::LE<std::uint32_t> textureStage;
};

// DX10 extended header, present when fourCC == "DX10".
struct DDSHeaderDXT10
{
    util::LE<std::uint32_t> dxgiFormat;
    util::LE<std::uint32_t> resourceDimension;
    util::LE<std::uint32_t> miscFlag;
    util::LE<std::uint32_t> arraySize;
    util::LE<std::uint32_t> miscFlags2;
};

// Subset of DXGI_FORMAT values relevant to Celestia.
enum class DXGIFormat : std::uint32_t
{
    BC1_UNORM      = 71,
    BC1_UNORM_SRGB = 72,
    BC2_UNORM      = 74,
    BC2_UNORM_SRGB = 75,
    BC3_UNORM      = 77,
    BC3_UNORM_SRGB = 78,
    BC7_UNORM      = 98,
    BC7_UNORM_SRGB = 99,
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
           format == PixelFormat::DXT5 ||
           format == PixelFormat::DXT1_sRGBA ||
           format == PixelFormat::DXT3_sRGBA ||
           format == PixelFormat::DXT5_sRGBA ||
           format == PixelFormat::BC7 ||
           format == PixelFormat::BC7_sRGBA;
}

constexpr bool
IsBPTCFormat(PixelFormat format)
{
    return format == PixelFormat::BC7 || format == PixelFormat::BC7_sRGBA;
}

constexpr bool
IsSRGBS3TCFormat(PixelFormat format)
{
    return format == PixelFormat::DXT1_sRGBA ||
           format == PixelFormat::DXT3_sRGBA ||
           format == PixelFormat::DXT5_sRGBA;
}

PixelFormat
GetUncompressedFormat(const DDSurfaceDesc& ddsd)
{
    util::GetLogger()->debug("DDS Format: {:08x}\n", ddsd.format.fourCC.get());
    switch (ddsd.format.bpp.get())
    {
    case 24:
        if (ddsd.format.redMask   == UINT32_C(0x000000ff) &&
            ddsd.format.greenMask == UINT32_C(0x0000ff00) &&
            ddsd.format.blueMask  == UINT32_C(0x00ff0000))
        {
            return PixelFormat::sRGB;
        }

#ifndef GL_ES
        if (ddsd.format.redMask   == UINT32_C(0x00ff0000) &&
            ddsd.format.greenMask == UINT32_C(0x0000ff00) &&
            ddsd.format.blueMask  == UINT32_C(0x000000ff))
        {
            util::GetLogger()->warn("Uncompressed BGR DDS texture loaded as linear; GL has no sBGR internal format.\n");
            return PixelFormat::BGR;
        }
#endif

        break;

    case 32:
        if (ddsd.format.redMask   == UINT32_C(0x00ff0000) &&
            ddsd.format.greenMask == UINT32_C(0x0000ff00) &&
            ddsd.format.blueMask  == UINT32_C(0x000000ff) &&
            ddsd.format.alphaMask == UINT32_C(0xff000000))
        {
            util::GetLogger()->warn("Uncompressed BGRA DDS texture loaded as linear; GL has no sBGRA internal format.\n");
            return PixelFormat::BGRA;
        }

        if (ddsd.format.redMask   == UINT32_C(0x000000ff) &&
            ddsd.format.greenMask == UINT32_C(0x0000ff00) &&
            ddsd.format.blueMask  == UINT32_C(0x00ff0000) &&
            ddsd.format.alphaMask == UINT32_C(0xff000000))
        {
            return PixelFormat::sRGBA;
        }

        break;

    default:
        break;
    }

    return PixelFormat::Invalid;
}

PixelFormat
GetDXT10Format(const DDSHeaderDXT10& dx10)
{
    const auto dxgiFormat = dx10.dxgiFormat.get();
    switch (static_cast<DXGIFormat>(dxgiFormat))
    {
    case DXGIFormat::BC1_UNORM:           return PixelFormat::DXT1;
    case DXGIFormat::BC1_UNORM_SRGB:      return PixelFormat::DXT1_sRGBA;
    case DXGIFormat::BC2_UNORM:           return PixelFormat::DXT3;
    case DXGIFormat::BC2_UNORM_SRGB:      return PixelFormat::DXT3_sRGBA;
    case DXGIFormat::BC3_UNORM:           return PixelFormat::DXT5;
    case DXGIFormat::BC3_UNORM_SRGB:      return PixelFormat::DXT5_sRGBA;
    case DXGIFormat::BC7_UNORM:           return PixelFormat::BC7;
    case DXGIFormat::BC7_UNORM_SRGB:      return PixelFormat::BC7_sRGBA;
    default:
        util::GetLogger()->error("Unsupported DXGI format in DDS DX10 header: {}\n", dxgiFormat);
        return PixelFormat::Invalid;
    }
}

PixelFormat
GetFormat(const DDSurfaceDesc& ddsd)
{
    const auto fourCC = ddsd.format.fourCC.get();
    switch (fourCC)
    {
    case 0:
        return GetUncompressedFormat(ddsd);

    case FourCC("DXT1"):
        return PixelFormat::DXT1_sRGBA;

    case FourCC("DXT3"):
        return PixelFormat::DXT3_sRGBA;

    case FourCC("DXT5"):
        return PixelFormat::DXT5_sRGBA;

    // DX10 extended header is handled by the caller before reaching here.

    default:
        util::GetLogger()->error("Unknown FourCC in DDS file: {:08x}\n", fourCC);
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
    case PixelFormat::DXT1_sRGBA:
        blocksize = 8;
        break;
    case PixelFormat::DXT3:
    case PixelFormat::DXT5:
    case PixelFormat::DXT3_sRGBA:
    case PixelFormat::DXT5_sRGBA:
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
            if (!in.read(reinterpret_cast<char*>(block.data()), blocksize)) /* Flawfinder: ignore */
                return nullptr;
            switch (format)
            {
            case PixelFormat::DXT1:
            case PixelFormat::DXT1_sRGBA:
                DecompressBlockDXT1(x, y, width, block.data(), transparent0, pixels.get());
                break;
            case PixelFormat::DXT3:
            case PixelFormat::DXT3_sRGBA:
                DecompressBlockDXT3(x, y, width, block.data(), transparent0, pixels.get());
                break;
            case PixelFormat::DXT5:
            case PixelFormat::DXT5_sRGBA:
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
CreateDecompressedImage(const DDSurfaceDesc& ddsd, PixelFormat format, std::istream& in, const std::filesystem::path& filename)
{
    // DXTc texture not supported, decompress DXTc to RGB/RGBA
    std::unique_ptr<std::uint32_t[]> pixels; //NOSONAR
    bool transparent0 = format == PixelFormat::DXT1 || format == PixelFormat::DXT1_sRGBA;
    const auto ddsdWidth = ddsd.width.get();
    const auto ddsdHeight = ddsd.height.get();
    if ((ddsdWidth & 3) != 0 || (ddsdHeight & 3) != 0)
    {
        std::uint32_t nw = std::max(ddsdWidth, UINT32_C(4));
        std::uint32_t nh = std::max(ddsdHeight, UINT32_C(4));
        auto tmp = DecompressDXTc(nw, nh, format, transparent0, in);
        if (tmp != nullptr)
        {
            pixels = std::make_unique<std::uint32_t[]>(ddsdWidth * ddsdHeight);
            // crop
            for (std::uint32_t y = 0; y < ddsdHeight; ++y)
            {
                std::memcpy(pixels.get() + y * ddsdWidth, tmp.get() + y * nw, ddsdWidth * 4);
            }
        }
    }
    else
    {
        pixels = DecompressDXTc(ddsdWidth, ddsdHeight, format, transparent0, in);
    }

    if (!pixels)
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
        std::uint32_t numberOfPixels = ddsdWidth * ddsdHeight;
        for (std::uint32_t index = 0; index < numberOfPixels; ++index)
        {
            std::memmove(&ptr[3 * index], &ptr[4 * index], sizeof(char) * 3);
        }
    }

    const bool isSRGB = format == PixelFormat::DXT1_sRGBA ||
                        format == PixelFormat::DXT3_sRGBA ||
                        format == PixelFormat::DXT5_sRGBA ||
                        format == PixelFormat::BC7_sRGBA;
    PixelFormat outFormat;
    if (transparent0)
        outFormat = isSRGB ? PixelFormat::sRGB : PixelFormat::RGB;
    else
        outFormat = isSRGB ? PixelFormat::sRGBA : PixelFormat::RGBA;
    auto img = std::make_unique<Image>(outFormat, ddsdWidth, ddsdHeight);
    std::memcpy(img->getPixels(), pixels.get(), (transparent0 ? 3 : 4) * ddsdWidth * ddsdHeight);
    return img;
}

} // anonymous namespace

Image* LoadDDSImage(const std::filesystem::path& filename)
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
    if (!in.read(reinterpret_cast<char*>(&ddsd), sizeof(ddsd)).good()) /* Flawfinder: ignore */
    {
        util::GetLogger()->error("DDS file {} has bad surface desc.\n", filename);
        return nullptr;
    }

    const auto ddsdWidth = ddsd.width.get();
    const auto ddsdHeight = ddsd.height.get();
    if (ddsdWidth == 0 || ddsdWidth > Image::MAX_DIMENSION ||
        ddsdHeight == 0 || ddsdHeight > Image::MAX_DIMENSION)
    {
        util::GetLogger()->error("DDS file {} size out of range.\n", filename);
        return nullptr;
    }

    std::uint32_t mipMapLevels = std::max(ddsd.mipMapLevels.get(), UINT32_C(1));
    std::uint32_t maxMipMapLevels = 1;
    for (std::uint32_t size = std::max(ddsdWidth, ddsdHeight); size > 1; size >>= 1)
        ++maxMipMapLevels;
    if (mipMapLevels > maxMipMapLevels)
    {
        util::GetLogger()->error("DDS file {} has too many mip levels.\n", filename);
        return nullptr;
    }

    PixelFormat format;
    if (ddsd.format.fourCC == FourCC("DX10"))
    {
        DDSHeaderDXT10 dx10;
        if (!in.read(reinterpret_cast<char*>(&dx10), sizeof(dx10)).good()) /* Flawfinder: ignore */
        {
            util::GetLogger()->error("DDS file {} has bad DX10 header.\n", filename);
            return nullptr;
        }

        format = GetDXT10Format(dx10);
    }
    else
    {
        format = GetFormat(ddsd);
    }

    if (format == PixelFormat::Invalid)
    {
        util::GetLogger()->error("Unsupported format for DDS texture file {}.\n", filename);
        return nullptr;
    }

    // BC7 must be uploaded as-is to the GPU; there is no software fallback.
    if (IsBPTCFormat(format) && !gl::ARB_texture_compression_bptc)
    {
        util::GetLogger()->error("DDS texture {} is BC7 but BPTC texture compression is not supported (requires GL_ARB_texture_compression_bptc on desktop GL, or ES 3.0 with GL_EXT_texture_compression_bptc on GLES).\n", filename);
        return nullptr;
    }

    // Decompress in software when the platform doesn't support the compressed format.
    if (IsCompressedFormat(format) && !IsBPTCFormat(format)
        && (!gl::EXT_texture_compression_s3tc
            || (IsSRGBS3TCFormat(format) && !gl::EXT_texture_compression_s3tc_srgb)))
        return CreateDecompressedImage(ddsd, format, in, filename).release();

    // TODO: Verify that the reported texture size matches the amount of
    // data expected.
    auto img = std::make_unique<Image>(format,
                                       static_cast<std::int32_t>(ddsdWidth),
                                       static_cast<std::int32_t>(ddsdHeight),
                                       static_cast<std::int32_t>(mipMapLevels));
    std::int32_t dataSize = 0;
    for (std::int32_t mip = 0; mip < img->getMipLevelCount(); ++mip)
        dataSize += img->getMipLevelSize(mip);
    if (!in.read(reinterpret_cast<char*>(img->getPixels()), dataSize)) /* Flawfinder: ignore */
    {
        util::GetLogger()->error("Failed reading data from DDS texture file {}.\n", filename);
        return nullptr;
    }

    return img.release();
}

} // namespace celestia::engine
