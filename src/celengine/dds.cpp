// dds.cpp
//
// Copyright (C) 2001, Chris Laurel
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include <fstream>
#include <algorithm>
#include <celutil/debug.h>
#include <celutil/bytes.h>
#include <celengine/image.h>
#include <GL/glew.h>

using namespace std;



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


static uint32_t FourCC(const char* s)
{
    return (((uint32_t) s[3] << 24) |
            ((uint32_t) s[2] << 16) |
            ((uint32_t) s[1] << 8) |
            (uint32_t) s[0]);
}


#define DDPF_RGB    0x40
#define DDPF_FOURCC 0x04


Image* LoadDDSImage(const fs::path& filename)
{
    ifstream in(filename.string(), ios::in | ios::binary);
    if (!in.good())
    {
        DPRINTF(0, "Error opening DDS texture file %s.\n", filename.c_str());
        return nullptr;
    }

    char header[4];
    in.read(header, sizeof header);
    if (header[0] != 'D' || header[1] != 'D' ||
        header[2] != 'S' || header[3] != ' ')
    {
        DPRINTF(0, "DDS texture file %s has bad header.\n", filename.c_str());
        return nullptr;
    }

    DDSurfaceDesc ddsd;
    in.read(reinterpret_cast<char*>(&ddsd), sizeof ddsd);
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
            cerr << "Unknown FourCC in DDS file: " << ddsd.format.fourCC;
        }
    }
    else
    {
        clog << "DDS Format: " << ddsd.format.fourCC << '\n';
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
            if (ddsd.format.redMask   == 0x00ff0000 &&
                ddsd.format.greenMask == 0x0000ff00 &&
                ddsd.format.blueMask  == 0x000000ff)
            {
                format = GL_BGR_EXT;
            }
            else if (ddsd.format.redMask   == 0x000000ff &&
                     ddsd.format.greenMask == 0x0000ff00 &&
                     ddsd.format.blueMask  == 0x00ff0000)
            {
                format = GL_RGB;
            }
        }
    }

    if (format == -1)
    {
        DPRINTF(0, "Unsupported format for DDS texture file %s.\n",
                filename.c_str());
        return nullptr;
    }

    // If we have a compressed format, give up if S3 texture compression
    // isn't supported
    if (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ||
        format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT ||
        format == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
    {
        if (!GLEW_EXT_texture_compression_s3tc)
            return nullptr;
    }

    // TODO: Verify that the reported texture size matches the amount of
    // data expected.

    Image* img = new Image(format,
                           (int) ddsd.width,
                           (int) ddsd.height,
                           max(ddsd.mipMapLevels, 1u));
    in.read(reinterpret_cast<char*>(img->getPixels()), img->getSize());
    if (!in.eof() && !in.good())
    {
        DPRINTF(0, "Failed reading data from DDS texture file %s.\n",
                filename.c_str());
        delete img;
        return nullptr;
    }

#if 0
    cout << "sizeof(ddsd) = " << sizeof(ddsd) << '\n';
    cout << "dimensions: " << ddsd.width << 'x' << ddsd.height << '\n';
    cout << "mipmap levels: " << ddsd.mipMapLevels << '\n';
    cout << "fourCC: " << ddsd.format.fourCC << '\n';
    cout << "bpp: " << ddsd.format.bpp << '\n';
#endif

    return img;
}
