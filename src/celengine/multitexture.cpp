// multitexture.cpp
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


void MultiTexture::setTexture(const string &source, bool compress)
{
    TextureManager* texMan = GetTextureManager();
    tex[lores]=texMan->getHandle(TextureInfo(source, compress, lores));
    tex[medres]=texMan->getHandle(TextureInfo(source, compress, medres));
    tex[hires]=texMan->getHandle(TextureInfo(source, compress, hires));
}


void MultiTexture::setTexture(const string &source, float height)
{
    TextureManager* texMan = GetTextureManager();
    tex[lores]=texMan->getHandle(TextureInfo(source, height, lores));
    tex[medres]=texMan->getHandle(TextureInfo(source, height, medres));
    tex[hires]=texMan->getHandle(TextureInfo(source, height, hires));
}

Texture* MultiTexture::find(unsigned int resolution)
{
    TextureManager* texMan = GetTextureManager();
    Texture *res=texMan->find(tex[resolution]);
    if(res||resolution==lores)
        return res;
    tex[resolution]=tex[resolution-1];
    res=texMan->find(tex[resolution]);
    if(res||resolution==medres)
        return res;
    tex[hires]=tex[medres]=tex[lores];
    return texMan->find(tex[lores]);
}
