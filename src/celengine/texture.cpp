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
#include <vector>

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
using celestia::engine::expandLuminanceToRGBA;
using celestia::engine::expandLuminanceAlphaToRGBA;

namespace
{

// When sRGB rendering is disabled, strip sRGB pixel formats to their
// non-sRGB equivalents so the GPU does not linearize on sample.
PixelFormat
effectiveFormat(PixelFormat format)
{
    if (gl::sRGBRendering)
        return format;

    switch (format)
    {
    case PixelFormat::sRGB:       return PixelFormat::RGB;
    case PixelFormat::sRGB8:      return PixelFormat::RGB8;
    case PixelFormat::sRGBA:      return PixelFormat::RGBA;
    case PixelFormat::sRGBA8:     return PixelFormat::RGBA8;
    case PixelFormat::sLuminance: return PixelFormat::Luminance;
    case PixelFormat::sLumAlpha:  return PixelFormat::LumAlpha;
    case PixelFormat::DXT1_sRGBA: return PixelFormat::DXT1;
    case PixelFormat::DXT3_sRGBA: return PixelFormat::DXT3;
    case PixelFormat::DXT5_sRGBA: return PixelFormat::DXT5;
    case PixelFormat::BC7_sRGBA:  return PixelFormat::BC7;
    default:                      return format;
    }
}

struct TextureCaps
{
    GLint preferredAnisotropy;
};

const TextureCaps&
GetTextureCaps()
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

bool
needsRGBAExpansion(PixelFormat format, [[maybe_unused]] bool needsMipmap)
{
    format = effectiveFormat(format);
#ifdef GL_ES
    // GLES 3.0 sRGB 3-component textures (sRGB/sRGB8) don't support mipmap
    // generation; expand to 4-component when mipmaps are needed.
    // sLuminance has no core single-channel sRGB format; use the GL_SR8 fast
    // path + swizzle when EXT_texture_sRGB_R8 is available, otherwise expand.
    // GL_SR8 is texture-filterable but NOT color-renderable, so glGenerateMipmap
    // fails on it; fall back to expansion (sRGBA8) whenever mipmaps are needed.
    // sLumAlpha always needs expansion (no per-channel sRGB-decode format).
    if (!needsMipmap)
        return (format == PixelFormat::sLuminance && !gl::EXT_texture_sRGB_R8)
            || format == PixelFormat::sLumAlpha;
    return format == PixelFormat::sRGB
        || format == PixelFormat::sRGB8
        || format == PixelFormat::sLuminance
        || format == PixelFormat::sLumAlpha;
#else
    // Desktop: only sLumAlpha needs expansion; see expansion site below.
    return format == PixelFormat::sLumAlpha;
#endif
}

std::unique_ptr<std::uint8_t[]>
expandToRGBA(const std::uint8_t* src, int width, int height, PixelFormat format)
{
    if (format == PixelFormat::sLuminance)
        return expandLuminanceToRGBA(src, width, height);
    if (format == PixelFormat::sLumAlpha)
        return expandLuminanceAlphaToRGBA(src, width, height);

    // RGB → RGBA
    auto dst = std::make_unique<std::uint8_t[]>(width * height * 4);
    for (int i = 0; i < width * height; ++i)
    {
        dst[i * 4 + 0] = src[i * 3 + 0];
        dst[i * 4 + 1] = src[i * 3 + 1];
        dst[i * 4 + 2] = src[i * 3 + 2];
        dst[i * 4 + 3] = 255;
    }
    return dst;
}

GLenum
getInternalFormat(PixelFormat format, bool needsMipmap)
{
    format = effectiveFormat(format);
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
    case PixelFormat::DXT1_sRGBA:
    case PixelFormat::DXT3_sRGBA:
    case PixelFormat::DXT5_sRGBA:
    case PixelFormat::BC7:
    case PixelFormat::BC7_sRGBA:
        return static_cast<GLenum>(format);
    case PixelFormat::sRGB:
        if (!needsRGBAExpansion(format, needsMipmap))
            return static_cast<GLenum>(PixelFormat::sRGB8);
        [[fallthrough]];
    case PixelFormat::sRGBA:
    case PixelFormat::sLumAlpha:
        return static_cast<GLenum>(PixelFormat::sRGBA8);
    // sLuminance fast path: GL_SR8 + swizzle when EXT_texture_sRGB_R8 is
    // present and mipmaps aren't needed (GL_SR8 isn't color-renderable, so
    // glGenerateMipmap fails); otherwise CPU-expand to sRGBA8.
    case PixelFormat::sLuminance:
        return needsRGBAExpansion(format, needsMipmap)
                   ? static_cast<GLenum>(PixelFormat::sRGBA8)
                   : static_cast<GLenum>(PixelFormat::sR8);
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
    case PixelFormat::DXT1:
    case PixelFormat::DXT3:
    case PixelFormat::DXT5:
    case PixelFormat::BC7:
    case PixelFormat::sRGB:
    case PixelFormat::sRGBA:
    case PixelFormat::DXT1_sRGBA:
    case PixelFormat::DXT3_sRGBA:
    case PixelFormat::DXT5_sRGBA:
    case PixelFormat::BC7_sRGBA:
        return static_cast<GLenum>(format);
    // GL_LUMINANCE/ALPHA/LUMINANCE_ALPHA were removed in 3.2 Core; map to
    // R8/RG8 + ApplyLegacyFormatSwizzle to keep legacy shader visibility.
    case PixelFormat::Luminance:
    case PixelFormat::Alpha:
        return GL_R8;
    case PixelFormat::LumAlpha:
        return GL_RG8;
    // No core single/two-channel sRGB format. Use GL_SR8 when available
    // (saves 4x VRAM); otherwise fall back to GL_SRGB8_ALPHA8 + swizzle.
    case PixelFormat::sLuminance:
        return gl::EXT_texture_sRGB_R8 ? static_cast<GLenum>(PixelFormat::sR8)
                                       : GL_SRGB8_ALPHA8;
    // sLumAlpha is CPU-expanded to (L,L,L,A); see needsRGBAExpansion.
    // GL_SRG8 has no fast path here (would sRGB-decode alpha).
    case PixelFormat::sLumAlpha:
        return GL_SRGB8_ALPHA8;
    default:
        return GL_NONE;
    }
#endif
}

GLenum
getExternalFormat(PixelFormat format, bool needsMipmap)
{
    format = effectiveFormat(format);
#ifdef GL_ES
    switch (format)
    {
    case PixelFormat::sRGB:
    case PixelFormat::sRGB8:
        if (!needsRGBAExpansion(format, needsMipmap))
            return static_cast<GLenum>(PixelFormat::RGB);
        [[fallthrough]];
    case PixelFormat::sRGBA:
    case PixelFormat::sRGBA8:
    case PixelFormat::sLumAlpha:
        return static_cast<GLenum>(PixelFormat::RGBA);
    // sLuminance with the GL_SR8 fast path uploads as GL_RED; when expanded
    // (no extension, or mipmaps needed) it's uploaded as RGBA.
    case PixelFormat::sLuminance:
        return needsRGBAExpansion(format, needsMipmap)
                   ? static_cast<GLenum>(PixelFormat::RGBA)
                   : static_cast<GLenum>(GL_RED);
    default:
        return getInternalFormat(format, needsMipmap);
    }
#else
    switch (format)
    {
    case PixelFormat::RGBA:
    case PixelFormat::BGRA:
    case PixelFormat::RGB:
    case PixelFormat::BGR:
    case PixelFormat::DXT1:
    case PixelFormat::DXT3:
    case PixelFormat::DXT5:
    case PixelFormat::BC7:
        return static_cast<GLenum>(format);
    case PixelFormat::Luminance:
    case PixelFormat::sLuminance:
    case PixelFormat::Alpha:
        return GL_RED;
    case PixelFormat::LumAlpha:
        return GL_RG;
    // sLumAlpha is pre-expanded to (L,L,L,A); upload as RGBA.
    case PixelFormat::sLumAlpha:
        return GL_RGBA;
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
    case PixelFormat::BC7_sRGBA:
        return static_cast<GLenum>(PixelFormat::BC7);
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

// Restore legacy GL_LUMINANCE/ALPHA/LUMINANCE_ALPHA shader visibility via
// texture swizzle, since those formats are gone in 3.2 Core. On GLES this is
// also used for the GL_SR8 sLuminance fast path (single-channel sRGB).
void
ApplyLegacyFormatSwizzle(PixelFormat format, GLenum target)
{
    format = effectiveFormat(format);
    std::array<GLint, 4> swizzle{};
#ifndef GL_ES
    if (format == PixelFormat::Luminance)
    {
        swizzle = { GL_RED, GL_RED, GL_RED, GL_ONE };
    }
    else if (format == PixelFormat::Alpha)
    {
        swizzle = { GL_ZERO, GL_ZERO, GL_ZERO, GL_RED };
    }
    else if (format == PixelFormat::LumAlpha)
    {
        swizzle = { GL_RED, GL_RED, GL_RED, GL_GREEN };
    }
    else
#endif
    if (format == PixelFormat::sLuminance)
    {
#ifdef GL_ES
        if (!gl::EXT_texture_sRGB_R8)
            return;
#endif
        swizzle = { GL_RED, GL_RED, GL_RED, GL_ONE };
    }
    else
    {
        // sLumAlpha is pre-expanded into the right slots; no swizzle needed.
        return;
    }
#ifndef GL_ES
    glTexParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, swizzle.data());
#else
    // GLES 3.0 lacks the vector form; set each channel individually.
    glTexParameteri(target, GL_TEXTURE_SWIZZLE_R, swizzle[0]);
    glTexParameteri(target, GL_TEXTURE_SWIZZLE_G, swizzle[1]);
    glTexParameteri(target, GL_TEXTURE_SWIZZLE_B, swizzle[2]);
    glTexParameteri(target, GL_TEXTURE_SWIZZLE_A, swizzle[3]);
#endif
}

// Load a prebuilt set of mipmaps; assumes that the image contains
// a complete set of mipmap levels.
void
LoadMipmapSet(const Image& img, GLenum target, bool needsMipmap)
{
    int internalFormat = getInternalFormat(img.getFormat(), needsMipmap);
#ifndef GL_ES
    glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, img.getMipLevelCount()-1);
#endif
    ApplyLegacyFormatSwizzle(img.getFormat(), target);

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
        else if (needsRGBAExpansion(img.getFormat(), needsMipmap))
        {
            auto expanded = expandToRGBA(img.getMipLevel(mip), mipWidth, mipHeight, img.getFormat());
            glTexImage2D(target, mip, internalFormat, mipWidth, mipHeight, 0,
                         getExternalFormat(img.getFormat(), needsMipmap), GL_UNSIGNED_BYTE, expanded.get());
        }
        else
        {
            glTexImage2D(target, mip, internalFormat, mipWidth, mipHeight, 0,
                         getExternalFormat(img.getFormat(), needsMipmap), GL_UNSIGNED_BYTE, img.getMipLevel(mip));
        }
    }
}

// Load a texture without any mipmaps
void
LoadMiplessTexture(const Image& img, GLenum target, bool needsMipmap)
{
    int internalFormat = getInternalFormat(img.getFormat(), needsMipmap);
    ApplyLegacyFormatSwizzle(img.getFormat(), target);

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
    else if (needsRGBAExpansion(img.getFormat(), needsMipmap))
    {
        auto expanded = expandToRGBA(img.getMipLevel(0), img.getWidth(), img.getHeight(), img.getFormat());
        glTexImage2D(target, 0, internalFormat, img.getWidth(), img.getHeight(), 0,
                     getExternalFormat(img.getFormat(), needsMipmap), GL_UNSIGNED_BYTE, expanded.get());
    }
    else
    {
        glTexImage2D(target, 0, internalFormat, img.getWidth(), img.getHeight(), 0,
                     getExternalFormat(img.getFormat(), needsMipmap), GL_UNSIGNED_BYTE, img.getMipLevel(0));
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

void
LoadPrecomputedTileMipMaps(const Image& img, Image& tile,
                           int u, int v,
                           int mipLevelCount, int tileMipLevelCount)
{
    if (img.isCompressed())
    {
        for (int mip = 0; mip < tileMipLevelCount; mip++)
        {
            int blockSize = getCompressedBlockSize(img.getFormat());
            int mipWidth  = std::max(img.getWidth() >> mip, 1);
            int tileMipWidth  = std::max(tile.getWidth() >> mip, 1);
            int tileMipHeight = std::max(tile.getHeight() >> mip, 1);
            int uBlocks = std::max(tileMipWidth / 4, 1);
            int vBlocks = std::max(tileMipHeight / 4, 1);
            int destBytesPerRow = uBlocks * blockSize;
            int srcBytesPerRow = std::max(mipWidth / 4, 1) * blockSize;
            int srcU = u * tileMipWidth / 4;
            int srcV = v * tileMipHeight / 4;
            int tileOffset = srcV * srcBytesPerRow + srcU * blockSize;

            const std::uint8_t *imgMip = img.getMipLevel(std::min(mip, mipLevelCount));
            std::uint8_t *tileMip = tile.getMipLevel(mip);

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

    LoadMipmapSet(tile, GL_TEXTURE_2D, false);
}

void
ComputeTileMipMaps(const Image& img, Image& tile,
                   int components,
                   int u, int v, bool mipmap)
{
    const auto tileWidth = tile.getWidth();
    const auto tileHeight = tile.getHeight();
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
            std::memcpy(tile.getPixels() + y * destBytesPerRow,
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
            std::memcpy(tile.getPixels() + y * tileWidth * components,
                        tilePixels + y * img.getWidth() * components,
                        tileWidth * components);
        }
    }

    bool genMipmaps = mipmap;

    LoadMiplessTexture(tile, GL_TEXTURE_2D, genMipmaps);
    if (genMipmaps)
        glGenerateMipmap(GL_TEXTURE_2D);
}

} // end unnamed namespace

Texture::Texture(int _width, int _height, bool _alpha) :
    width(_width),
    height(_height),
    alpha(_alpha)
{
}

int
Texture::getLODCount() const
{
    return 1;
}

int
Texture::getUTileCount(int) const
{
    return 1;
}

int
Texture::getVTileCount(int) const
{
    return 1;
}


int
Texture::getWTileCount(int) const
{
    return 1;
}

void
Texture::setBorderColor(Color)
{
    // no-op
}

int
Texture::getWidth() const
{
    return width;
}

int
Texture::getHeight() const
{
    return height;
}

unsigned int
Texture::getFormatOptions() const
{
    return formatOptions;
}

void
Texture::setFormatOptions(unsigned int opts)
{
    formatOptions = opts;
}

ImageTexture::ImageTexture(const Image& img,
                           AddressMode addressMode,
                           MipMapMode mipMapMode) :
    Texture(img.getWidth(), img.getHeight(), img.hasAlpha()),
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
            LoadMipmapSet(img, GL_TEXTURE_2D, genMipmaps);
        }
        else if (mipMapMode == DefaultMipMaps)
        {
            LoadMiplessTexture(img, GL_TEXTURE_2D, genMipmaps);
#ifndef GL_ES
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, expectedCount-1);
#endif
        }
    }
    else
    {
        LoadMiplessTexture(img, GL_TEXTURE_2D, genMipmaps);
#ifndef GL_ES
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
#endif
    }
    if (genMipmaps)
        glGenerateMipmap(GL_TEXTURE_2D);
}

ImageTexture::~ImageTexture()
{
    glDeleteTextures(1, &glName);
}

void
ImageTexture::bind()
{
    glBindTexture(GL_TEXTURE_2D, glName);
}

TextureTile
ImageTexture::getTile(int lod, int u, int v)
{
    if (lod != 0 || u != 0 || v != 0)
        return TextureTile(0);

    return TextureTile(glName);
}

unsigned int
ImageTexture::getName() const
{
    return glName;
}

void
ImageTexture::setBorderColor(Color borderColor)
{
    bind();
    SetBorderColor(borderColor, GL_TEXTURE_2D);
}

TiledTexture::TiledTexture(const Image& img,
                           int _uSplit, int _vSplit,
                           MipMapMode mipMapMode) :
    Texture(img.getWidth(), img.getHeight(), img.hasAlpha()),
    uSplit(std::max(1, _uSplit)),
    vSplit(std::max(1, _vSplit))
{
    // make_unique performs value-initialization -> sets to 0
    glNames = std::make_unique<unsigned int[]>(uSplit * vSplit);

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
    Image tile(img.getFormat(),
               tileWidth, tileHeight,
               tileMipLevelCount);

    glGenTextures(uSplit * vSplit, glNames.get());
    for (int v = 0; v < vSplit; ++v)
    {
        for (int u = 0; u < uSplit; ++u)
        {
            // Set up sampling and addressing
            glBindTexture(GL_TEXTURE_2D, glNames[v * uSplit + u]);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texAddress);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texAddress);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                            mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

            if (gl::EXT_texture_filter_anisotropic && GetTextureCaps().preferredAnisotropy > 1)
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, GetTextureCaps().preferredAnisotropy);

            // Copy texels from the subtexture area to the pixel buffer.  This
            // is straightforward for normal textures, but an immense headache
            // for compressed textures with prebuilt mipmaps.
            if (precomputedMipMaps)
                LoadPrecomputedTileMipMaps(img, tile, u, v, mipLevelCount, tileMipLevelCount);
            else
                ComputeTileMipMaps(img, tile, components, u, v, mipmap);
        }
    }
}

TiledTexture::~TiledTexture()
{
    if (glNames)
        glDeleteTextures(uSplit * vSplit, glNames.get());
}

void
TiledTexture::bind()
{
    // nothing to do
}

void
TiledTexture::setBorderColor(Color borderColor)
{
    for (int i = 0; i < vSplit; ++i)
    {
        for (int j = 0; j < uSplit; ++j)
        {
            glBindTexture(GL_TEXTURE_2D, glNames[i * uSplit + j]);
            SetBorderColor(borderColor, GL_TEXTURE_2D);
        }
    }
}

int
TiledTexture::getUTileCount(int /*lod*/) const
{
    return uSplit;
}

int
TiledTexture::getVTileCount(int /*lod*/) const
{
    return vSplit;
}

TextureTile
TiledTexture::getTile(int lod, int u, int v)
{
    if (lod != 0 || u >= uSplit || u < 0 || v >= vSplit || v < 0)
        return TextureTile(0);

    return TextureTile(glNames[v * uSplit + u]);
}

CubeMap::CubeMap(celestia::util::array_view<Image> faces) :
    Texture(faces[0].getWidth(), faces[0].getHeight()),
    glName(0)
{
    // Verify that all the faces are square and have the same size
    assert(faces.size() == 6);
    const Image& firstFace = faces[0];
    std::int32_t width = firstFace.getWidth();
    if (firstFace.getHeight() != width ||
        std::any_of(faces.begin() + 1, faces.end(),
                    [&firstFace](const Image& face)
                    {
                        return face.getWidth() != firstFace.getWidth() ||
                               face.getHeight() != firstFace.getHeight() ||
                               face.getFormat() != firstFace.getFormat() ||
                               face.getMipLevelCount() != firstFace.getMipLevelCount() ||
                               face.isCompressed() != firstFace.isCompressed();
                    }))
    {
        return;
    }

    // For now, always enable mipmaps; in the future, it should be possible to
    // override this.
    bool mipmap = true;
    bool precomputedMipMaps = false;

    // Require a complete set of mipmaps
    int mipLevelCount = firstFace.getMipLevelCount();
    if (mipmap && mipLevelCount == CalcMipLevelCount(width, width))
        precomputedMipMaps = true;

    // We can't automatically generate mipmaps for compressed textures.
    // If a precomputed mipmap set isn't provided, turn of mipmapping entirely.
    if (!precomputedMipMaps && firstFace.isCompressed())
        mipmap = false;

    glGenTextures(1, &glName);
    glBindTexture(GL_TEXTURE_CUBE_MAP, glName);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                    mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

    bool genMipmaps = mipmap && !precomputedMipMaps;

    for (int i = 0; i < 6; ++i)
    {
        auto targetFace = static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
        const Image& face = faces[i];

        if (mipmap && precomputedMipMaps)
            LoadMipmapSet(face, targetFace, genMipmaps);
        else
            LoadMiplessTexture(face, targetFace, genMipmaps);
    }
    if (genMipmaps)
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

CubeMap::~CubeMap()
{
    glDeleteTextures(1, &glName);
}

void
CubeMap::bind()
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

// Helper function for createProcedural; return the normalized
// vector pointing to (s, t) on the specified face.
Eigen::Vector3f
CubeMap::cubeVector(int face, float s, float t)
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
LoadTextureFromFile(const std::filesystem::path& filename,
                    Texture::AddressMode addressMode,
                    Texture::MipMapMode mipMode,
                    Texture::Colorspace colorspace)
{
    // Check for a Celestia texture--these need to be handled specially.
    ContentType contentType = DetermineFileType(filename);

    if (contentType == ContentType::CelestiaTexture)
        return LoadVirtualTexture(filename, colorspace);

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
LoadHeightMapFromFile(const std::filesystem::path& filename,
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
