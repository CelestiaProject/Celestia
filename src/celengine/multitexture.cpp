// multirestexture.cpp
//
// Copyright (C) 2002 Deon Ramsey <dramsey@sourceforge.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "multitexture.h"
#include "texmanager.h"
#include <celutil/debug.h>

using namespace std;


MultiResTexture::MultiResTexture()
{
    tex[lores] = InvalidResource;
    tex[medres] = InvalidResource;
    tex[hires] = InvalidResource;
}


MultiResTexture::MultiResTexture(ResourceHandle loTex,
                                 ResourceHandle medTex,
                                 ResourceHandle hiTex)
{
    tex[lores] = loTex;
    tex[medres] = medTex;
    tex[hires] = hiTex;
}


MultiResTexture::MultiResTexture(const string& source,
                                 const string& path)
{
    setTexture(source, path);
}


void MultiResTexture::setTexture(const string& source, const string& path,
                                 unsigned int flags)
{
    TextureManager* texMan = GetTextureManager();
    tex[lores] = texMan->getHandle(TextureInfo(source, path, flags, lores));
    tex[medres] = texMan->getHandle(TextureInfo(source, path, flags, medres));
    tex[hires] = texMan->getHandle(TextureInfo(source, path, flags, hires));
}


void MultiResTexture::setTexture(const string& source, const string& path,
                                 float bumpHeight, unsigned int flags)
{
    TextureManager* texMan = GetTextureManager();
    tex[lores] = texMan->getHandle(TextureInfo(source, path, bumpHeight, flags, lores));
    tex[medres] = texMan->getHandle(TextureInfo(source, path, bumpHeight, flags, medres));
    tex[hires] = texMan->getHandle(TextureInfo(source, path, bumpHeight, flags, hires));
}


Texture* MultiResTexture::find(unsigned int resolution)
{
    TextureManager* texMan = GetTextureManager();

    Texture* res = texMan->find(tex[resolution]);
    if (res != NULL)
        return res;

    // Preferred resolution isn't available; try the second choice
    // Set these to some defaults to avoid GCC complaints
    // about possible uninitialized variable usage:
    unsigned int secondChoice   = medres;
    unsigned int lastResort     = hires;
    switch (resolution)
    {
    case lores:
        secondChoice = medres;
        lastResort = hires;
        break;
    case medres:
        secondChoice = lores;
        lastResort = hires;
        break;
    case hires:
        secondChoice = medres;
        lastResort = lores;
        break;
    }

    tex[resolution] = tex[secondChoice];
    res = texMan->find(tex[resolution]);
    if (res != NULL)
        return res;

    tex[resolution] = tex[lastResort];

    return texMan->find(tex[resolution]);
}


bool MultiResTexture::isValid() const
{
    return (tex[lores] != InvalidResource ||
            tex[medres] != InvalidResource ||
            tex[hires] != InvalidResource);
}
