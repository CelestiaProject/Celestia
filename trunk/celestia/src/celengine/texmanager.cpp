// texmanager.cpp
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celutil/debug.h>
#include "celestia.h"
#include "texmanager.h"

using namespace std;


static TextureManager* textureManager = NULL;


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
        DPRINTF("Loading texture: %s\n", source.c_str());
        Texture* tex = LoadTextureFromFile(baseDir + "/" + source);

        if (tex != NULL)
        {
            uint32 texFlags = Texture::WrapTexture;
            if (compressed)
                texFlags |= Texture::CompressTexture;
            tex->bindName(texFlags);
            return tex;
        }
    }
    else
    {
        DPRINTF("Loading bump map: %s\n", source.c_str());
        Texture* tex = LoadTextureFromFile(baseDir + "/" + source);

        if (tex != NULL)
        {
            tex->normalMap(bumpHeight, true);
            tex->bindName(Texture::WrapTexture);
            return tex;
        }
    }

    return NULL;
}

