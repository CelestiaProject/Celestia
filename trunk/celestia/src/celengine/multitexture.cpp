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


MultiResTexture::MultiResTexture(const std::string& source)
{
    setTexture(source);
}


void MultiResTexture::setTexture(const string& source, unsigned int flags)
{
    TextureManager* texMan = GetTextureManager();
    tex[lores] = texMan->getHandle(TextureInfo(source, flags, lores));
    tex[medres] = texMan->getHandle(TextureInfo(source, flags, medres));
    tex[hires] = texMan->getHandle(TextureInfo(source, flags, hires));
}


void MultiResTexture::setTexture(const string& source, float height, unsigned int flags)
{
    TextureManager* texMan = GetTextureManager();
    tex[lores] = texMan->getHandle(TextureInfo(source, height, flags, lores));
    tex[medres] = texMan->getHandle(TextureInfo(source, height, flags, medres));
    tex[hires] = texMan->getHandle(TextureInfo(source, height, flags, hires));
}


Texture* MultiResTexture::find(unsigned int resolution)
{
    TextureManager* texMan = GetTextureManager();
    Texture* res = texMan->find(tex[resolution]);
    if (res != NULL || resolution == lores)
        return res;
    tex[resolution] = tex[resolution - 1];
    res = texMan->find(tex[resolution]);
    if(res != NULL || resolution == medres)
        return res;
    tex[hires] = tex[medres] = tex[lores];

    return texMan->find(tex[lores]);
}
