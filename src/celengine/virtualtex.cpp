// virtualtex.cpp
//
// Copyright (C) 2003, Chris Laurel
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include <fstream>
#include <cmath>
#include <cassert>
#include "celutil/debug.h"
#include "virtualtex.h"
#include "gl.h"
#include "parser.h"

using namespace std;


static bool isPow2(int x)
{
    return ((x & (x - 1)) == 0);
}


VirtualTexture::VirtualTexture(const string& _tilePath,
                               unsigned int _baseSplit,
                               unsigned int _tileSize) :
    Texture(0, 0),
    tilePath(_tilePath),
    baseSplit(_baseSplit),
    tileSize(_tileSize)
{
    assert(baseSplit > 0);
    assert(tileSize != 0 && isPow2(tileSize));
}


VirtualTexture::~VirtualTexture()
{
}


const TextureTile VirtualTexture::getTile(int lod, int u, int v)
{
    if (lod == 0)
    {
        return TextureTile(0);
    }
    else
    {
        return TextureTile(0);
    }
}


void VirtualTexture::bind()
{
    // Treating a virtual texture like an ordinary one will not work; it's a
    // weakness in the class hierarchy.
}


static VirtualTexture* CreateVirtualTexture(Hash* texParams,
                                            const string& path)
{
    string imageDirectory;
    if (!texParams->getString("ImageDirectory", imageDirectory))
    {
        DPRINTF(0, "ImageDirectory missing in virtual texture.\n");
        return NULL;
    }

    double baseSplit = 0.0;
    if (!texParams->getNumber("BaseSplit", baseSplit) ||
        baseSplit < 1.0 || baseSplit != floor(baseSplit))
    {
        DPRINTF(0, "BaseSplit in virtual texture missing or has bad value\n");
        return NULL;
    }

    double tileSize = 0.0;
    if (!texParams->getNumber("TileSize", tileSize))
    {
        DPRINTF(0, "TileSize is missing from virtual texture\n");
        return NULL;
    }

    if (tileSize != floor(tileSize) ||
        tileSize < 64.0 ||
        !isPow2((int) tileSize))
    {
        DPRINTF(0, "Virtual texture tile size must be a power of two >= 64\n");
        return NULL;
    }

    return new VirtualTexture(path + "/" + imageDirectory,
                              (unsigned int) baseSplit,
                              (unsigned int) tileSize);
}


static VirtualTexture* LoadVirtualTexture(istream& in, const string& path)
{
    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);

    if (tokenizer.nextToken() != Tokenizer::TokenName)
        return NULL;

    string virtTexString = tokenizer.getNameValue();
    if (virtTexString != "VirtualTexture")
        return NULL;

    Value* texParamsValue = parser.readValue();
    if (texParamsValue == NULL || texParamsValue->getType() != Value::HashType)
    {
        DPRINTF(0, "Error parsing virtual texture\n");
        return NULL;
    }

    Hash* texParams = texParamsValue->getHash();
    
    return CreateVirtualTexture(texParams, path);
}


VirtualTexture* LoadVirtualTexture(const string& filename)
{
    ifstream in(filename.c_str(), ios::in);
    
    if (!in.good())
    {
        DPRINTF(0, "Error opening virtual texture file: %s\n", filename);
        return NULL;
    }

    // Strip off every character including and after the final slash to get
    // the pathname.
    string path = ".";
    string::size_type pos = filename.rfind('/');
    if (pos != string::npos)
        path = string(filename, 0, pos);

    return LoadVirtualTexture(in, path);
}
