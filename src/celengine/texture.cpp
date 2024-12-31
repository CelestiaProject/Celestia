// texture.cpp
//
// Copyright (C) 2001-2003, Chris Laurel
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <cmath>

#include <Eigen/Core>
#include "glsupport.h"

#include <celutil/filetype.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include "framebuffer.h"
#include "texture.h"
#include "virtualtex.h"


using namespace celestia;
using celestia::util::GetLogger;
using celestia::engine::Image;
using celestia::engine::PixelFormat;

namespace
{

struct TextureCaps
{
    GLint preferredAnisotropy;
};

const
TextureCaps& GetTextureCaps()
{
    static TextureCaps texCaps;
    static bool texCapsInitialized = false;

    if (!texCapsInitialized)
    {
        texCapsInitialized = true;

        texCaps.preferredAnisotropy = 1;
#ifndef GL_ES
        if (gl::EXT_texture_filter_anisotropic)
        {
            // Cap the preferred level texture anisotropy to 8; eventually, we should allow
            // the user to control this.
            texCaps.preferredAnisotropy = std::min(8, gl::maxTextureAnisotropy);
        }
#endif
    }

    return texCaps;
}

GLenum
getInternalFormat(PixelFormat format)
{
#ifdef GL_ES
    switch (format)
    {
    case PixelFormat::RGBA:
    case PixelFormat::RGB:
    case PixelFormat::LumAlpha:
    case PixelFormat::Alpha:
    case PixelFormat::Luminance:
    case PixelFormat::DXT1:
    case PixelFormat::DXT3:
    case PixelFormat::DXT5:
        return static_cast<GLenum>(format);
    default:
        return GL_NONE;
    }
#else
    switch (format)
    {
    case PixelFormat::RGBA:
    case PixelFormat::BGRA:
    case PixelFormat::RGB:
    case PixelFormat::BGR:
    case PixelFormat::LumAlpha:
    case PixelFormat::Alpha:
    case PixelFormat::Luminance:
    case PixelFormat::DXT1:
    case PixelFormat::DXT3:
    case PixelFormat::DXT5:
    case PixelFormat::sLumAlpha:
    case PixelFormat::sLuminance:
    case PixelFormat::sRGB:
    case PixelFormat::sRGBA:
    case PixelFormat::DXT1_sRGBA:
    case PixelFormat::DXT3_sRGBA:
    case PixelFormat::DXT5_sRGBA:
        return static_cast<GLenum>(format);
    default:
        return GL_NONE;
    }
#endif
}

GLenum
getExternalFormat(PixelFormat format)
{
#ifdef GL_ES
    return getInternalFormat(format);
#else
    switch (format)
    {
    case PixelFormat::RGBA:
    case PixelFormat::BGRA:
    case PixelFormat::RGB:
    case PixelFormat::BGR:
    case PixelFormat::LumAlpha:
    case PixelFormat::Alpha:
    case PixelFormat::Luminance:
    case PixelFormat::DXT1:
    case PixelFormat::DXT3:
    case PixelFormat::DXT5:
        return static_cast<GLenum>(format);
    case PixelFormat::sLumAlpha:
        return static_cast<GLenum>(PixelFormat::LumAlpha);
    case PixelFormat::sLuminance:
        return static_cast<GLenum>(PixelFormat::Luminance);
    case PixelFormat::sRGB:
    case PixelFormat::sRGB8:
        return static_cast<GLenum>(PixelFormat::RGB);
    case PixelFormat::sRGBA:
    case PixelFormat::sRGBA8:
        return static_cast<GLenum>(PixelFormat::RGBA);
    case PixelFormat::DXT1_sRGBA:
        return static_cast<GLenum>(PixelFormat::DXT1);
    case PixelFormat::DXT3_sRGBA:
        return static_cast<GLenum>(PixelFormat::DXT3);
    case PixelFormat::DXT5_sRGBA:
        return static_cast<GLenum>(PixelFormat::DXT5);
    default:
        return GL_NONE;
    }
#endif
}

#if 0
// Required in order to support on-the-fly compression; currently, this
// feature is disabled.
int
getCompressedInternalFormat(int format)
{
    switch (format)
    {
    case GL_RGB:
    case GL_BGR:
        return GL_COMPRESSED_RGB;
    case GL_RGBA:
    case GL_BGRA:
        return GL_COMPRESSED_RGBA;
    case GL_ALPHA:
        return GL_COMPRESSED_ALPHA;
    case GL_LUMINANCE:
        return GL_COMPRESSED_LUMINANCE;
    case GL_LUMINANCE_ALPHA:
        return GL_COMPRESSED_LUMINANCE_ALPHA;
    case GL_INTENSITY:
        return GL_COMPRESSED_INTENSITY;
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        return format;
    default:
        return 0;
    }
}
#endif

int
getCompressedBlockSize(PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::DXT1:
    case PixelFormat::DXT1_sRGBA:
        return 8;
    default:
        return 16;
    }
}

GLenum
GetGLTexAddressMode(Texture::AddressMode addressMode)
{
    switch (addressMode)
    {
    case Texture::Wrap:
        return GL_REPEAT;

    case Texture::EdgeClamp:
        return GL_CLAMP_TO_EDGE;

    case Texture::BorderClamp:
#ifndef GL_ES
        return GL_CLAMP_TO_BORDER;
#else
        return GL_CLAMP_TO_BORDER_OES;
#endif
    }

    return 0;
}

void
SetBorderColor(Color borderColor, GLenum target)
{
    float bc[4] = { borderColor.red(), borderColor.green(),
                    borderColor.blue(), borderColor.alpha() };
#ifndef GL_ES
    glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, bc);
#else
    glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR_OES, bc);
#endif
}

// Load a prebuilt set of mipmaps; assumes that the image contains
// a complete set of mipmap levels.
void
LoadMipmapSet(const Image& img, GLenum target)
{
    int internalFormat = getInternalFormat(img.getFormat());
#ifndef GL_ES
    glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, img.getMipLevelCount()-1);
#endif

    for (int mip = 0; mip < img.getMipLevelCount(); mip++)
    {
        int mipWidth  = std::max(img.getWidth() >> mip, 1);
        int mipHeight = std::max(img.getHeight() >> mip, 1);

        if (img.isCompressed())
        {
            glCompressedTexImage2D(target,
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
                         getExternalFormat(img.getFormat()),
                         GL_UNSIGNED_BYTE,
                         img.getMipLevel(mip));
        }
    }
}

// Load a texture without any mipmaps
void
LoadMiplessTexture(const Image& img, GLenum target)
{
    int internalFormat = getInternalFormat(img.getFormat());

    if (img.isCompressed())
    {
        glCompressedTexImage2D(target,
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
                     getExternalFormat(img.getFormat()),
                     GL_UNSIGNED_BYTE,
                     img.getMipLevel(0));
    }
}


int
ilog2(unsigned int x)
{
    int n = -1;

    while (x != 0)
    {
        x >>= 1;
        n++;
    }

    return n;
}

int
CalcMipLevelCount(int w, int h)
{
    return std::max(ilog2(w), ilog2(h)) + 1;
}

// Helper function for CreateProceduralCubeMap; return the normalized
// vector pointing to (s, t) on the specified face.
Eigen::Vector3f
cubeVector(int face, float s, float t)
{
    Eigen::Vector3f v;
    switch (face)
    {
    case 0:
        v = Eigen::Vector3f(1.0f, -t, -s);
        break;
    case 1:
        v = Eigen::Vector3f(-1.0f, -t, s);
        break;
    case 2:
        v = Eigen::Vector3f(s, 1.0f, t);
        break;
    case 3:
        v = Eigen::Vector3f(s, -1.0f, -t);
        break;
    case 4:
        v = Eigen::Vector3f(s, -t, 1.0f);
        break;
    case 5:
        v = Eigen::Vector3f(-s, -t, -1.0f);
        break;
    default:
        assert(false);
        break;
    }

    return v.normalized();
}

std::unique_ptr<Texture>
CreateTextureFromImage(const Image& img,
                       Texture::AddressMode addressMode,
                       Texture::MipMapMode mipMode)
{
    std::unique_ptr<Texture> tex = nullptr;

    const int maxDim = gl::maxTextureSize;
    if ((img.getWidth() > maxDim || img.getHeight() > maxDim))
    {
        // The texture is too large; we need to split it.
        int uSplit = std::max(1, img.getWidth() / maxDim);
        int vSplit = std::max(1, img.getHeight() / maxDim);
        GetLogger()->info(_("Creating tiled texture. Width={}, max={}\n"),
                          img.getWidth(), maxDim);
        tex = std::make_unique<TiledTexture>(img, uSplit, vSplit, mipMode);
    }
    else
    {
        GetLogger()->info(_("Creating ordinary texture: {}x{}\n"),
                          img.getWidth(), img.getHeight());
        tex = std::make_unique<ImageTexture>(img, addressMode, mipMode);
    }

    return tex;
}

}

Texture::Texture(int w, int h, int d) :
    width(w),
    height(h),
    depth(d)
{
}


int Texture::getLODCount() const
{
    return 1;
}


int Texture::getUTileCount(int /*unused*/) const
{
    return 1;
}


int Texture::getVTileCount(int /*unused*/) const
{
    return 1;
}


int Texture::getWTileCount(int /*unused*/) const
{
    return 1;
}


void Texture::setBorderColor(Color /*unused*/)
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


ImageTexture::ImageTexture(const Image& img,
                           AddressMode addressMode,
                           MipMapMode mipMapMode) :
    Texture(img.getWidth(), img.getHeight()),
    glName(0)
{
    glGenTextures(1, &glName);
    glBindTexture(GL_TEXTURE_2D, glName);

    bool mipmap = mipMapMode != NoMipMaps;
    bool precomputedMipMaps = false;

    // Use precomputed mipmaps only if a complete set is supplied
    int mipLevelCount = img.getMipLevelCount();
    int expectedCount = CalcMipLevelCount(img.getWidth(), img.getHeight());
    if (mipmap && mipLevelCount == expectedCount)
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

    if (gl::EXT_texture_filter_anisotropic && GetTextureCaps().preferredAnisotropy > 1)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, GetTextureCaps().preferredAnisotropy);
    }

    bool genMipmaps = mipmap && !precomputedMipMaps;

    if (mipmap)
    {
        if (precomputedMipMaps)
        {
            LoadMipmapSet(img, GL_TEXTURE_2D);
        }
        else if (mipMapMode == DefaultMipMaps)
        {
            LoadMiplessTexture(img, GL_TEXTURE_2D);
#ifndef GL_ES
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, expectedCount-1);
#endif
        }
    }
    else
    {
        LoadMiplessTexture(img, GL_TEXTURE_2D);
#ifndef GL_ES
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
#endif
    }
    if (genMipmaps)
        glGenerateMipmap(GL_TEXTURE_2D);

    alpha = img.hasAlpha();
    compressed = img.isCompressed();
}


ImageTexture::~ImageTexture()
{
    if (glName != 0)
        glDeleteTextures(1, &glName);
}


void ImageTexture::bind()
{
    glBindTexture(GL_TEXTURE_2D, glName);
}


TextureTile ImageTexture::getTile(int lod, int u, int v)
{
    if (lod != 0 || u != 0 || v != 0)
        return TextureTile(0);

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


TiledTexture::TiledTexture(const Image& img,
                           int _uSplit, int _vSplit,
                           MipMapMode mipMapMode) :
    Texture(img.getWidth(), img.getHeight()),
    uSplit(std::max(1, _uSplit)),
    vSplit(std::max(1, _vSplit)),
    glNames(nullptr)
{
    glNames = new unsigned int[uSplit * vSplit];
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
    int components = img.getComponents();

    // Create a temporary image which we'll use for the tile texels
    int tileWidth = img.getWidth() / uSplit;
    int tileHeight = img.getHeight() / vSplit;
    int tileMipLevelCount = CalcMipLevelCount(tileWidth, tileHeight);
    Image* tile = new Image(img.getFormat(),
                            tileWidth, tileHeight,
                            tileMipLevelCount);

    for (int v = 0; v < vSplit; v++)
    {
        for (int u = 0; u < uSplit; u++)
        {
            // Create the texture and set up sampling and addressing
            glGenTextures(1, &glNames[v * uSplit + u]);
            glBindTexture(GL_TEXTURE_2D, glNames[v * uSplit + u]);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texAddress);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texAddress);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                            mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

            if (gl::EXT_texture_filter_anisotropic && GetTextureCaps().preferredAnisotropy > 1)
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, GetTextureCaps().preferredAnisotropy);
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
                        int mipWidth  = std::max(img.getWidth() >> mip, 1);
                        int tileMipWidth  = std::max(tile->getWidth() >> mip, 1);
                        int tileMipHeight = std::max(tile->getHeight() >> mip, 1);
                        int uBlocks = std::max(tileMipWidth / 4, 1);
                        int vBlocks = std::max(tileMipHeight / 4, 1);
                        int destBytesPerRow = uBlocks * blockSize;
                        int srcBytesPerRow = std::max(mipWidth / 4, 1) * blockSize;
                        int srcU = u * tileMipWidth / 4;
                        int srcV = v * tileMipHeight / 4;
                        int tileOffset = srcV * srcBytesPerRow + srcU * blockSize;

                        const std::uint8_t *imgMip = img.getMipLevel(std::min(mip, mipLevelCount));
                        std::uint8_t *tileMip = tile->getMipLevel(mip);

                        for (int y = 0; y < vBlocks; y++)
                        {
                            std::memcpy(tileMip + y * destBytesPerRow,
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
                    int uBlocks = std::max(tileWidth / 4, 1);
                    int vBlocks = std::max(tileHeight / 4, 1);
                    int destBytesPerRow = uBlocks * blockSize;
                    int srcBytesPerRow = std::max(img.getWidth() / 4, 1) * blockSize;
                    int srcU = u * tileWidth / 4;
                    int srcV = v * tileHeight / 4;
                    int tileOffset = srcV * srcBytesPerRow + srcU * blockSize;

                    for (int y = 0; y < vBlocks; y++)
                    {
                        memcpy(tile->getPixels() + y * destBytesPerRow,
                               img.getPixels() + tileOffset + y * srcBytesPerRow,
                               destBytesPerRow);
                    }
                }
                else
                {
                    const std::uint8_t* tilePixels = img.getPixels() +
                        (v * tileHeight * img.getWidth() + u * tileWidth) * components;
                    for (int y = 0; y < tileHeight; y++)
                    {
                        memcpy(tile->getPixels() + y * tileWidth * components,
                               tilePixels + y * img.getWidth() * components,
                               tileWidth * components);
                    }
                }

                LoadMiplessTexture(*tile, GL_TEXTURE_2D);
                if (mipmap)
                    glGenerateMipmap(GL_TEXTURE_2D);
            }
        }
    }

    delete tile;
}


TiledTexture::~TiledTexture()
{
    if (glNames != nullptr)
    {
        for (int i = 0; i < uSplit * vSplit; i++)
        {
            if (glNames[i] != 0)
                glDeleteTextures(1, &glNames[i]);
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


int TiledTexture::getUTileCount(int /*lod*/) const
{
    return uSplit;
}


int TiledTexture::getVTileCount(int /*lod*/) const
{
    return vSplit;
}


TextureTile TiledTexture::getTile(int lod, int u, int v)
{
    if (lod != 0 || u >= uSplit || u < 0 || v >= vSplit || v < 0)
        return TextureTile(0);

    return TextureTile(glNames[v * uSplit + u]);
}


CubeMap::CubeMap(celestia::util::array_view<const Image*> faces) :
    Texture(faces[0]->getWidth(), faces[0]->getHeight()),
    glName(0)
{
    // Verify that all the faces are square and have the same size
    int width = faces[0]->getWidth();
    PixelFormat format = faces[0]->getFormat();
    for (int i = 0; i < 6; i++)
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

    glGenTextures(1, &glName);
    glBindTexture(GL_TEXTURE_CUBE_MAP, glName);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                    mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

    bool genMipmaps = mipmap && !precomputedMipMaps;

    for (int i = 0; i < 6; i++)
    {
        auto targetFace = static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
        const Image* face = faces[i];

        if (mipmap && precomputedMipMaps)
            LoadMipmapSet(*face, targetFace);
        else
            LoadMiplessTexture(*face, targetFace);
    }
    if (genMipmaps)
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}


CubeMap::~CubeMap()
{
    if (glName != 0)
        glDeleteTextures(1, &glName);
}


void CubeMap::bind()
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, glName);
}


TextureTile CubeMap::getTile(int lod, int u, int v)
{
    if (lod != 0 || u != 0 || v != 0)
        return TextureTile(0);

    return TextureTile(glName);
}


void CubeMap::setBorderColor(Color borderColor)
{
    bind();
    SetBorderColor(borderColor, GL_TEXTURE_CUBE_MAP);
}



std::unique_ptr<Texture>
CreateProceduralTexture(int width, int height,
                        PixelFormat format,
                        ProceduralTexEval func,
                        Texture::AddressMode addressMode,
                        Texture::MipMapMode mipMode)
{
    auto img = std::make_unique<Image>(format, width, height);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            float u = ((float) x + 0.5f) / (float) width * 2 - 1;
            float v = ((float) y + 0.5f) / (float) height * 2 - 1;
            func(u, v, 0, img->getPixelRow(y) + x * img->getComponents());
        }
    }

    return std::make_unique<ImageTexture>(*img, addressMode, mipMode);
}


std::unique_ptr<Texture>
CreateProceduralTexture(int width, int height,
                        PixelFormat format,
                        TexelFunctionObject& func,
                        Texture::AddressMode addressMode,
                        Texture::MipMapMode mipMode)
{
    auto img = std::make_unique<Image>(format, width, height);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            float u = ((float) x + 0.5f) / (float) width * 2 - 1;
            float v = ((float) y + 0.5f) / (float) height * 2 - 1;
            func(u, v, 0, img->getPixelRow(y) + x * img->getComponents());
        }
    }

    return std::make_unique<ImageTexture>(*img, addressMode, mipMode);
}


std::unique_ptr<Texture>
CreateProceduralCubeMap(int size,
                        PixelFormat format,
                        ProceduralTexEval func)
{
    std::array<std::unique_ptr<Image>, 6> faces{ };

    for (int i = 0; i < 6; i++)
    {
        faces[i] = std::make_unique<Image>(format, size, size);

        Image* face = faces[i].get();
        for (int y = 0; y < size; y++)
        {
            for (int x = 0; x < size; x++)
            {
                float s = ((float) x + 0.5f) / (float) size * 2 - 1;
                float t = ((float) y + 0.5f) / (float) size * 2 - 1;
                Eigen::Vector3f v = cubeVector(i, s, t);
                func(v.x(), v.y(), v.z(),
                     face->getPixelRow(y) + x * face->getComponents());
            }
        }
    }

    std::array<const Image*, 6> facePtrs;
    std::transform(faces.begin(), faces.end(), facePtrs.begin(),
                   [](const std::unique_ptr<Image>& iptr) { return iptr.get(); });

    return std::make_unique<CubeMap>(facePtrs);
}


std::unique_ptr<Texture>
LoadTextureFromFile(const fs::path& filename,
                    Texture::AddressMode addressMode,
                    Texture::MipMapMode mipMode,
                    Texture::Colorspace colorspace)
{
    // Check for a Celestia texture--these need to be handled specially.
    ContentType contentType = DetermineFileType(filename);

    if (contentType == ContentType::CelestiaTexture)
        return LoadVirtualTexture(filename);

    // All other texture types are handled by first loading an image, then
    // creating a texture from that image.
    std::unique_ptr<Image> img = Image::load(filename);
    if (img == nullptr)
        return nullptr;

    if (colorspace == Texture::LinearColorspace)
        img->forceLinear();

    std::unique_ptr<Texture> tex = CreateTextureFromImage(*img, addressMode, mipMode);

    if (contentType == ContentType::DXT5NormalMap)
    {
        // If the texture came from a .dxt5nm file then mark it as a dxt5
        // compressed normal map. There's no separate OpenGL format for dxt5
        // normal maps, so the file extension is the only thing that
        // distinguishes it from a plain old dxt5 texture.
        if (img->getFormat() == PixelFormat::DXT5)
        {
            tex->setFormatOptions(Texture::DXT5NormalMap);
        }
    }

    return tex;
}


// Load a height map texture from a file and convert it to a normal map.
std::unique_ptr<Texture>
LoadHeightMapFromFile(const fs::path& filename,
                      float height,
                      Texture::AddressMode addressMode)
{
    auto img = Image::load(filename);
    if (img == nullptr)
        return nullptr;

    img->forceLinear();

    auto normalMap = img->computeNormalMap(height, addressMode == Texture::Wrap);
    if (normalMap == nullptr)
        return nullptr;

    return CreateTextureFromImage(*normalMap, addressMode, Texture::DefaultMipMaps);
}
