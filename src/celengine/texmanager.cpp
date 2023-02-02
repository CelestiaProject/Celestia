// texmanager.cpp
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "texmanager.h"

#include <array>
#include <cstddef>
#include <fstream>
#include <string_view>

#include <celutil/fsutils.h>
#include <celutil/logger.h>

using namespace std::string_view_literals;
using celestia::util::GetLogger;

namespace
{

constexpr std::array<std::string_view, 3> directories =
{
    "lores"sv,
    "medres"sv,
    "hires"sv,
};

constexpr std::array extensions =
{
#ifdef USE_LIBAVIF
    "avif"sv,
#endif
    "png"sv,
    "jpg"sv,
    "jpeg"sv,
    "dds"sv,
    "dxt5nm"sv,
    "ctx"sv,
};

} // end unnamed namespace

TextureManager*
GetTextureManager()
{
    static TextureManager* textureManager = nullptr;
    if (textureManager == nullptr)
        textureManager = std::make_unique<TextureManager>("textures").release();
    return textureManager;
}

fs::path
TextureInfo::resolve(const fs::path& baseDir) const
{
    bool wildcard = source.extension() == ".*";

    if (!path.empty())
    {
        fs::path filename = path / "textures" / directories[resolution] / source;
        // cout << "Resolve: testing [" << filename << "]\n";
        if (wildcard)
        {
            filename = celestia::util::ResolveWildcard(filename, extensions);
            if (!filename.empty())
                return filename;
        }
        else
        {
            std::ifstream in(filename);
            if (in.good())
                return filename;
        }
    }

    fs::path filename = baseDir / directories[resolution] / source;
    if (wildcard)
    {
        fs::path matched = celestia::util::ResolveWildcard(filename, extensions);
        if (!matched.empty())
            return matched;
    }

    return filename;
}


std::unique_ptr<Texture>
TextureInfo::load(const fs::path& name) const
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
        GetLogger()->debug("Loading texture: {}\n", name);
        return LoadTextureFromFile(name, addressMode, mipMode);
    }

    GetLogger()->debug("Loading bump map: {}\n", name);
    return LoadHeightMapFromFile(name, bumpHeight, addressMode);
}
