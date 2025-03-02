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

#include "texmanager.h"

namespace
{
constexpr auto loresIndex = static_cast<std::size_t>(TextureResolution::lores);
constexpr auto medresIndex = static_cast<std::size_t>(TextureResolution::medres);
constexpr auto hiresIndex = static_cast<std::size_t>(TextureResolution::hires);
}

MultiResTexture::MultiResTexture()
{
    tex[loresIndex] = InvalidResource;
    tex[medresIndex] = InvalidResource;
    tex[hiresIndex] = InvalidResource;
}


MultiResTexture::MultiResTexture(ResourceHandle loTex,
                                 ResourceHandle medTex,
                                 ResourceHandle hiTex)
{
    tex[loresIndex] = loTex;
    tex[medresIndex] = medTex;
    tex[hiresIndex] = hiTex;
}


MultiResTexture::MultiResTexture(const fs::path& source,
                                 const fs::path& path)
{
    setTexture(source, path);
}


void MultiResTexture::setTexture(const fs::path& source,
                                 const fs::path& path,
                                 unsigned int flags)
{
    TextureManager* texMan = GetTextureManager();
    tex[loresIndex] = texMan->getHandle(TextureInfo(source, path, flags, TextureResolution::lores));
    tex[medresIndex] = texMan->getHandle(TextureInfo(source, path, flags, TextureResolution::medres));
    tex[hiresIndex] = texMan->getHandle(TextureInfo(source, path, flags, TextureResolution::hires));
}


void MultiResTexture::setTexture(const fs::path& source,
                                 const fs::path& path,
                                 float bumpHeight,
                                 unsigned int flags)
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
    Texture* res = texMan->find(tex[resolutionIndex]);
    if (res != nullptr)
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
    res = texMan->find(tex[resolutionIndex]);
    if (res != nullptr)
        return res;

    tex[resolutionIndex] = tex[lastResort];

    return texMan->find(tex[resolutionIndex]);
}


bool MultiResTexture::isValid() const
{
    return (tex[loresIndex] != InvalidResource ||
            tex[medresIndex] != InvalidResource ||
            tex[hiresIndex] != InvalidResource);
}
