// texmanager.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _TEXMANAGER_H_
#define _TEXMANAGER_H_

#include <string>
#include <map>
#include "resmanager.h"
#include "texture.h"


class TextureInfo : public ResourceInfo<Texture>
{
 public:
    std::string source;
    float bumpHeight;
    bool compressed;

    TextureInfo(const std::string _source, bool _compressed = false) :
        source(_source), bumpHeight(0.0f), compressed(_compressed) {};
    TextureInfo(const std::string _source, float _bumpHeight) :
        source(_source), bumpHeight(_bumpHeight), compressed(false) {};
    virtual Texture* load(const std::string&);
};

inline bool operator<(const TextureInfo& ti0, const TextureInfo& ti1)
{
    return ti0.source < ti1.source;
}

typedef ResourceManager<TextureInfo> TextureManager;

extern TextureManager* GetTextureManager();

#endif // _TEXMANAGER_H_

