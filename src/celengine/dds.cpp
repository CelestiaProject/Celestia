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
#include <celutil/debug.h>
#include <celutil/bytes.h>
#include <celengine/gl.h>
#include <celengine/glext.h>
#include <celengine/texture.h>

using namespace std;


struct DDPixelFormat
{
    uint32 size;
    uint32 flags;
    uint32 fourCC;
    uint32 bpp;
    uint32 redMask;
    uint32 greenMask;
    uint32 blueMask;
    uint32 alphaMask;
};

struct DDSCaps
{
    uint32 caps;
    uint32 caps2;
    uint32 caps3;
    uint32 caps4;
};

struct DDColorKey
{
    uint32 lowVal;
    uint32 highVal;
};

struct DDSurfaceDesc
{
    uint32 size;
    uint32 flags;
    uint32 height;
    uint32 width;
    uint32 pitch;
    uint32 depth;
    uint32 mipMapLevels;
    uint32 alphaBitDepth;
    uint32 reserved;
    uint32 surface;

    DDColorKey ckDestOverlay;
    DDColorKey ckDestBlt;
    DDColorKey ckSrcOverlay;
    DDColorKey ckSrcBlt;

    DDPixelFormat format;
    DDSCaps caps;
    
    uint32 textureStage;
};


static uint32 FourCC(const char* s)
{
    return (((uint32) s[3] << 24) |
            ((uint32) s[2] << 16) |
            ((uint32) s[1] << 8) |
            (uint32) s[0]);
}


Texture* CreateDDSTexture(const string& filename)
{
    // We only load compressed DDS textures, so if S3 texture compression
    // isn't supported by the driver, give up now.
    if (!ExtensionSupported("GL_EXT_texture_compression_s3tc"))
        return NULL;

    ifstream in(filename.c_str(), ios::in | ios::binary);
    if (!in.good())
    {
        DPRINTF(0, "Error opening DDS texture file %s.\n", filename.c_str());
        return NULL;
    }

    char header[4];
    in.read(header, sizeof header);
    if (header[0] != 'D' || header[1] != 'D' ||
        header[2] != 'S' || header[3] != ' ')
    {
        DPRINTF(0, "DDS texture file %s has bad header.\n", filename.c_str());
        return NULL;
    }

    DDSurfaceDesc ddsd;
    in.read(reinterpret_cast<char*>(&ddsd), sizeof ddsd);
        LE_TO_CPU_INT32(ddsd.size, ddsd.size);
        LE_TO_CPU_INT32(ddsd.pitch, ddsd.pitch);
        LE_TO_CPU_INT32(ddsd.width, ddsd.width);
        LE_TO_CPU_INT32(ddsd.height, ddsd.height);
        LE_TO_CPU_INT32(ddsd.mipMapLevels, ddsd.mipMapLevels);
        LE_TO_CPU_INT32(ddsd.format.fourCC, ddsd.format.fourCC);

    int format = 0;
    int mipMapSizeFactor = 1;
    if (ddsd.format.fourCC == FourCC("DXT1"))
    {
        format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        mipMapSizeFactor = 2;
    }
    else if (ddsd.format.fourCC == FourCC("DXT3"))
    {
        format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        mipMapSizeFactor = 4;
    }
    else if (ddsd.format.fourCC == FourCC("DXT5"))
    {
        format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        mipMapSizeFactor = 4;
    }
    else
    {
        DPRINTF(0, "Unsupported format for DDS texture file %s.\n",
                filename.c_str());
        return NULL;
    }

    int size = ddsd.pitch;
    if (ddsd.mipMapLevels > 1)
        size *= mipMapSizeFactor;

    // TODO: Verify that the reported texture size matches the amount of
    // data expected.

    Texture* tex = new Texture((int) ddsd.width, (int) ddsd.height, format);
    if (tex == NULL)
        return NULL;

    // This is a hack; the members of texture should really be private
    tex->pixels = new unsigned char[size];
    if (tex->pixels == NULL)
    {
        DPRINTF(0, "Unable to allocated memory for DDS texture.\n");
        delete tex;
        return NULL;
    }

    if (ddsd.mipMapLevels == 1)
        tex->setMaxMipMapLevel(0);

    in.read(reinterpret_cast<char*>(tex->pixels), size);
    if (!in.eof() && !in.good())
    {
        DPRINTF(0, "Failed reading data from DDS texture file %s.\n",
                filename.c_str());
        delete tex;
        return NULL;
    }

#if 0
    cout << "sizeof(ddsd) = " << sizeof(ddsd) << '\n';
    cout << "ddsd.size = " << ddsd.size << '\n';
    cout << "size = " << size << '\n';
    cout << "dimensions: " << ddsd.width << 'x' << ddsd.height << '\n';
    cout << "mipmap levels: " << ddsd.mipMapLevels << '\n';
    cout << "format: " << ddsd.format.fourCC << '\n';
    cout << "DXT1: " << FourCC("DXT1") << '\n';
    cout << "DXT3: " << FourCC("DXT3") << '\n';
    cout << "DXT5: " << FourCC("DXT5") << '\n';
#endif

    return tex;
}
