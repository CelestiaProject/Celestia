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
#include <celutil/basictypes.h>


typedef void (*ProceduralTexEval)(float, float, float, unsigned char*);

class TexelFunctionObject
{
 public:
    TexelFunctionObject() {};
    virtual ~TexelFunctionObject() {};
    virtual void operator()(float u, float v, float w,
                            unsigned char* pixel) = 0;
};


class Texture
{
 public:
    Texture(int w, int h, int fmt, bool _cubeMap = false);
    ~Texture();

    enum {
        WrapTexture      = 0x1,
        CompressTexture  = 0x2,
        NoMipMaps        = 0x4,
        AutoMipMaps      = 0x8,
        AllowSplitting   = 0x10,
    };

    void bindName(uint32 flags = 0);
    unsigned int getName();
    unsigned int getName(int, int);
    void normalMap(float scale, bool wrap);
    void bind() const;
    void bind(int, int) const;

    int getWidth() const;
    int getHeight() const;
    int getComponents() const;
    bool hasAlpha() const;

    int getUSubtextures() const;
    int getVSubtextures() const;

    void setMaxMipMapLevel(int);

    enum {
        ColorChannel = 1,
        AlphaChannel = 2
    };

 private:
    void loadCompressedTexture(unsigned char*, int, int);

 public:
    int width;
    int height;
    int components;
    int format;
    int maxMipMapLevel;
    bool cubeMap;
    bool isNormalMap;
    unsigned char* pixels;

    int cmapEntries;
    int cmapFormat;
    unsigned char* cmap;

    int uSplit;
    int vSplit;
    unsigned int* glNames;
};


extern Texture* CreateProceduralTexture(int width, int height,
                                        int format,
                                        ProceduralTexEval func);
extern Texture* CreateProceduralTexture(int width, int height,
                                        int format,
                                        TexelFunctionObject& func);
extern Texture* CreateProceduralCubeMap(int size, int format,
                                        ProceduralTexEval func);
extern Texture* CreateJPEGTexture(const char* filename,
                                  int channels = Texture::ColorChannel);
extern Texture* CreateBMPTexture(const char* filename);
extern Texture* CreatePNGTexture(const std::string& filename);
extern Texture* CreateDDSTexture(const std::string& filename);

extern Texture* LoadTextureFromFile(const std::string& filename);

extern Texture* CreateNormalizationCubeMap(int size);
extern Texture* CreateDiffuseLightCubeMap(int size);

#endif // _TEXTURE_H_
