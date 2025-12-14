// multirestexture.h
//
// Copyright (C) 2002, Deon Ramsey
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <array>
#include <cstddef>
#include <filesystem>

#include <celutil/reshandle.h>
#include "texmanager.h"

class Texture;

class MultiResTexture
{
public:
    MultiResTexture();
    MultiResTexture(const std::filesystem::path& source, const std::filesystem::path& path);

    void setTexture(const std::filesystem::path& source,
                    const std::filesystem::path& path,
                    TextureFlags flags = TextureFlags::None);
    void setTexture(const std::filesystem::path& source,
                    const std::filesystem::path& path,
                    float bumpHeight,
                    TextureFlags flags);
    Texture* find(TextureResolution resolution);

    ResourceHandle texture(TextureResolution resolution) const { return tex[static_cast<std::size_t>(resolution)]; }

    bool isValid() const;

private:
    std::array<ResourceHandle, 3> tex;
};
