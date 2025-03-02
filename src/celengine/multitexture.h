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

#include <celcompat/filesystem.h>
#include <celutil/reshandle.h>

enum class TextureResolution : int
{
    lores = 0,
    medres,
    hires,
};

class Texture;

class MultiResTexture
{
public:
    MultiResTexture();
    MultiResTexture(ResourceHandle loTex,
                    ResourceHandle medTex = InvalidResource,
                    ResourceHandle hiTex = InvalidResource);
    MultiResTexture(const fs::path& source, const fs::path& path);

    void setTexture(const fs::path& source,
                    const fs::path& path,
                    unsigned int flags = 0);
    void setTexture(const fs::path& source,
                    const fs::path& path,
                    float bumpHeight,
                    unsigned int flags);
    Texture* find(TextureResolution resolution);

    ResourceHandle texture(TextureResolution resolution) const { return tex[static_cast<std::size_t>(resolution)]; }

    bool isValid() const;

private:
    std::array<ResourceHandle, 3> tex;
};
