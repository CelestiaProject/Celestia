// multirestexture.h
//
// Copyright (C) 2002, Deon Ramsey
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string>

#include <celcompat/filesystem.h>
#include <celutil/reshandle.h>

enum
{
    lores  = 0,
    medres = 1,
    hires  = 2
};

class Texture;

class MultiResTexture
{
public:
    static constexpr int kTextureResolution = 3;

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
    Texture* find(unsigned int resolution);

    bool isValid() const;

    ResourceHandle tex[kTextureResolution];
};
