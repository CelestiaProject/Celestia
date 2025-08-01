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

#include <celutil/resmanager.h>
#include "multitexture.h"
#include "texture.h"

class TextureInfo
{
public:
    using ResourceType = Texture;
    using ResourceKey = std::filesystem::path;

    enum
    {
        WrapTexture      = 0x1,
        CompressTexture  = 0x2,
        NoMipMaps        = 0x4,
        AutoMipMaps      = 0x8,
        AllowSplitting   = 0x10,
        BorderClamp      = 0x20,
        LinearColorspace = 0x40,
    };

    TextureInfo(const std::filesystem::path& _source,
                const std::filesystem::path& _path,
                unsigned int _flags,
                TextureResolution _resolution = TextureResolution::medres) :
        source(_source),
        path(_path),
        flags(_flags),
        bumpHeight(0.0f),
        resolution(_resolution) {};

    TextureInfo(const std::filesystem::path& _source,
                const std::filesystem::path& _path,
                float _bumpHeight,
                unsigned int _flags,
                TextureResolution _resolution = TextureResolution::medres) :
        source(_source),
        path(_path),
        flags(_flags),
        bumpHeight(_bumpHeight),
        resolution(_resolution) {};

    TextureInfo(const std::filesystem::path& _source,
                unsigned int _flags,
                TextureResolution _resolution = TextureResolution::medres) :
        source(_source),
        path(""),
        flags(_flags),
        bumpHeight(0.0f),
        resolution(_resolution) {};

    std::filesystem::path resolve(const std::filesystem::path&) const;
    std::unique_ptr<Texture> load(const std::filesystem::path&) const;

private:
    std::filesystem::path source;
    std::filesystem::path path;
    unsigned int flags;
    float bumpHeight;
    TextureResolution resolution;

    friend bool operator<(const TextureInfo&, const TextureInfo&);
};

inline bool operator<(const TextureInfo& ti0, const TextureInfo& ti1)
{
    return std::tie(ti0.resolution, ti0.source, ti0.path) <
           std::tie(ti1.resolution, ti1.source, ti1.path);
}

using TextureManager = ResourceManager<TextureInfo>;

TextureManager* GetTextureManager();
