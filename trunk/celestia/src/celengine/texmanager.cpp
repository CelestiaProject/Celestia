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


Texture* TextureInfo::load(const string& baseDir)
{
    if (bumpHeight == 0.0f)
    {
        DPRINTF(0, "Loading texture: %s%s%s\n", baseDir.c_str(), directories[resolution], source.c_str());
        Texture* tex = LoadTextureFromFile(baseDir + directories[resolution] + source);

        if (tex != NULL)
        {
            tex->bindName(flags);
            return tex;
        }
    }
    else
    {
        DPRINTF(0, "Loading bump map: %s%s%s\n", baseDir.c_str(), directories[resolution], source.c_str());
        Texture* tex = LoadTextureFromFile(baseDir + directories[resolution] + source);

        if (tex != NULL)
        {
            tex->normalMap(bumpHeight, true);
            tex->bindName(flags);
            return tex;
        }
    }

    return NULL;
}

