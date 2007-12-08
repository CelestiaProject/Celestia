// multirestexture.h
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


class MultiResTexture
{
 public:
    MultiResTexture();
    MultiResTexture(ResourceHandle loTex,
                    ResourceHandle medTex = InvalidResource,
                    ResourceHandle hiTex = InvalidResource);
    MultiResTexture(const std::string& source, const std::string& path);
    ~MultiResTexture() {};
    void setTexture(const std::string& source,
                    const std::string& path,
                    unsigned int flags = 0);
    void setTexture(const std::string& source,
                    const std::string& path,
                    float bumpHeight,
                    unsigned int flags);
    Texture* find(unsigned int resolution);

    bool isValid() const;

 public:
    ResourceHandle tex[3];
};

#endif // _CELENGINE_MULTITEXTURE_H_
