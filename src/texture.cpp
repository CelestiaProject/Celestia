// texture.cpp
//
// Copyright (C) 2001, Chris Laurel
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#define JPEG_SUPPORT

#include <fstream>
#include "gl.h"
#ifdef JPEG_SUPPORT
#include "ijl.h"
#endif

#include "texture.h"

using namespace std;


typedef struct
{
    unsigned char b;
    unsigned char m;
    unsigned int size;
    unsigned int reserved;
    unsigned int offset;
} BMPFileHeader;

typedef struct
{
    unsigned int size;
    int width;
    int height;
    unsigned short planes;
    unsigned short bpp;
    unsigned int compression;
    unsigned int imageSize;
    int widthPPM;
    int heightPPM;
    unsigned int colorsUsed;
    unsigned int colorsImportant;
} BMPImageHeader;



CTexture::CTexture(int w, int h, int fmt) :
    width(w),
    height(h),
    format(fmt)
{
    cmap = NULL;
    cmapEntries = 0;

    switch (format)
    {
    case GL_RGB:
    case GL_BGR_EXT:
        components = 3;
        break;
    case GL_RGBA:
        components = 4;
        break;
    case GL_ALPHA:
        components = 1;
        break;
    case GL_LUMINANCE:
        components = 1;
        break;
    case GL_LUMINANCE_ALPHA:
        components = 2;
        break;
    default:
        break;
    }

    pixels = new unsigned char[width * height * components];

    glName = 0;
}


CTexture::~CTexture()
{
    if (pixels != NULL)
        delete[] pixels;
    if (cmap != NULL)
        delete[] cmap;
}


void CTexture::bindName()
{
    if (pixels == NULL)
        return;

    GLuint tn;

    glGenTextures(1, &tn);
    glBindTexture(GL_TEXTURE_2D, tn);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    gluBuild2DMipmaps(GL_TEXTURE_2D,
                      components,
                      width, height,
                      format,
                      GL_UNSIGNED_BYTE,
                      pixels);
    
    glName = tn;

    delete pixels;
}


unsigned int CTexture::getName()
{
    return glName;
}


CTexture* CreateProceduralTexture(int width, int height,
                                  int format,
                                  ProceduralTexEval func)
{
    CTexture* tex = new CTexture(width, height, format);
    if (tex == NULL)
        return NULL;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            float u = (float) x / (float) width * 2 - 1;
            float v = (float) y / (float) height * 2 - 1;
            func(u, v, 0, tex->pixels + (y * width + x) * tex->components);
        }
    }

    return tex;
}


CTexture* LoadTextureFromFile(string filename)
{
    int extPos = filename.length() - 3;
    if (extPos < 0)
        extPos = 0;
    string ext = string(filename, extPos, 3);

    if (ext.compare("JPG") == 0 || ext.compare("jpg") == 0)
        return CreateJPEGTexture(filename.c_str());
    else if (ext.compare("BMP") == 0 || ext.compare("bmp") == 0)
        return CreateBMPTexture(filename.c_str());
    else
        return NULL;
}


CTexture* CreateJPEGTexture(const char* filename,
                            int channels)
{
#ifndef JPEG_SUPPORT
    return NULL;
#else
    JPEG_CORE_PROPERTIES jpegProps;

    printf("Reading texture: %s\n", filename);

    // Must specify at least one of color or alpha
    if (channels == 0)
        return NULL;

    ZeroMemory(&jpegProps, sizeof(JPEG_CORE_PROPERTIES));
    if (ijlInit(&jpegProps) != IJL_OK)
        return NULL;

    jpegProps.JPGFile = (char*) filename;
    if (ijlRead(&jpegProps, IJL_JFILE_READPARAMS) != IJL_OK)
    {
        ijlFree(&jpegProps);
        return NULL;
    }

    // Set up the JPG color space, guessing based on the number of
    // color channels.
    switch (jpegProps.JPGChannels)
    {
    case 1:
        jpegProps.JPGColor = IJL_G;
        break;
    case 3:
        jpegProps.JPGColor = IJL_YCBCR;
        break;
    default:
        jpegProps.JPGColor = (IJL_COLOR) IJL_OTHER;
        break;
    }

    // Set up the target color space
    int format;
    if (jpegProps.JPGColor == IJL_YCBCR)
    {
        if ((channels & CTexture::AlphaChannel) != 0)
            format = GL_RGBA;
        else
            format = GL_RGB;
        jpegProps.DIBChannels = 3;
        jpegProps.DIBColor = IJL_RGB;
    }
    else if (jpegProps.JPGColor == IJL_G)
    {
        if ((channels & CTexture::AlphaChannel) != 0)
            format = GL_LUMINANCE_ALPHA;
        else
            format = GL_LUMINANCE;
        jpegProps.DIBChannels = 1;
        jpegProps.DIBColor = IJL_G;
    }
    else
    {
        ijlFree(&jpegProps);
        return NULL;
    }

    // Create the texture
    CTexture* tex = new CTexture(jpegProps.JPGWidth, jpegProps.JPGHeight,
                                 format);
    if (tex == NULL)
    {
        ijlFree(&jpegProps);
        return NULL;
    }

    jpegProps.DIBBytes = tex->pixels;
    jpegProps.DIBWidth = tex->width;
    jpegProps.DIBHeight = tex->height;

    // Slurp the body of the image
    if (ijlRead(&jpegProps, IJL_JFILE_READWHOLEIMAGE) != IJL_OK)
    {
        printf("Failed to read texture\n");
        ijlFree(&jpegProps);
        delete tex;
        return NULL;
    }

    ijlFree(&jpegProps);

    // If necessary, synthesize an alpha channel from color information
    if ((channels & CTexture::AlphaChannel) != 0)
    {
        if (format == GL_LUMINANCE_ALPHA)
        {
            int nPixels = tex->width * tex->height;
            unsigned char *newPixels = new unsigned char[nPixels * 2];
            for (int i = 0; i < nPixels; i++)
            {
                newPixels[i * 2] = newPixels[i * 2 + 1] = tex->pixels[i];
            }
            delete[] tex->pixels;
            tex->pixels = newPixels;
        }
    }
    
    return tex;
#endif // JPEG_SUPPORT
}


static int readInt(ifstream& in)
{
    unsigned char b[4];
    in.read(reinterpret_cast<char*>(b), 4);
    return ((int) b[3] << 24) + ((int) b[2] << 16)
        + ((int) b[1] << 8) + (int) b[0];
}


static short readShort(ifstream& in)
{
    unsigned char b[2];
    in.read(reinterpret_cast<char*>(b), 2);
    return ((short) b[1] << 8) + (short) b[0];
}


static CTexture* CreateBMPTexture(ifstream& in)
{
    BMPFileHeader fileHeader;
    BMPImageHeader imageHeader;
    unsigned char* pixels;

    printf("*** CreateBMPTexture\n");
    in >> fileHeader.b;
    in >> fileHeader.m;
    fileHeader.size = readInt(in);
    fileHeader.reserved = readInt(in);
    fileHeader.offset = readInt(in);

    printf("Checking header . . .\n");
    if (fileHeader.b != 'B' || fileHeader.m != 'M')
        return NULL;
    printf("Header is correct.\n");

    printf("File size: %d\n", fileHeader.size);
    printf("Bytes read: %d\n", in.tellg());

    imageHeader.size = readInt(in);
    imageHeader.width = readInt(in);
    imageHeader.height = readInt(in);
    imageHeader.planes = readShort(in);
    imageHeader.bpp = readShort(in);
    imageHeader.compression = readInt(in);
    imageHeader.imageSize = readInt(in);
    imageHeader.widthPPM = readInt(in);
    imageHeader.heightPPM = readInt(in);
    imageHeader.colorsUsed = readInt(in);
    imageHeader.colorsImportant = readInt(in);

    printf("%d Planes @ %d BPP\n", imageHeader.planes, imageHeader.bpp);
    printf("Size: %d\n", imageHeader.size);
    printf("Dimensions: %d x %d\n", imageHeader.width, imageHeader.height);

    if (imageHeader.width <= 0 || imageHeader.height <= 0)
        return NULL;

    // We currently don't support compressed BMPs
    if (imageHeader.compression != 0)
        return NULL;
    // We don't handle 1-, 2-, or 4-bpp images
    if (imageHeader.bpp != 8 && imageHeader.bpp != 24 && imageHeader.bpp != 32)
        return NULL;

    printf("Image size: %d\n", imageHeader.imageSize);
    printf("Compression: %d\n", imageHeader.compression);
    printf("WidthPPM x HeightPPM: %d x %d\n", imageHeader.widthPPM, imageHeader.heightPPM);

    unsigned char* palette = NULL;
    if (imageHeader.bpp == 8)
    {
        printf("Reading %d color palette\n", imageHeader.colorsUsed);
        palette = new unsigned char[imageHeader.colorsUsed * 4];
        in.read(reinterpret_cast<char*>(palette), imageHeader.colorsUsed * 4);
    }

    in.seekg(fileHeader.offset, ios_base::beg);

    unsigned int bytesPerRow =
        (imageHeader.width * imageHeader.bpp / 8 + 1) & ~1;
    unsigned int imageBytes = bytesPerRow * imageHeader.height;

    // slurp the image data
    pixels = new unsigned char[imageBytes];
    in.read(reinterpret_cast<char*>(pixels), imageBytes);

    // check for truncated file

    CTexture* tex = new CTexture(imageHeader.width, imageHeader.height,
                                 GL_RGB);
    if (tex == NULL)
    {
        delete[] pixels;
        return NULL;
    }

    // copy the image into the texture and perform any necessary conversions
    for (int y = 0; y < imageHeader.height; y++)
    {
        unsigned char* src = &pixels[y * bytesPerRow];
        unsigned char* dst = &tex->pixels[y * tex->width * 3];

        switch (imageHeader.bpp)
        {
        case 8:
            {
                for (int x = 0; x < imageHeader.width; x++)
                {
                    unsigned char* color = palette + (*src << 2);
                    dst[0] = color[2];
                    dst[1] = color[1];
                    dst[2] = color[0];
                    src++;
                    dst += 3;
                }
            }
            break;
            
        case 24:
            {
                for (int x = 0; x < imageHeader.width; x++)
                {
                    dst[0] = src[2];
                    dst[1] = src[1];
                    dst[2] = src[0];
                    src += 3;
                    dst += 3;
                }
            }
            break;

        case 32:
            {
                for (int x = 0; x < imageHeader.width; x++)
                {
                    dst[0] = src[2];
                    dst[1] = src[1];
                    dst[2] = src[0];
                    src += 4;
                    dst += 3;
                }
            }
            break;
        }
    }

    delete[] pixels;

    return tex;
}


CTexture* CreateBMPTexture(const char* filename)
{
    ifstream bmpFile(filename, ios::in | ios::binary);

    if (bmpFile.good())
    {
        CTexture* tex = CreateBMPTexture(bmpFile);
        bmpFile.close();
        return tex;
    }
    else
    {
        return NULL;
    }
}

