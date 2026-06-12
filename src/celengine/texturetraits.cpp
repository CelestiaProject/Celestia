// texturetraits.cpp
//
// Copyright (C) 2026-present, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "texturetraits.h"

#include <utility>

#include <celutil/filetype.h>
#include <celutil/flag.h>

namespace celestia::engine
{

namespace
{

// Convert the SSC-derived TextureFlags into the lower-level enums consumed
// by ImageTexture/TiledTexture. Kept in sync with the legacy LoadTexture()
// in texmanager.cpp.
void
translateFlags(TextureFlags flags,
               Texture::AddressMode& addressMode,
               Texture::MipMapMode& mipMode,
               Texture::Colorspace& colorspace)
{
    addressMode = Texture::EdgeClamp;
    mipMode     = Texture::DefaultMipMaps;
    colorspace  = Texture::DefaultColorspace;

    if (util::is_set(flags, TextureFlags::WrapTexture))
        addressMode = Texture::Wrap;
    else if (util::is_set(flags, TextureFlags::BorderClamp))
        addressMode = Texture::BorderClamp;

    if (util::is_set(flags, TextureFlags::NoMipMaps))
        mipMode = Texture::NoMipMaps;

    if (util::is_set(flags, TextureFlags::LinearColorspace))
        colorspace = Texture::LinearColorspace;
}

} // namespace

std::optional<DecodedTexture>
TextureTraits::decode(const Info& info) const
{
    Texture::AddressMode addressMode;
    Texture::MipMapMode  mipMode;
    Texture::Colorspace  colorspace;
    translateFlags(info.flags, addressMode, mipMode, colorspace);

    // Virtual textures are detected by extension; their loader only does
    // file parsing (no GL), so we run it on the worker too.
    if (DetermineFileType(info.path) == ContentType::CelestiaTexture)
    {
        auto vtex = LoadVirtualTexture(info.path, colorspace);
        if (vtex == nullptr)
            return std::nullopt;

        DecodedTexture out;
        out.virtualTexture = std::move(vtex);
        return out;
    }

    // Bump map: load image, compute the normal map on the worker thread.
    if (info.bumpHeight != 0.0f)
    {
        auto img = Image::load(info.path);
        if (img == nullptr)
            return std::nullopt;

        img->forceLinear();
        auto normalMap = img->computeNormalMap(info.bumpHeight,
                                               addressMode == Texture::Wrap);
        if (normalMap == nullptr)
            return std::nullopt;

        DecodedTexture out;
        out.image        = std::move(normalMap);
        out.addressMode  = addressMode;
        out.mipMode      = Texture::DefaultMipMaps;
        out.dxt5NormalMap = false;
        return out;
    }

    // Ordinary raster texture.
    const ContentType contentType = DetermineFileType(info.path);
    auto img = Image::load(info.path);
    if (img == nullptr)
        return std::nullopt;

    if (colorspace == Texture::LinearColorspace)
        img->forceLinear();

    DecodedTexture out;
    out.dxt5NormalMap = (contentType == ContentType::DXT5NormalMap &&
                         img->getFormat() == PixelFormat::DXT5);
    out.image        = std::move(img);
    out.addressMode  = addressMode;
    out.mipMode      = mipMode;
    return out;
}

std::unique_ptr<Texture>
TextureTraits::upload(CpuData&& cpu) const
{
    // Virtual textures were fully built on the worker; just hand the
    // unique_ptr off.
    if (cpu.virtualTexture != nullptr)
        return std::move(cpu.virtualTexture);

    if (cpu.image == nullptr)
        return nullptr;

    // CreateTextureFromImage performs the actual GL allocation; this is
    // the only step that must run on the render thread.
    auto tex = CreateTextureFromImage(*cpu.image, cpu.addressMode, cpu.mipMode);
    if (tex != nullptr && cpu.dxt5NormalMap)
        tex->setFormatOptions(Texture::DXT5NormalMap);
    return tex;
}

} // namespace celestia::engine
