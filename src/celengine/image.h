// image.h
//
// Copyright (C) 2001, Chris Laurel
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_IMAGE_H_
#define _CELENGINE_IMAGE_H_

#include <string>
#include <celutil/basictypes.h>

// The image class supports multiple GL formats, including compressed ones.
// Mipmaps may be stored within an image as well.  The mipmaps are stored in
// one contiguous block of memory (i.e. there's not an instance of Image per
// mipmap.)  Mip levels are addressed such that zero is the base (largest) mip
// level.

class Image
{
 public:
    Image(int fmt, int w, int h, int mips = 1);
    ~Image();

    int getWidth() const;
    int getHeight() const;
    int getPitch() const;
    int getMipLevelCount() const;
    int getFormat() const;
    int getComponents() const;
    unsigned char* getPixels();
    unsigned char* getPixelRow(int row);
    unsigned char* getPixelRow(int mip, int row);
    unsigned char* getMipLevel(int mip);
    int getSize() const;
    int getMipLevelSize(int mip) const;

    bool isCompressed() const;
    bool hasAlpha() const;

    Image* computeNormalMap(float scale, bool wrap) const;

    enum {
        ColorChannel = 1,
        AlphaChannel = 2
    };

 private:
    int width;
    int height;
    int pitch;
    int mipLevels;
    int components;
    int format;
    int size;
    unsigned char* pixels;
};

extern Image* LoadJPEGImage(const std::string& filename,
                            int channels = Image::ColorChannel);
extern Image* LoadBMPImage(const std::string& filename);
extern Image* LoadPNGImage(const std::string& filename);
extern Image* LoadDDSImage(const std::string& filename);

extern Image* LoadImageFromFile(const std::string& filename);

#endif // _CELENGINE_IMAGE_H_
