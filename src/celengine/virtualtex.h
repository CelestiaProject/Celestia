// virtualtex.h
//
// Copyright (C) 2003, Chris Laurel
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_VIRTUALTEX_H_
#define _CELENGINE_VIRTUALTEX_H_

#include <string>
#include <celengine/texture.h>


class VirtualTexture : public Texture
{
 public:
    VirtualTexture(const std::string& _tilePath,
                   unsigned int _baseSplit,
                   unsigned int _tileSize);
    ~VirtualTexture();

    virtual const TextureTile getTile(int lod, int u, int v);
    virtual void bind();

 private:
    std::string tilePath;
    unsigned int baseSplit;
    unsigned int tileSize;
};


VirtualTexture* LoadVirtualTexture(const std::string& filename);

#endif // _CELENGINE_VIRTUALTEX_H_
