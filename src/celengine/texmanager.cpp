// texmanager.cpp
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celutil/debug.h>
#include <celutil/fsutils.h>
#include <fstream>
#include <array>
#include "multitexture.h"
#include "texmanager.h"

using namespace std;
using namespace celestia;

static TextureManager* textureManager = nullptr;

static std::array<const char*, 3> directories =
{
    "lores",
    "medres",
    "hires"
};

#ifdef USE_LIBAVIF
static constexpr size_t nExt = 7;
#else
static constexpr size_t nExt = 6;
#endif

static std::array<const char*, nExt> extensions =
{
#ifdef USE_LIBAVIF
    "avif",
#endif
    "png",
    "jpg",
    "jpeg",
    "dds",
    "dxt5nm",
    "ctx"
};

TextureManager* GetTextureManager()
{
    if (textureManager == nullptr)
        textureManager = new TextureManager("textures");
    return textureManager;
}

fs::path TextureInfo::resolve(const fs::path& baseDir)
{
    bool wildcard = source.extension() == ".*";

    if (!path.empty())
    {
        fs::path filename = path / "textures" / directories[resolution] / source;
        // cout << "Resolve: testing [" << filename << "]\n";
        if (wildcard)
        {
            filename = util::ResolveWildcard(filename, extensions);
            if (!filename.empty())
                return filename;
        }
        else
        {
            ifstream in(filename);
            if (in.good())
                return filename;
        }
    }

    fs::path filename = baseDir / directories[resolution] / source;
    if (wildcard)
    {
        fs::path matched = util::ResolveWildcard(filename, extensions);
        if (!matched.empty())
            return matched;
    }

    return filename;
}


Texture* TextureInfo::load(const fs::path& name)
{
    Texture::AddressMode addressMode = Texture::EdgeClamp;
    Texture::MipMapMode mipMode = Texture::DefaultMipMaps;

    if (flags & WrapTexture)
        addressMode = Texture::Wrap;
    else if (flags & BorderClamp)
        addressMode = Texture::BorderClamp;

    if (flags & NoMipMaps)
        mipMode = Texture::NoMipMaps;

    if (bumpHeight == 0.0f)
    {
        DPRINTF(LOG_LEVEL_ERROR, "Loading texture: %s\n", name);
        // cout << "Loading texture: " << name << '\n';

        return LoadTextureFromFile(name, addressMode, mipMode);
    }

    DPRINTF(LOG_LEVEL_ERROR, "Loading bump map: %s\n", name);
    // cout << "Loading texture: " << name << '\n';

    return LoadHeightMapFromFile(name, bumpHeight, addressMode);
}
