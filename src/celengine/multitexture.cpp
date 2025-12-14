// multirestexture.cpp
//
// Copyright (C) 2002 Deon Ramsey <dramsey@sourceforge.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "multitexture.h"

#include <cassert>

namespace
{
constexpr auto loresIndex = static_cast<std::size_t>(TextureResolution::lores);
constexpr auto medresIndex = static_cast<std::size_t>(TextureResolution::medres);
constexpr auto hiresIndex = static_cast<std::size_t>(TextureResolution::hires);

}

MultiResTexture::MultiResTexture()
{
    tex.fill(ResourceHandle::InvalidResource);
}


MultiResTexture::MultiResTexture(const std::filesystem::path& source,
                                 const std::filesystem::path& path)
{
    setTexture(source, path);
}


void MultiResTexture::setTexture(const std::filesystem::path& source,
                                 const std::filesystem::path& path,
                                 TextureFlags flags)
{
    TextureManager* texMan = GetTextureManager();
    tex[loresIndex] = texMan->getHandle(TextureInfo(source, path, flags, TextureResolution::lores));
    tex[medresIndex] = texMan->getHandle(TextureInfo(source, path, flags, TextureResolution::medres));
    tex[hiresIndex] = texMan->getHandle(TextureInfo(source, path, flags, TextureResolution::hires));
}


void MultiResTexture::setTexture(const std::filesystem::path& source,
                                 const std::filesystem::path& path,
                                 float bumpHeight,
                                 TextureFlags flags)
{
    TextureManager* texMan = GetTextureManager();
    tex[loresIndex] = texMan->getHandle(TextureInfo(source, path, bumpHeight, flags, TextureResolution::lores));
    tex[medresIndex] = texMan->getHandle(TextureInfo(source, path, bumpHeight, flags, TextureResolution::medres));
    tex[hiresIndex] = texMan->getHandle(TextureInfo(source, path, bumpHeight, flags, TextureResolution::hires));
}


Texture* MultiResTexture::find(TextureResolution resolution)
{
    assert(static_cast<std::size_t>(resolution) < tex.size());
    TextureManager* texMan = GetTextureManager();

    const auto resolutionIndex = static_cast<std::size_t>(resolution);
    if (Texture* res = texMan->find(tex[resolutionIndex]); res != nullptr)
        return res;

    // Preferred resolution isn't available; try the second choice
    // Set these to some defaults to avoid GCC complaints
    // about possible uninitialized variable usage:
    std::size_t secondChoice;
    std::size_t lastResort;
    switch (resolution)
    {
    case TextureResolution::medres:
        secondChoice = loresIndex;
        lastResort = hiresIndex;
        break;
    case TextureResolution::hires:
        secondChoice = medresIndex;
        lastResort = loresIndex;
        break;
    default:
        secondChoice = medresIndex;
        lastResort = hiresIndex;
        break;
    }

    tex[resolutionIndex] = tex[secondChoice];
    if (Texture* res = texMan->find(tex[resolutionIndex]); res != nullptr)
        return res;

    tex[secondChoice] = tex[lastResort];
    tex[resolutionIndex] = tex[lastResort];

    return texMan->find(tex[resolutionIndex]);
}


bool MultiResTexture::isValid() const
{
    return (tex[loresIndex] != ResourceHandle::InvalidResource ||
            tex[medresIndex] != ResourceHandle::InvalidResource ||
            tex[hiresIndex] != ResourceHandle::InvalidResource);
}
