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


class TextureManager : public ResourceManager
{
 public:
    TextureManager() : ResourceManager() {};
    TextureManager(std::string _baseDir) : ResourceManager(_baseDir) {};
    TextureManager(char* _baseDir) : ResourceManager(_baseDir) {};
    ~TextureManager();

    bool find(const std::string& name, CTexture**);
    CTexture* load(const std::string& name);
    CTexture* loadBumpMap(const std::string& name);
};

#endif // _TEXMANAGER_H_

