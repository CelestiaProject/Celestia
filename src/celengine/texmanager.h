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
#include <celutil/resmanager.h>
#include <celengine/texture.h>
#include "multitexture.h"


class TextureInfo : public ResourceInfo<Texture>
{
 public:
    std::string source;
    float bumpHeight;
    bool compressed;
    unsigned int resolution;

    TextureInfo(const std::string _source, bool _compressed = false, unsigned int _resolution=medres) :
        source(_source), bumpHeight(0.0f), compressed(_compressed), resolution(_resolution) {};
    TextureInfo(const std::string _source, float _bumpHeight, unsigned int _resolution=medres) :
        source(_source), bumpHeight(_bumpHeight), compressed(false), resolution(_resolution) {};
    virtual Texture* load(const std::string&);
};

inline bool operator<(const TextureInfo& ti0, const TextureInfo& ti1)
{
    if(ti0.resolution == ti1.resolution)
        return ti0.source < ti1.source;
    return ti0.resolution < ti1.resolution;
}

typedef ResourceManager<TextureInfo> TextureManager;

extern TextureManager* GetTextureManager();

#endif // _TEXMANAGER_H_

