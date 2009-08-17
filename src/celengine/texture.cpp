// texture.cpp
//
// Copyright (C) 2001-2003, Chris Laurel
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef TARGET_OS_MAC
#define JPEG_SUPPORT
#define PNG_SUPPORT
#endif

#ifdef TARGET_OS_MAC
#include <unistd.h>
#include "CGBuffer.h"
#ifndef PNG_SUPPORT
#include <Quicktime/ImageCompression.h>
#include <QuickTime/QuickTimeComponents.h>
#endif
#endif

#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <cassert>

#ifndef _WIN32
#ifndef TARGET_OS_MAC
#include <config.h>
#endif /* ! TARGET_OS_MAC */
#endif /* ! _WIN32 */

#include <celutil/filetype.h>
#include <celutil/debug.h>
#include <celutil/util.h>

#include <GL/glew.h>
#include "celestia.h"

#include <Eigen/Core>

#ifdef JPEG_SUPPORT

#ifndef PNG_SUPPORT
#include "setjmp.h"
#endif // PNG_SUPPORT

extern "C" {
#ifdef _WIN32
#include "jpeglib.h"
#else
#include <jpeglib.h>
#endif
}

#endif // JPEG_SUPPORT

#ifdef PNG_SUPPORT // PNG_SUPPORT
#ifdef TARGET_OS_MAC
#include "../../macosx/png.h"
#else
#include "png.h"
#endif // TARGET_OS_MAC

// Define png_jmpbuf() in case we are using a pre-1.0.6 version of libpng
#ifndef png_jmpbuf
#define png_jmpbuf(png_ptr) png_ptr->jmpbuf
#endif // PNG_SUPPORT

// Define various expansion transformations for old versions of libpng
#if PNG_LIBPNG_VER < 10004
#define png_set_palette_to_rgb(p)  png_set_expand(p)
#define png_set_gray_1_2_4_to_8(p) png_set_expand(p)
#define png_set_tRNS_to_alpha(p)   png_set_expand(p)
#endif // PNG_LIBPNG_VER < 10004

#endif // PNG_SUPPORT

#include "texture.h"
#include "virtualtex.h"

using namespace Eigen;
using namespace std;

static bool texCapsInitialized = false;

struct TextureCaps
{
    bool compressionSupported;
    bool clampToEdgeSupported;
    bool clampToBorderSupported;
    bool autoMipMapSupported;
    bool maxLevelSupported;
    GLint maxTextureSize;
    bool nonPow2Supported;
    GLint preferredAnisotropy;
};

static TextureCaps texCaps;


static bool testMaxLevel()
{
    unsigned char texels[64];

    glEnable(GL_TEXTURE_2D);
    // Test whether GL_TEXTURE_MAX_LEVEL is supported . . .
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE,
                 8, 8,
                 0,
                 GL_LUMINANCE,
                 GL_UNSIGNED_BYTE,
                 texels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 2);
    float maxLev = -1.0f;
    glGetTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &maxLev);
    glDisable(GL_TEXTURE_2D);

    return maxLev == 2;
}


static const TextureCaps& GetTextureCaps()
{
    if (!texCapsInitialized)
    {
        texCapsInitialized = true;
        texCaps.compressionSupported = (GLEW_ARB_texture_compression == GL_TRUE);

#ifdef GL_VERSION_1_2
        texCaps.clampToEdgeSupported = true;
#else
        texCaps.clampToEdgeSupported = ExtensionSupported("GL_EXT_texture_edge_clamp");
#endif // GL_VERSION_1_2
        texCaps.clampToBorderSupported = (GLEW_ARB_texture_border_clamp == GL_TRUE);
        texCaps.autoMipMapSupported = (GLEW_SGIS_generate_mipmap == GL_TRUE);
        texCaps.maxLevelSupported = testMaxLevel();
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texCaps.maxTextureSize);
        texCaps.nonPow2Supported = (GLEW_ARB_texture_non_power_of_two == GL_TRUE);

        texCaps.preferredAnisotropy = 1;
        if (GLEW_EXT_texture_filter_anisotropic)
        {
            GLint maxAnisotropy = 1;
            glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);

            // Cap the preferred level texture anisotropy to 8; eventually, we should allow
            // the user to control this.
            texCaps.preferredAnisotropy = min(8, maxAnisotropy);
        }
    }

    return texCaps;
}



static int getInternalFormat(int format)
{
    switch (format)
    {
    case GL_RGBA:
    case GL_BGRA_EXT:
        return 4;
    case GL_RGB:
    case GL_BGR_EXT:
        return 3;
    case GL_LUMINANCE_ALPHA:
        return 2;
    case GL_ALPHA:
    case GL_INTENSITY:
    case GL_LUMINANCE:
        return 1;
    case GL_DSDT_NV:
        return format;
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        return format;
    default:
        return 0;
    }
}


#if 0
// Required in order to support on-the-fly compression; currently, this
// feature is disabled.
static int getCompressedInternalFormat(int format)
{
    switch (format)
    {
    case GL_RGB:
    case GL_BGR_EXT:
        return GL_COMPRESSED_RGB_ARB;
    case GL_RGBA:
    case GL_BGRA_EXT:
        return GL_COMPRESSED_RGBA_ARB;
    case GL_ALPHA:
        return GL_COMPRESSED_ALPHA_ARB;
    case GL_LUMINANCE:
        return GL_COMPRESSED_LUMINANCE_ARB;
    case GL_LUMINANCE_ALPHA:
        return GL_COMPRESSED_LUMINANCE_ALPHA_ARB;
    case GL_INTENSITY:
        return GL_COMPRESSED_INTENSITY_ARB;
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        return format;
    default:
        return 0;
    }
}
#endif


static int getCompressedBlockSize(int format)
{
    if (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)
        return 8;
    else
        return 16;
}


static GLenum GetGLTexAddressMode(Texture::AddressMode addressMode)
{
    const TextureCaps& caps = GetTextureCaps();

    switch (addressMode)
    {
    case Texture::Wrap:
        return GL_REPEAT;

    case Texture::EdgeClamp:
        return caps.clampToEdgeSupported ? GL_CLAMP_TO_EDGE : GL_CLAMP;

    case Texture::BorderClamp:
        if (caps.clampToBorderSupported)
            return GL_CLAMP_TO_BORDER_ARB;
        else
            return caps.clampToEdgeSupported ? GL_CLAMP_TO_EDGE : GL_CLAMP;
    }

    return 0;
}


static void SetBorderColor(Color borderColor, GLenum target)
{
    float bc[4] = { borderColor.red(), borderColor.green(),
                    borderColor.blue(), borderColor.alpha() };
    glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, bc);
}


// Load a prebuilt set of mipmaps; assumes that the image contains
// a complete set of mipmap levels.
static void LoadMipmapSet(Image& img, GLenum target)
{
    int internalFormat = getInternalFormat(img.getFormat());

    for (int mip = 0; mip < img.getMipLevelCount(); mip++)
    {
        uint mipWidth  = max((uint) img.getWidth() >> mip, 1u);
        uint mipHeight = max((uint) img.getHeight() >> mip, 1u);

        if (img.isCompressed())
        {
            glCompressedTexImage2DARB(target,
                                           mip,
                                           internalFormat,
                                           mipWidth, mipHeight,
                                           0,
                                           img.getMipLevelSize(mip),
                                           img.getMipLevel(mip));
        }
        else
        {
            glTexImage2D(target,
                         mip,
                         internalFormat,
                         mipWidth, mipHeight,
                         0,
                         (GLenum) img.getFormat(),
                         GL_UNSIGNED_BYTE,
                         img.getMipLevel(mip));
        }
    }
}


// Load a texture without any mipmaps
static void LoadMiplessTexture(Image& img, GLenum target)
{
    int internalFormat = getInternalFormat(img.getFormat());

    if (img.isCompressed())
    {
        glCompressedTexImage2DARB(target,
                                       0,
                                       internalFormat,
                                       img.getWidth(), img.getHeight(),
                                       0,
                                       img.getMipLevelSize(0),
                                       img.getMipLevel(0));
    }
    else
    {
        glTexImage2D(target,
                     0,
                     internalFormat,
                     img.getWidth(), img.getHeight(),
                     0,
                     (GLenum) img.getFormat(),
                     GL_UNSIGNED_BYTE,
                     img.getMipLevel(0));
    }
}


static int ilog2(unsigned int x)
{
    int n = -1;

    while (x != 0)
    {
        x >>= 1;
        n++;
    }

    return n;
}


static int CalcMipLevelCount(int w, int h)
{
    return max(ilog2(w), ilog2(h)) + 1;
}


Texture::Texture(int w, int h, int d) :
    alpha(false),
    compressed(false),
    width(w),
    height(h),
    depth(d),
    formatOptions(0)
{
}


Texture::~Texture()
{
}


int Texture::getLODCount() const
{
    return 1;
}


int Texture::getUTileCount(int) const
{
    return 1;
}


int Texture::getVTileCount(int) const
{
    return 1;
}


int Texture::getWTileCount(int) const
{
    return 1;
}


void Texture::setBorderColor(Color)
{
}


int Texture::getWidth() const
{
    return width;
}


int Texture::getHeight() const
{
    return height;
}


int Texture::getDepth() const
{
    return depth;
}


unsigned int Texture::getFormatOptions() const
{
    return formatOptions;
}


void Texture::setFormatOptions(unsigned int opts)
{
    formatOptions = opts;
}


ImageTexture::ImageTexture(Image& img,
                           AddressMode addressMode,
                           MipMapMode mipMapMode) :
    Texture(img.getWidth(), img.getHeight()),
    glName(0)
{
    glGenTextures(1, (GLuint*) &glName);
    glBindTexture(GL_TEXTURE_2D, glName);


    bool mipmap = mipMapMode != NoMipMaps;
    bool precomputedMipMaps = false;

    // Use precomputed mipmaps only if a complete set is supplied
    int mipLevelCount = img.getMipLevelCount();
    if (mipmap && mipLevelCount == CalcMipLevelCount(img.getWidth(), img.getHeight()))
    {
        precomputedMipMaps = true;
    }

    // We can't automatically generate mipmaps for compressed textures.
    // If a precomputed mipmap set isn't provided, turn off mipmapping entirely.
    if (!precomputedMipMaps && img.isCompressed())
    {
        mipmap = false;
    }

    GLenum texAddress = GetGLTexAddressMode(addressMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texAddress);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texAddress);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

    if (GLEW_EXT_texture_filter_anisotropic && texCaps.preferredAnisotropy > 1)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, texCaps.preferredAnisotropy);
    }
    
    if (mipMapMode == AutoMipMaps)
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);

    int internalFormat = getInternalFormat(img.getFormat());

    if (mipmap)
    {
        if (precomputedMipMaps)
        {
            LoadMipmapSet(img, GL_TEXTURE_2D);
        }
        else if (mipMapMode == DefaultMipMaps)
        {
            gluBuild2DMipmaps(GL_TEXTURE_2D,
                              internalFormat,
                              getWidth(), getHeight(),
                              (GLenum) img.getFormat(),
                              GL_UNSIGNED_BYTE,
                              img.getPixels());
        }
        else
        {
            assert(mipMapMode == AutoMipMaps);
            LoadMiplessTexture(img, GL_TEXTURE_2D);
        }
    }
    else
    {
        LoadMiplessTexture(img, GL_TEXTURE_2D);
    }

    alpha = img.hasAlpha();
    compressed = img.isCompressed();
}


ImageTexture::~ImageTexture()
{
    if (glName != 0)
        glDeleteTextures(1, (const GLuint*) &glName);
}


void ImageTexture::bind()
{
    glBindTexture(GL_TEXTURE_2D, glName);
}


const TextureTile ImageTexture::getTile(int lod, int u, int v)
{
    if (lod != 0 || u != 0 || v != 0)
        return TextureTile(0);
    else
        return TextureTile(glName);
}


unsigned int ImageTexture::getName() const
{
    return glName;
}


void ImageTexture::setBorderColor(Color borderColor)
{
    bind();
    SetBorderColor(borderColor, GL_TEXTURE_2D);
}


TiledTexture::TiledTexture(Image& img,
                           int _uSplit, int _vSplit,
                           MipMapMode mipMapMode) :
    Texture(img.getWidth(), img.getHeight()),
    uSplit(_uSplit),
    vSplit(_vSplit),
    glNames(NULL)
{
    glNames = new uint[uSplit * vSplit];
    {
        for (int i = 0; i < uSplit * vSplit; i++)
            glNames[i] = 0;
    }

    alpha = img.hasAlpha();
    compressed = img.isCompressed();

    bool mipmap = mipMapMode != NoMipMaps;
    bool precomputedMipMaps = false;

    // Require a complete set of mipmaps
    int mipLevelCount = img.getMipLevelCount();
    int completeMipCount = CalcMipLevelCount(img.getWidth(), img.getHeight());
    // Allow a bit of slack here--it turns out that some tools don't want to
    // calculate the 1x1 mip level.  Rather than turn off mipmaps, we'll just
    // point the 1x1 mip to the 2x1.
    if (mipmap && mipLevelCount >= completeMipCount - 1)
        precomputedMipMaps = true;

    // We can't automatically generate mipmaps for compressed textures.
    // If a precomputed mipmap set isn't provided, turn of mipmapping entirely.
    if (!precomputedMipMaps && img.isCompressed())
        mipmap = false;

    GLenum texAddress = GetGLTexAddressMode(EdgeClamp);
    int internalFormat = getInternalFormat(img.getFormat());
    int components = img.getComponents();

    // Create a temporary image which we'll use for the tile texels
    int tileWidth = img.getWidth() / uSplit;
    int tileHeight = img.getHeight() / vSplit;
    int tileMipLevelCount = CalcMipLevelCount(tileWidth, tileHeight);
    Image* tile = new Image(img.getFormat(),
                            tileWidth, tileHeight,
                            tileMipLevelCount);
    if (tile == NULL)
        return;

    for (int v = 0; v < vSplit; v++)
    {
        for (int u = 0; u < uSplit; u++)
        {
            // Create the texture and set up sampling and addressing
            glGenTextures(1, (GLuint*)&glNames[v * uSplit + u]);
            glBindTexture(GL_TEXTURE_2D, glNames[v * uSplit + u]);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texAddress);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texAddress);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                            mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
            if (GLEW_EXT_texture_filter_anisotropic && texCaps.preferredAnisotropy > 1)
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, texCaps.preferredAnisotropy);
            }

            // Copy texels from the subtexture area to the pixel buffer.  This
            // is straightforward for normal textures, but an immense headache
            // for compressed textures with prebuilt mipmaps.
            if (precomputedMipMaps)
            {
                if (img.isCompressed())
                {
                    for (int mip = 0; mip < tileMipLevelCount; mip++)
                    {
                        int blockSize = getCompressedBlockSize(img.getFormat());
                        unsigned char* imgMip =
                            img.getMipLevel(min(mip, mipLevelCount));
                        uint mipWidth  = max((uint) img.getWidth() >> mip, 1u);
                        unsigned char* tileMip = tile->getMipLevel(mip);
                        uint tileMipWidth  = max((uint) tile->getWidth() >> mip, 1u);
                        uint tileMipHeight = max((uint) tile->getHeight() >> mip, 1u);
                        int uBlocks = max(tileMipWidth / 4, 1u);
                        int vBlocks = max(tileMipHeight / 4, 1u);
                        int destBytesPerRow = uBlocks * blockSize;
                        int srcBytesPerRow = max(mipWidth / 4, 1u) * blockSize;
                        int srcU = u * tileMipWidth / 4;
                        int srcV = v * tileMipHeight / 4;
                        int tileOffset = srcV * srcBytesPerRow +
                            srcU * blockSize;

                        for (int y = 0; y < vBlocks; y++)
                        {
                            memcpy(tileMip + y * destBytesPerRow,
                                   imgMip + tileOffset + y * srcBytesPerRow,
                                   destBytesPerRow);
                        }
                    }
                }
                else
                {
                    // TODO: Handle uncompressed textures with prebuilt mipmaps
                }

                LoadMipmapSet(*tile, GL_TEXTURE_2D);
            }
            else
            {
                if (img.isCompressed())
                {
                    int blockSize = getCompressedBlockSize(img.getFormat());
                    int uBlocks = max(tileWidth / 4, 1);
                    int vBlocks = max(tileHeight / 4, 1);
                    int destBytesPerRow = uBlocks * blockSize;
                    int srcBytesPerRow = max(img.getWidth() / 4, 1) * blockSize;
                    int srcU = u * tileWidth / 4;
                    int srcV = v * tileHeight / 4;
                    int tileOffset = srcV * srcBytesPerRow +
                            srcU * blockSize;

                    for (int y = 0; y < vBlocks; y++)
                    {
                        memcpy(tile->getPixels() + y * destBytesPerRow,
                               img.getPixels() + tileOffset + y * srcBytesPerRow,
                               destBytesPerRow);
                    }
                }
                else
                {
                    unsigned char* tilePixels = img.getPixels() +
                        (v * tileHeight * img.getWidth() + u * tileWidth) * components;
                    for (int y = 0; y < tileHeight; y++)
                    {
                        memcpy(tile->getPixels() + y * tileWidth * components,
                               tilePixels + y * img.getWidth() * components,
                               tileWidth * components);
                    }
                }

                if (mipmap)
                {
                    gluBuild2DMipmaps(GL_TEXTURE_2D,
                                      internalFormat,
                                      tileWidth, tileHeight,
                                      (GLenum) tile->getFormat(),
                                      GL_UNSIGNED_BYTE,
                                      tile->getPixels());
                }
                else
                {
                    LoadMiplessTexture(*tile, GL_TEXTURE_2D);
                }
            }
        }
    }

    delete tile;
}


TiledTexture::~TiledTexture()
{
    if (glNames != NULL)
    {
        for (int i = 0; i < uSplit * vSplit; i++)
        {
            if (glNames[i] != 0)
                glDeleteTextures(1, (const GLuint*) &glNames[i]);
        }
        delete[] glNames;
    }
}


void TiledTexture::bind()
{
}


void TiledTexture::setBorderColor(Color borderColor)
{
    for (int i = 0; i < vSplit; i++)
    {
        for (int j = 0; j < uSplit; j++)
        {
            glBindTexture(GL_TEXTURE_2D, glNames[i * uSplit + j]);
            SetBorderColor(borderColor, GL_TEXTURE_2D);
        }
    }
}


int TiledTexture::getUTileCount(int) const
{
    return uSplit;
}


int TiledTexture::getVTileCount(int) const
{
    return vSplit;
}


const TextureTile TiledTexture::getTile(int lod, int u, int v)
{
    if (lod != 0 || u >= uSplit || u < 0 || v >= vSplit || v < 0)
        return TextureTile(0);
    else
    {
        return TextureTile(glNames[v * uSplit + u]);
    }
}



CubeMap::CubeMap(Image* faces[]) :
    Texture(faces[0]->getWidth(), faces[0]->getHeight()),
    glName(0)
{
    // Verify that all the faces are square and have the same size
    int width = faces[0]->getWidth();
    int format = faces[0]->getFormat();
    int i = 0;
    for (i = 0; i < 6; i++)
    {
        if (faces[i]->getWidth() != width ||
            faces[i]->getHeight() != width ||
            faces[i]->getFormat() != format)
            return;
    }

    // For now, always enable mipmaps; in the future, it should be possible to
    // override this.
    bool mipmap = true;
    bool precomputedMipMaps = false;

    // Require a complete set of mipmaps
    int mipLevelCount = faces[0]->getMipLevelCount();
    if (mipmap && mipLevelCount == CalcMipLevelCount(width, width))
        precomputedMipMaps = true;

    // We can't automatically generate mipmaps for compressed textures.
    // If a precomputed mipmap set isn't provided, turn of mipmapping entirely.
    if (!precomputedMipMaps && faces[0]->isCompressed())
        mipmap = false;

    glGenTextures(1, (GLuint*) &glName);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, glName);

    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER,
                    mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

    int internalFormat = getInternalFormat(format);

    for (i = 0; i < 6; i++)
    {
        GLenum targetFace = (GLenum) ((int) GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + i);
        Image* face = faces[i];

        if (mipmap)
        {
            if (precomputedMipMaps)
            {
                LoadMipmapSet(*face, targetFace);
            }
            else
            {
                gluBuild2DMipmaps(targetFace,
                                  internalFormat,
                                  getWidth(), getHeight(),
                                  (GLenum) face->getFormat(),
                                  GL_UNSIGNED_BYTE,
                                  face->getPixels());
            }
        }
        else
        {
            LoadMiplessTexture(*face, targetFace);
        }
    }
}


CubeMap::~CubeMap()
{
    if (glName != 0)
        glDeleteTextures(1, (const GLuint*) &glName);
}


void CubeMap::bind()
{
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, glName);
}


const TextureTile CubeMap::getTile(int lod, int u, int v)
{
    if (lod != 0 || u != 0 || v != 0)
        return TextureTile(0);
    else
        return TextureTile(glName);
}


void CubeMap::setBorderColor(Color borderColor)
{
    bind();
    SetBorderColor(borderColor, GL_TEXTURE_CUBE_MAP_ARB);
}



Texture* CreateProceduralTexture(int width, int height,
                                 int format,
                                 ProceduralTexEval func,
                                 Texture::AddressMode addressMode,
                                 Texture::MipMapMode mipMode)
{
    Image* img = new Image(format, width, height);
    if (img == NULL)
        return NULL;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            float u = ((float) x + 0.5f) / (float) width * 2 - 1;
            float v = ((float) y + 0.5f) / (float) height * 2 - 1;
            func(u, v, 0, img->getPixelRow(y) + x * img->getComponents());
        }
    }

    Texture* tex = new ImageTexture(*img, addressMode, mipMode);
    delete img;

    return tex;
}


Texture* CreateProceduralTexture(int width, int height,
                                 int format,
                                 TexelFunctionObject& func,
                                 Texture::AddressMode addressMode,
                                 Texture::MipMapMode mipMode)
{
    Image* img = new Image(format, width, height);
    if (img == NULL)
        return NULL;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            float u = ((float) x + 0.5f) / (float) width * 2 - 1;
            float v = ((float) y + 0.5f) / (float) height * 2 - 1;
            func(u, v, 0, img->getPixelRow(y) + x * img->getComponents());
        }
    }

    Texture* tex = new ImageTexture(*img, addressMode, mipMode);
    delete img;

    return tex;
}


// Helper function for CreateProceduralCubeMap; return the normalized
// vector pointing to (s, t) on the specified face.
static Vector3f cubeVector(int face, float s, float t)
{
    Vector3f v;
    switch (face)
    {
    case 0:
        v = Vector3f(1.0f, -t, -s);
        break;
    case 1:
        v = Vector3f(-1.0f, -t, s);
        break;
    case 2:
        v = Vector3f(s, 1.0f, t);
        break;
    case 3:
        v = Vector3f(s, -1.0f, -t);
        break;
    case 4:
        v = Vector3f(s, -t, 1.0f);
        break;
    case 5:
        v = Vector3f(-s, -t, -1.0f);
        break;
    default:
        // assert(false);
        break;
    }

    v.normalize();

    return v;
}


extern Texture* CreateProceduralCubeMap(int size, int format,
                                        ProceduralTexEval func)
{
    Image* faces[6];
    bool failed = false;

    int i = 0;
    for (i = 0; i < 6; i++)
    {
        faces[i] = NULL;
        faces[i] = new Image(format, size, size);
        if (faces == NULL)
            failed = true;
    }

    if (!failed)
    {
        for (int i = 0; i < 6; i++)
        {
            Image* face = faces[i];
            for (int y = 0; y < size; y++)
            {
                for (int x = 0; x < size; x++)
                {
                    float s = ((float) x + 0.5f) / (float) size * 2 - 1;
                    float t = ((float) y + 0.5f) / (float) size * 2 - 1;
                    Vector3f v = cubeVector(i, s, t);
                    func(v.x(), v.y(), v.z(),
                         face->getPixelRow(y) + x * face->getComponents());
                }
            }
        }
    }

    Texture* tex = new CubeMap(faces);

    // Clean up the images
    for (i = 0; i < 6; i++)
    {
        if (faces[i] != NULL)
            delete faces[i];
    }

    return tex;
}

#if 0
static bool isPow2(int x)
{
    return ((x & (x - 1)) == 0);
}
#endif

static Texture* CreateTextureFromImage(Image& img,
                                       Texture::AddressMode addressMode,
                                       Texture::MipMapMode mipMode)
{
#if 0
    // Require texture dimensions to be powers of two.  Even though the
    // OpenGL driver will automatically rescale textures with non-power of
    // two sizes, some quality loss may result.  The power of two requirement
    // forces texture creators to resize their textures in an image editing
    // program, hopefully resulting in  textures that look as good as possible
    // when rendered on current hardware.
    if (!isPow2(img.getWidth()) || !isPow2(img.getHeight()))
    {
        clog << "Texture has non-power of two dimensions.\n";
        return NULL;
    }
#endif

    // If non power of two textures are supported switch mipmap generation to
    // automatic. gluBuildMipMaps may rescale the texture to a power of two
    // on some drivers even when the hardware supports non power of two textures,
    // whereas auto mipmap generation will properly deal with the dimensions.
    if (GetTextureCaps().nonPow2Supported)
    {
        if (mipMode == Texture::DefaultMipMaps)
            mipMode = Texture::AutoMipMaps;
    }

    bool splittingAllowed = true;
    Texture* tex = NULL;

    int maxDim = GetTextureCaps().maxTextureSize;
    if ((img.getWidth() > maxDim || img.getHeight() > maxDim) &&
        splittingAllowed)
    {
        // The texture is too large; we need to split it.
        int uSplit = max(1, img.getWidth() / maxDim);
        int vSplit = max(1, img.getHeight() / maxDim);
        clog << _("Creating tiled texture. Width=") << img.getWidth() << _(", max=") << maxDim << "\n";
        tex = new TiledTexture(img, uSplit, vSplit, mipMode);
    }
    else
    {
        clog << _("Creating ordinary texture: ") << img.getWidth() << "x" << img.getHeight() << "\n";
        // The image is small enough to fit in a single texture; or, splitting
        // was disallowed so we'll scale the large image down to fit in
        // an ordinary texture.
        tex = new ImageTexture(img, addressMode, mipMode);
    }

    return tex;
}


Texture* LoadTextureFromFile(const string& filename,
                             Texture::AddressMode addressMode,
                             Texture::MipMapMode mipMode)
{
    // Check for a Celestia texture--these need to be handled specially.
    ContentType contentType = DetermineFileType(filename);

    if (contentType == Content_CelestiaTexture)
        return LoadVirtualTexture(filename);

    // All other texture types are handled by first loading an image, then
    // creating a texture from that image.
    Image* img = LoadImageFromFile(filename);
    if (img == NULL)
        return NULL;

    Texture* tex = CreateTextureFromImage(*img, addressMode, mipMode);

    if (contentType == Content_DXT5NormalMap)
    {
        // If the texture came from a .dxt5nm file then mark it as a dxt5
        // compressed normal map. There's no separate OpenGL format for dxt5
        // normal maps, so the file extension is the only thing that
        // distinguishes it from a plain old dxt5 texture.
        if (img->getFormat() == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
        {
            tex->setFormatOptions(Texture::DXT5NormalMap);
        }
    }

    delete img;

    return tex;
}


// Load a height map texture from a file and convert it to a normal map.
Texture* LoadHeightMapFromFile(const string& filename,
                               float height,
                               Texture::AddressMode addressMode)
{
    Image* img = LoadImageFromFile(filename);
    if (img == NULL)
        return NULL;
    Image* normalMap = img->computeNormalMap(height,
                                             addressMode == Texture::Wrap);
    delete img;
    if (normalMap == NULL)
        return NULL;

    Texture* tex = CreateTextureFromImage(*normalMap, addressMode,
                                          Texture::DefaultMipMaps);
    delete normalMap;

    return tex;
}
