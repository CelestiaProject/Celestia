// texmanager.cpp
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "celestia.h"
#include <celutil/debug.h>
#include <iostream>
#include <fstream>
#include "multitexture.h"
#include "texmanager.h"

using namespace std;


static TextureManager* textureManager = NULL;

static const char *directories[]=
{
    "/lores/",
    "/medres/",
    "/hires/"
};


TextureManager* GetTextureManager()
{
    if (textureManager == NULL)
        textureManager = new TextureManager("textures");
    return textureManager;
}


static string resolveWildcard(const string& filename)
{
    string base(filename, 0, filename.length() - 1);

    string pngfile = base + "png";
    {
        ifstream in(pngfile.c_str());
        if (in.good())
            return pngfile;
    }
    string jpgfile = base + "jpg";
    {
        ifstream in(jpgfile.c_str());
        if (in.good())
            return jpgfile;
    }
    string ddsfile = base + "dds";
    {
        cout << "Trying DDS: " << ddsfile << '\n';
        ifstream in(ddsfile.c_str());
        if (in.good())
            return ddsfile;
    }

    return "";
}


string TextureInfo::resolve(const string& baseDir)
{
    bool wildcard = false;
    if (!source.empty() && source.at(source.length() - 1) == '*')
        wildcard = true;

    if (!path.empty())
    {
        string filename = path + "/textures" + directories[resolution] + source;
        // cout << "Resolve: testing [" << filename << "]\n";
        if (wildcard)
        {
            filename = resolveWildcard(filename);
            if (!filename.empty())
                return filename;
        }
        else
        {
            ifstream in(filename.c_str());
            if (in.good())
                return filename;
        }
    }

    string filename = baseDir + directories[resolution] + source;
    if (wildcard)
    {
        string matched = resolveWildcard(filename);
        if (matched.empty())
            return filename; // . . . for lack of any better way to handle it.
        else
            return matched;
    }
    else
    {
        return filename;
    }
}


Texture* TextureInfo::load(const string& name)
{
    if (bumpHeight == 0.0f)
    {
        DPRINTF(0, "Loading texture: %s\n", name.c_str());
        // cout << "Loading texture: " << name << '\n';
        Texture* tex = LoadTextureFromFile(name);

        if (tex != NULL)
        {
            tex->bindName(flags);
            return tex;
        }
    }
    else
    {
        DPRINTF(0, "Loading bump map: %s\n", name.c_str());
        // cout << "Loading texture: " << name << '\n';
        Texture* tex = LoadTextureFromFile(name);

        if (tex != NULL)
        {
            tex->normalMap(bumpHeight, true);
            tex->bindName(flags);
            return tex;
        }
    }

    return NULL;
}

