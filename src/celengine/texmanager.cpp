// texmanager.cpp
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <config.h>
#include <celutil/debug.h>
#include <iostream>
#include <fstream>
#include "multitexture.h"
#include "texmanager.h"

using namespace std;


static TextureManager* textureManager = nullptr;

static const char *directories[]=
{
    "lores",
    "medres",
    "hires"
};


TextureManager* GetTextureManager()
{
    if (textureManager == nullptr)
        textureManager = new TextureManager("textures");
    return textureManager;
}


static string resolveWildcard(const string& filename)
{
    string base(filename, 0, filename.length() - 1);

    string pngfile = base + "png";
    {
        ifstream in(pngfile);
        if (in.good())
            return pngfile;
    }
    string jpgfile = base + "jpg";
    {
        ifstream in(jpgfile);
        if (in.good())
            return jpgfile;
    }
    string ddsfile = base + "dds";
    {
        ifstream in(ddsfile);
        if (in.good())
            return ddsfile;
    }
    string dxt5file = base + "dxt5nm";
    {
        ifstream in(dxt5file);
        if (in.good())
            return dxt5file;
    }
    string ctxfile = base + "ctx";
    {
        ifstream in(ctxfile);
        if (in.good())
            return ctxfile;
    }

    return "";
}


fs::path TextureInfo::resolve(const fs::path& baseDir)
{
    bool wildcard = false;
    if (!source.empty() && source.at(source.length() - 1) == '*')
        wildcard = true;

    if (!path.empty())
    {
        fs::path filename = path / "textures" / directories[resolution] / source;
        // cout << "Resolve: testing [" << filename << "]\n";
        if (wildcard)
        {
            filename = resolveWildcard(filename.string());
            if (!filename.empty())
                return filename;
        }
        else
        {
            ifstream in(filename.string());
            if (in.good())
                return filename;
        }
    }

    fs::path filename = baseDir / directories[resolution] / source;
    if (wildcard)
    {
        string matched = resolveWildcard(filename.string());
        if (matched.empty())
            return filename; // . . . for lack of any better way to handle it.
        else
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
    else if (flags & AutoMipMaps)
        mipMode = Texture::AutoMipMaps;

    if (bumpHeight == 0.0f)
    {
        DPRINTF(LOG_LEVEL_ERROR, "Loading texture: %s\n", name.c_str());
        // cout << "Loading texture: " << name << '\n';

        return LoadTextureFromFile(name, addressMode, mipMode);
    }

    DPRINTF(LOG_LEVEL_ERROR, "Loading bump map: %s\n", name.c_str());
    // cout << "Loading texture: " << name << '\n';

    return LoadHeightMapFromFile(name, bumpHeight, addressMode);
}
