// dds.cpp
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstring>
#include <fstream>
#include <algorithm>
#include <memory>
#include <celengine/glsupport.h>
#include <celengine/image.h>
#include <celutil/logger.h>
#include <celutil/bytes.h>
#include "dds_decompress.h"

using namespace celestia;
using namespace std;
using celestia::util::GetLogger;


struct DDPixelFormat
{
    uint32_t size;
    uint32_t flags;
    uint32_t fourCC;
    uint32_t bpp;
    uint32_t redMask;
    uint32_t greenMask;
    uint32_t blueMask;
    uint32_t alphaMask;
};

struct DDSCaps
{
    uint32_t caps;
    uint32_t caps2;
    uint32_t caps3;
    uint32_t caps4;
};

struct DDColorKey
{
    uint32_t lowVal;
    uint32_t highVal;
};

struct DDSurfaceDesc
{
    uint32_t size;
    uint32_t flags;
    uint32_t height;
    uint32_t width;
    uint32_t pitch;
    uint32_t depth;
    uint32_t mipMapLevels;
    uint32_t alphaBitDepth;
    uint32_t reserved;
    uint32_t surface;

    DDColorKey ckDestOverlay;
    DDColorKey ckDestBlt;
    DDColorKey ckSrcOverlay;
    DDColorKey ckSrcBlt;

    DDPixelFormat format;
    DDSCaps caps;

    uint32_t textureStage;
};


namespace
{

constexpr uint32_t FourCC(const char* s)
{
    return (((uint32_t) s[3] << 24) |
            ((uint32_t) s[2] << 16) |
            ((uint32_t) s[1] << 8) |
            (uint32_t) s[0]);
}

// decompress a DXTc texture to a RGBA texture, taken from https://github.com/ptitSeb/gl4es
uint32_t* decompressDXTc(uint32_t width, uint32_t height, GLenum format, bool transparent0, ifstream &in)
{
    // TODO: check with the size of the input data stream if the stream is in fact decompressed
    // alloc memory
    auto *pixels = new uint32_t[((width + 3) & ~3) * ((height + 3) & ~3)];

    int blocksize = 0;
#define DDS_MAX_BLOCK_SIZE      16
    switch (format)
    {
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        blocksize = 8;
        break;
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        blocksize = 16;
        break;
    }
    uint8_t block[DDS_MAX_BLOCK_SIZE]; // enough to hold DXT1/3/5 blocks
    for (uint32_t y = 0; y < height; y += 4)
    {
        for (uint32_t x = 0; x < width; x += 4)
        {
            if (!in.good())
            {
                delete[] pixels;
                return nullptr;
            }
            in.read((char*)block, blocksize);
            switch(format)
            {
            case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
                DecompressBlockDXT1(x, y, width, block, transparent0, pixels);
                break;
            case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
                DecompressBlockDXT3(x, y, width, block, transparent0, pixels);
                break;
            case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
                DecompressBlockDXT5(x, y, width, block, transparent0, pixels);
                break;
            }
        }
    }
    return pixels;
}

} // anonymous namespace

Image* LoadDDSImage(const fs::path& filename)
{
    ifstream in(filename, ios::in | ios::binary);
    if (!in.good())
    {
        GetLogger()->error("Error opening DDS texture file {}.\n", filename);
        return nullptr;
    }

    char header[4];
    if (!in.read(header, sizeof(header)).good()
        || header[0] != 'D' || header[1] != 'D'
        || header[2] != 'S' || header[3] != ' ')
    {
        GetLogger()->error("DDS texture file {} has bad header.\n", filename);
        return nullptr;
    }

    DDSurfaceDesc ddsd;
    if (!in.read(reinterpret_cast<char*>(&ddsd), sizeof ddsd).good())
    {
        GetLogger()->error("DDS file {} has bad surface desc.\n", filename);
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

    int format = -1;

    if (ddsd.format.fourCC != 0)
    {
        if (ddsd.format.fourCC == FourCC("DXT1"))
        {
            format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        }
        else if (ddsd.format.fourCC == FourCC("DXT3"))
        {
            format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        }
        else if (ddsd.format.fourCC == FourCC("DXT5"))
        {
            format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        }
        else
        {
            GetLogger()->error("Unknown FourCC in DDS file: {}\n", ddsd.format.fourCC);
        }
    }
    else
    {
        GetLogger()->debug("DDS Format: {}\n", ddsd.format.fourCC);
        if (ddsd.format.bpp == 32)
        {
            if (ddsd.format.redMask   == 0x00ff0000 &&
                ddsd.format.greenMask == 0x0000ff00 &&
                ddsd.format.blueMask  == 0x000000ff &&
                ddsd.format.alphaMask == 0xff000000)
            {
                format = GL_BGRA_EXT;
            }
            else if (ddsd.format.redMask   == 0x000000ff &&
                     ddsd.format.greenMask == 0x0000ff00 &&
                     ddsd.format.blueMask  == 0x00ff0000 &&
                     ddsd.format.alphaMask == 0xff000000)
            {
                format = GL_RGBA;
            }
        }
        else if (ddsd.format.bpp == 24)
        {
            if (ddsd.format.redMask   == 0x000000ff &&
                ddsd.format.greenMask == 0x0000ff00 &&
                ddsd.format.blueMask  == 0x00ff0000)
            {
                format = GL_RGB;
            }
#ifndef GL_ES
            else if (ddsd.format.redMask   == 0x00ff0000 &&
                     ddsd.format.greenMask == 0x0000ff00 &&
                     ddsd.format.blueMask  == 0x000000ff)
            {
                format = GL_BGR;
            }
#endif
        }
    }

    if (format == -1)
    {
        GetLogger()->error("Unsupported format for DDS texture file {}.\n", filename);
        return nullptr;
    }

    // Check if the platform supports compressed DTXc textures
    if (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ||
        format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT ||
        format == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
    {
        if (!gl::EXT_texture_compression_s3tc)
        {
            // DXTc texture not supported, decompress DXTc to RGB/RGBA
            uint32_t *pixels = nullptr;
            bool transparent0 = format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            if ((ddsd.width & 3) != 0 || (ddsd.height & 3) != 0)
            {
                uint32_t nw = max(ddsd.width, 4u);
                uint32_t nh = max(ddsd.height, 4u);
                auto *tmp = decompressDXTc(nw, nh, format, transparent0, in);
                if (tmp != nullptr)
                {
                    pixels = new uint32_t[ddsd.width * ddsd.height];
                    // crop
                    for (uint32_t y = 0; y < ddsd.height; y++)
                        memcpy((char *)pixels + y * ddsd.width * 4, (char *)tmp + y * nw * 4, ddsd.width * 4);
                    delete[] tmp;
                }
            }
            else
            {
                pixels = decompressDXTc(ddsd.width, ddsd.height, format, transparent0, in);
            }

            if (pixels == nullptr)
            {
                GetLogger()->error("Failed to decompress DDS texture file {}.\n", filename);
                return nullptr;
            }

            if (transparent0)
            {
                // Remove the alpha channel for DXT1 since DXT1 textures
                // are deemed not to contain alpha values in Celestia
                // https://github.com/CelestiaProject/Celestia/pull/1086
                char *ptr = reinterpret_cast<char*>(pixels);
                uint32_t numberOfPixels = ddsd.width * ddsd.height;
                for (uint32_t index = 0; index < numberOfPixels; ++index)
                {
                    memcpy(&ptr[3 * index], &ptr[4 * index], sizeof(char) * 3);
                }
            }

            Image *img = new Image(transparent0 ? PixelFormat::RGB : PixelFormat::RGBA, ddsd.width, ddsd.height);
            memcpy(img->getPixels(), pixels, (transparent0 ? 3 : 4) * ddsd.width * ddsd.height);
            delete[] pixels;
            return img;
        }
    }

    // TODO: Verify that the reported texture size matches the amount of
    // data expected.

    Image* img = new Image(static_cast<PixelFormat>(format),
                           (int) ddsd.width,
                           (int) ddsd.height,
                           max(ddsd.mipMapLevels, 1u));
    in.read(reinterpret_cast<char*>(img->getPixels()), img->getSize());
    if (!in.eof() && !in.good())
    {
        GetLogger()->error("Failed reading data from DDS texture file {}.\n", filename);
        delete img;
        return nullptr;
    }

    return img;
}
