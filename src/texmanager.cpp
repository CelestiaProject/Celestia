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

using namespace std;


TextureManager::~TextureManager()
{
    // TODO: Clean up
}


bool TextureManager::find(const string& name, CTexture** tex)
{
    return findResource(name, (void**) tex);
}


CTexture* TextureManager::load(const string& name)
{
    DPRINTF("Loading texture: %s\n", name.c_str());
    CTexture* tex = LoadTextureFromFile(baseDir + "\\" + name);
    if (tex != NULL)
        tex->bindName(true);
    addResource(name, (void*) tex);
 
    return tex;
}


CTexture* TextureManager::loadBumpMap(const string& name)
{
    DPRINTF("Loading bump map: %s\n", name.c_str());
    CTexture* tex = LoadTextureFromFile(baseDir + "\\" + name);
    if (tex != NULL)
    {
        tex->normalMap(5.0f, true);
        tex->bindName();
    }
    addResource(name, static_cast<void*>(tex));

    return tex;
}
