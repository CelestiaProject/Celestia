// texmanager.cpp
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "celestia.h"
#include "texmanager.h"


TextureManager::~TextureManager()
{
    // TODO: Clean up
}


bool TextureManager::find(string name, CTexture** tex)
{
    return findResource(name, (void**) tex);
}


CTexture* TextureManager::load(string name)
{
    DPRINTF("Loading texture: %s\n", name.c_str());
    CTexture* tex = LoadTextureFromFile(baseDir + "\\" + name);
    if (tex != NULL)
        tex->bindName();
    addResource(name, (void*) tex);
 
    return tex;
}

