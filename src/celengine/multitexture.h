// multitexture.h
//
// Copyright (C) 2002, Deon Ramsey
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_MULTITEXTURE_H_
#define _CELENGINE_MULTITEXTURE_H_

#include <string>
#include "texture.h"
#include <celutil/reshandle.h>

#define TEXTURE_RESOLUTION 3

enum {
    lores  = 0,
    medres = 1,
    hires  = 2
};


class MultiTexture
{
 public:
    MultiTexture() {tex[lores]=tex[medres]=tex[hires]=InvalidResource;};
    MultiTexture(ResourceHandle loTexture, ResourceHandle medTexture=InvalidResource, ResourceHandle hiTexture=InvalidResource) {tex[lores]=loTexture; tex[medres]=medTexture; tex[hires]=hiTexture;};
    MultiTexture(string source) {setTexture(source);};
    ~MultiTexture() {};
    void setTexture(const string &source, bool compressed = false);
    void setTexture(const string &source, float height);
    Texture* find(unsigned int resolution);

 public:
    ResourceHandle tex[3];
};

#endif // _CELENGINE_MULTITEXTURE_H_
