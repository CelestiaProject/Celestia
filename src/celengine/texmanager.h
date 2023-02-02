// texmanager.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>
#include <tuple>

#include <celcompat/filesystem.h>
#include <celutil/resmanager.h>
#include "multitexture.h"
#include "texture.h"

class TextureInfo
{
 private:
    fs::path source;
    fs::path path;
    unsigned int flags;
    float bumpHeight;
    unsigned int resolution;

    friend bool operator<(const TextureInfo&, const TextureInfo&);

 public:
    using ResourceType = Texture;
    using ResourceKey = fs::path;

    enum {
        WrapTexture      = 0x1,
        CompressTexture  = 0x2,
        NoMipMaps        = 0x4,
        AutoMipMaps      = 0x8,
        AllowSplitting   = 0x10,
        BorderClamp      = 0x20,
    };

    TextureInfo(const fs::path& _source,
                const fs::path& _path,
                unsigned int _flags,
                unsigned int _resolution = medres) :
        source(_source),
        path(_path),
        flags(_flags),
        bumpHeight(0.0f),
        resolution(_resolution) {};

    TextureInfo(const fs::path& _source,
                const fs::path& _path,
                float _bumpHeight,
                unsigned int _flags,
                unsigned int _resolution = medres) :
        source(_source),
        path(_path),
        flags(_flags),
        bumpHeight(_bumpHeight),
        resolution(_resolution) {};

    TextureInfo(const fs::path& _source,
                unsigned int _flags,
                unsigned int _resolution = medres) :
        source(_source),
        path(""),
        flags(_flags),
        bumpHeight(0.0f),
        resolution(_resolution) {};

    fs::path resolve(const fs::path&) const;
    std::unique_ptr<Texture> load(const fs::path&) const;
};

inline bool operator<(const TextureInfo& ti0, const TextureInfo& ti1)
{
    return std::tie(ti0.resolution, ti0.source, ti0.path) <
           std::tie(ti1.resolution, ti1.source, ti1.path);
}

using TextureManager = ResourceManager<TextureInfo>;

extern TextureManager* GetTextureManager();
