// texture.h
//
// Copyright (C) 2001, Chris Laurel
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include <string>

using namespace std;

typedef void (*ProceduralTexEval)(float, float, float, unsigned char*);

class CTexture
{
 public:
    CTexture(int w, int h, int fmt);
    ~CTexture();

    void bindName();
    unsigned int getName();

    enum {
        ColorChannel = 1,
        AlphaChannel = 2
    };

 public:
    int width;
    int height;
    int components;
    int format;
    unsigned char* pixels;

    int cmapEntries;
    int cmapFormat;
    unsigned char* cmap;

    unsigned int glName;
};


extern CTexture* CreateProceduralTexture(int width, int height,
                                         int format,
                                         ProceduralTexEval func);
extern CTexture* CreateJPEGTexture(const char* filename,
                                   int channels = CTexture::ColorChannel);
extern CTexture* CreateBMPTexture(const char* filename);

extern CTexture* LoadTextureFromFile(string filename);


#endif // _TEXTURE_H_
