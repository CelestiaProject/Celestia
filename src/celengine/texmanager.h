// texmanager.h
//
// Copyright (C) 2001-present, Celestia Development Team
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <filesystem>
#include <memory>
#include <tuple>

#include <celutil/flag.h>
#include <celutil/resmanager.h>
#include "texture.h"

enum class TextureResolution : int
{
    lores = 0,
    medres,
    hires,
};

enum class TextureFlags : unsigned int
{
    None             = 0,
    WrapTexture      = 0x1,
    NoMipMaps        = 0x2,
    BorderClamp      = 0x4,
    LinearColorspace = 0x8,
};

ENUM_CLASS_BITWISE_OPS(TextureFlags)

class TextureInfo
{
public:
    using ResourceType = Texture;
    using ResourceKey = std::filesystem::path;

    TextureInfo(const std::filesystem::path& _source,
                const std::filesystem::path& _path,
                TextureFlags _flags,
                TextureResolution _resolution = TextureResolution::medres) :
        source(_source),
        path(_path),
        flags(_flags),
        resolution(_resolution) {};

    TextureInfo(const std::filesystem::path& _source,
                const std::filesystem::path& _path,
                float _bumpHeight,
                TextureFlags _flags,
                TextureResolution _resolution = TextureResolution::medres) :
        source(_source),
        path(_path),
        flags(_flags),
        bumpHeight(_bumpHeight),
        resolution(_resolution) {};

    TextureInfo(const std::filesystem::path& _source,
                TextureFlags _flags,
                TextureResolution _resolution = TextureResolution::medres) :
        source(_source),
        flags(_flags),
        resolution(_resolution) {};

    std::filesystem::path resolve(const std::filesystem::path&) const;
    std::unique_ptr<Texture> load(const std::filesystem::path&) const;

private:
    std::filesystem::path source;
    std::filesystem::path path;
    TextureFlags flags;
    float bumpHeight{ 0.0f };
    TextureResolution resolution;

    friend bool operator<(const TextureInfo&, const TextureInfo&);
};

inline bool operator<(const TextureInfo& ti0, const TextureInfo& ti1)
{
    return std::tie(ti0.resolution, ti0.source, ti0.path, ti0.flags, ti0.bumpHeight) <
           std::tie(ti1.resolution, ti1.source, ti1.path, ti1.flags, ti1.bumpHeight);
}

using TextureManager = ResourceManager<TextureInfo>;

TextureManager* GetTextureManager();
