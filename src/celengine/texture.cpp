// texture.cpp
//
// Copyright (C) 2001, Chris Laurel
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef MACOSX
#define IJG_JPEG_SUPPORT
#define PNG_SUPPORT
#endif

#ifdef MACOSX
#include <unistd.h>
#include "CGBuffer.h"
#ifndef PNG_SUPPORT
#include <Quicktime/ImageCompression.h>
#include <QuickTime/QuickTimeComponents.h>
#endif
#endif

#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <cassert>

#ifndef _WIN32
#ifndef MACOSX
#include <config.h>
#endif /* ! MACOSX */
#endif /* ! _WIN32 */

#include <celmath/vecmath.h>
#include <celutil/filetype.h>
#include <celutil/debug.h>

#include "gl.h"
#include "glext.h"
#include "celestia.h"

// OpenGL 1.2 stuff missing from Windows headers . . . probably should be
// moved into glext.h
#ifndef GL_TEXTURE_MAX_LEVEL
#define GL_TEXTURE_MAX_LEVEL 0x813D
#endif

#ifdef IJG_JPEG_SUPPORT

#ifndef PNG_SUPPORT
#include "setjmp.h"
#endif // PNG_SUPPORT

extern "C" {
#ifdef _WIN32
#include "jpeglib.h"
#else
#include <jpeglib.h>
#endif
}

#endif // IJG_JPEG_SUPPORT

#ifdef PNG_SUPPORT // PNG_SUPPORT
#ifdef MACOSX
#include "../../macosx/png.h"
#else
#include "png.h"
#endif // MACOSX

// Define png_jmpbuf() in case we are using a pre-1.0.6 version of libpng
#ifndef png_jmpbuf
#define png_jmpbuf(png_ptr) png_ptr->jmpbuf
#endif // PNG_SUPPORT

// Define various expansion transformations for old versions of libpng
#if PNG_LIBPNG_VER < 10004
#define png_set_palette_to_rgb(p)  png_set_expand(p)
#define png_set_gray_1_2_4_to_8(p) png_set_expand(p)
#define png_set_tRNS_to_alpha(p)   png_set_expand(p)
#endif // PNG_LIBPNG_VER < 10004

#endif // PNG_SUPPORT

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

static bool initialized = false;
static bool compressionSupported = false;
static bool clampToEdgeSupported = false;
static bool clampToBorderSupported = false;
static bool autoMipMapSupported = false;
static bool maxLevelSupported = false;
static GLint maxTextureSize = 0;


static bool testMaxLevel()
{
    unsigned char texels[64];

    glEnable(GL_TEXTURE_2D);
    // Test whether GL_TEXTURE_MAX_LEVEL is supported . . .
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_LUMINANCE,
                 8, 8,
                 0,
                 GL_LUMINANCE,
                 GL_UNSIGNED_BYTE,
                 texels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 2);
    float maxLev = -1.0f;
    glGetTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &maxLev);
    glDisable(GL_TEXTURE_2D);

    return maxLev == 2;
}


static void initTextureLoader()
{
    compressionSupported = ExtensionSupported("GL_ARB_texture_compression");
    if (compressionSupported)
        InitExtTextureCompression();

#ifdef GL_VERSION_1_2
    clampToEdgeSupported = true;
#else
    clampToEdgeSupported = ExtensionSupported("GL_EXT_texture_edge_clamp");
#endif // GL_VERSION_1_2
    clampToBorderSupported = ExtensionSupported("GL_ARB_texture_border_clamp");
    autoMipMapSupported = ExtensionSupported("GL_SGIS_generate_mipmap");
    maxLevelSupported = testMaxLevel();
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

    initialized = true;
}


Texture::Texture(int w, int h, int fmt, bool _cubeMap) :
    width(w),
    height(h),
    format(fmt),
    maxMipMapLevel(-1),
    cubeMap(_cubeMap),
    pixels(NULL),
    glNames(NULL)
{
    cmap = NULL;
    cmapEntries = 0;

    assert(!cubeMap || height == width);

    // Yuck . . .
    if (!initialized)
        initTextureLoader();

    bool compressedFormat = false;

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
    case GL_DSDT_NV:
        components = 2;
        break;
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        components = 3;
        compressedFormat = true;
        break;
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        components = 4;
        compressedFormat = true;
        break;
    default:
        break;
    }

    int faces = cubeMap ? 6 : 1;

    // Determine the amount of texture splitting required.
    uSplit = width / maxTextureSize;
    vSplit = height / maxTextureSize;
    if (uSplit < 1)
        uSplit = 1;
    if (vSplit < 1)
        vSplit = 1;

    glNames = new unsigned int[uSplit * vSplit];
    for (int i = 0; i < uSplit * vSplit; i++)
        glNames[i] = 0;

    if (!compressedFormat)
        pixels = new unsigned char[width * height * components * faces];
}


Texture::~Texture()
{
    if (pixels != NULL)
        delete[] pixels;
    if (cmap != NULL)
        delete[] cmap;
    if (glNames != NULL)
    {
        for (int i = 0; i < uSplit * vSplit; i++)
        {
            if (glNames[i] != 0)
                glDeleteTextures(1, (const GLuint*) &glNames[i]);
        }
        delete[] glNames;
    }
}


static int log2(unsigned int x)
{
    int n = -1;

    while (x != 0)
    {
        x >>= 1;
        n++;
    }

    return n;
}


static int compressedBlockSize(int format)
{
    if (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)
        return 8;
    else
        return 16;
}


// static int compressedTextureAddress(int );
static int compressedTextureSize(int w, int h, int mipMapLevels,
                                 int format)
{

    int totalSize = 0;
    for (int i = 0; i <= mipMapLevels; i++)
    {
        // Compute the size in bytes of this mip map level.
        // For S3TC, each block is 4x4 pixels.
        totalSize += ((w + 3) / 4) * ((h + 3) / 4);

        w >>= 1;
        if (w < 1)
            w = 1;
        h >>= 1;
        if (h < 1)
            h = 1;
    }

    return totalSize * compressedBlockSize(format);
}


void Texture::bindName(uint32 flags)
{
    bool wrap = ((flags & WrapTexture) != 0);
    bool compress = ((flags & CompressTexture) != 0) && compressionSupported;
    bool mipmap = ((flags & NoMipMaps) == 0);
    bool automipmap = ((flags & AutoMipMaps) != 0) && autoMipMapSupported && mipmap;
    if (maxMipMapLevel == 0 || (maxMipMapLevel > 0 && !maxLevelSupported))
        mipmap = false;

    if (pixels == NULL)
        return;
    GLenum textureType = GL_TEXTURE_2D;

    if ((flags & AllowSplitting) == 0)
    {
        uSplit = 1;
        vSplit = 1;
    }

    // Disable wrapping if the texture is split
    if (uSplit > 1 || vSplit > 1)
        wrap = false;

    // If we're not wrapping, use GL_CLAMP_TO_EDGE if it's available; we want
    // to ignore the border color.
    GLenum wrapMode = GL_REPEAT;
    if (!wrap)
    {
        if ((flags & BorderClamp) != 0 && clampToBorderSupported)
        {
            wrapMode = GL_CLAMP_TO_BORDER_ARB;
        }
        else
        {
            wrapMode = clampToEdgeSupported ? GL_CLAMP_TO_EDGE : GL_CLAMP;
        }
    }

    if (cubeMap)
    {
        textureType = GL_TEXTURE_CUBE_MAP_EXT;
        wrapMode = clampToEdgeSupported ? GL_CLAMP_TO_EDGE : GL_CLAMP;
    }

    bool compressedFormat = false;
    int internalFormat = 0;
    if (compress)
    {
        switch (format)
        {
        case GL_RGB:
        case GL_BGR_EXT:
            internalFormat = GL_COMPRESSED_RGB_ARB;
            break;
        case GL_RGBA:
            internalFormat = GL_COMPRESSED_RGBA_ARB;
            break;
        case GL_ALPHA:
            internalFormat = GL_COMPRESSED_ALPHA_ARB;
            break;
        case GL_LUMINANCE:
            internalFormat = GL_COMPRESSED_LUMINANCE_ARB;
            break;
        case GL_LUMINANCE_ALPHA:
            internalFormat = GL_COMPRESSED_LUMINANCE_ALPHA_ARB;
            break;
        case GL_INTENSITY:
            internalFormat = GL_COMPRESSED_INTENSITY_ARB;
            break;
        }
	glHint((GLenum) GL_TEXTURE_COMPRESSION_HINT_ARB, GL_NICEST);
    }
    else
    {
        switch (format)
        {
        case GL_DSDT_NV:
            internalFormat = format;
            break;
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            compressedFormat = true;
            internalFormat = format;
            break;
        default:
            internalFormat = components;
            break;
        }
    }

    int nFaces = 1;
    unsigned int textureTarget = GL_TEXTURE_2D;
    if (cubeMap)
    {
        nFaces = 6;
        textureTarget = (unsigned int) GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT;
    }

    bool isSplit = (uSplit > 1 || vSplit > 1);
    int subtexWidth = width / uSplit;
    int subtexHeight = height / vSplit;

    // Determine how many mip map levels are required for a compressed
    // subtexture.
    int compMipMaps = maxMipMapLevel;
    int compSubtexSize = 1;
    if (compressedFormat)
    {
        int log2Dim = log2((unsigned int) ((subtexWidth > subtexHeight) ? subtexWidth : subtexHeight));
        if (compMipMaps < 0 || compMipMaps >= log2Dim)
            compMipMaps = log2Dim;
        compSubtexSize = compressedTextureSize(subtexWidth, subtexHeight,
                                               compMipMaps, format);
    }

    unsigned char* subtexPixelBuffer = NULL;
    unsigned char* pixelSource = NULL;
    if (isSplit)
    {
        int subtexSize;
        if (compressedFormat)
            subtexSize = compSubtexSize;
        else
            subtexSize = subtexWidth * subtexHeight * components;
        subtexPixelBuffer = new unsigned char[subtexSize];
        pixelSource = subtexPixelBuffer;
    }
    else
    {
        pixelSource = pixels;
    }

    for (int i = 0; i < vSplit; i++)
    {
        for (int j = 0; j < uSplit; j++)
        {
            // If the texture is split, copy texels from the subtexture
            // area to the pixel buffer.  This is straightforward for normal
            // textures, but an immense headache for compressed textures with
            // prebuilt mipmaps.
            if (isSplit)
            {
                if (compressedFormat)
                {
                    int srcMipMapOffset = 0;
                    int destMipMapOffset = 0;
                    for (int mipMapLevel = 0; mipMapLevel < compMipMaps;
                         mipMapLevel++)
                    {
                        int mipWidth = max(width >> mipMapLevel, 1);
                        int mipHeight = max(height >> mipMapLevel, 1);
                        int subMipWidth = max(subtexWidth >> mipMapLevel, 1);
                        int subMipHeight = max(subtexHeight >> mipMapLevel, 1);
                        int xBlocks = max(subMipWidth / 4, 1);
                        int yBlocks = max(subMipHeight / 4, 1);
                        int blockSize = compressedBlockSize(format);
                        int destBytesPerRow = xBlocks * blockSize;
                        int srcBytesPerRow = max(mipWidth / 4, 1) * blockSize;
                        int srcX = j * subMipWidth / 4;
                        int srcY = i * subMipHeight / 4;
                        int subtexOffset = srcY * srcBytesPerRow +
                            srcX * blockSize;

                        for (int y = 0; y < yBlocks; y++)
                        {
                            memcpy(subtexPixelBuffer + destMipMapOffset + y * destBytesPerRow,
                                   pixels + srcMipMapOffset + subtexOffset + y * srcBytesPerRow,
                                   destBytesPerRow);
                        }

                        srcMipMapOffset += max(mipHeight / 4, 1) * srcBytesPerRow;
                        destMipMapOffset += yBlocks * destBytesPerRow;
                    }
                }
                else
                {
                    unsigned char* subtexPixels = pixels +
                        (i * subtexHeight * width + j * subtexWidth) * components;
                    for (int y = 0; y < subtexHeight; y++)
                    {
                        memcpy(subtexPixelBuffer + y * subtexWidth * components,
                               subtexPixels + y * width * components,
                               subtexWidth * components);
                    }
                }
            }

            GLuint tn;
            glGenTextures(1, &tn);
            glBindTexture(textureType, tn);
            glNames[i * uSplit + j] = tn;

            glTexParameteri(textureType, GL_TEXTURE_WRAP_S, wrapMode);
            glTexParameteri(textureType, GL_TEXTURE_WRAP_T, wrapMode);
            glTexParameteri(textureType, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(textureType, GL_TEXTURE_MIN_FILTER,
                            mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
            if (wrapMode == GL_CLAMP_TO_BORDER_ARB)
            {
                float bc[4] = { borderColor.red(), borderColor.green(),
                                borderColor.blue(), borderColor.alpha() };
                glTexParameterfv(textureType, GL_TEXTURE_BORDER_COLOR, bc);
            }
            
            if (automipmap)
                glTexParameteri(textureType, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);

            for (int face = 0; face < nFaces; face++)
            {
                if (compressedFormat)
                {
                    loadCompressedTexture(pixelSource,
                                          subtexWidth, subtexHeight);
                }
                else if (mipmap && !automipmap)
                {
                    if (maxMipMapLevel > 0)
                    {
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,
                                        maxMipMapLevel);
                    }
                    gluBuild2DMipmaps((GLenum) (textureTarget + face),
                                      internalFormat,
                                      subtexWidth, subtexHeight,
                                      (GLenum) format,
                                      GL_UNSIGNED_BYTE,
                                      pixelSource + face * width * height * components);
                }
                else
                {
                    glTexImage2D(GL_TEXTURE_2D,
                                 0,
                                 internalFormat,
                                 subtexWidth, subtexHeight,
                                 0,
                                 (GLenum) format,
                                 GL_UNSIGNED_BYTE,
                                 pixelSource + face * width * height * components);
                }
            }
        }
    }

    if (subtexPixelBuffer != NULL)
        delete[] subtexPixelBuffer;
    delete[] pixels;
    pixels = NULL;
}


void Texture::loadCompressedTexture(unsigned char* texels,
                                    int w, int h)
{
    // Determine how many mip map levels to use
    int numMipMaps = maxMipMapLevel;
    int largestDimension = (w > h) ? w : h;
    int log2Dim = log2((unsigned int) largestDimension);

    if (numMipMaps < 0 || numMipMaps >= log2Dim)
        numMipMaps = log2Dim;

    int mmOffset = 0;
    for (int mipMapLevel = 0; mipMapLevel <= numMipMaps;
         mipMapLevel++)
    {
        unsigned int mmWidth =
            (unsigned int) w >> mipMapLevel;
        unsigned int mmHeight =
            (unsigned int) h >> mipMapLevel;
        if (mmWidth < 1)
            mmWidth = 1;
        if (mmHeight < 1)
            mmHeight = 1;
        
        // Compute the size in bytes of this mip map level.
        // For S3TC, each block is 4x4 pixels.
        int mmSize = compressedBlockSize(format) *
            ((mmWidth + 3) / 4) * ((mmHeight + 3) / 4);
                        
        EXTglCompressedTexImage2DARB(GL_TEXTURE_2D,
                                     mipMapLevel,
                                     format,
                                     mmWidth, mmHeight,
                                     0,
                                     mmSize,
                                     texels + mmOffset);
        mmOffset += mmSize;
    }
}


unsigned int Texture::getName()
{
    assert(glNames != NULL);
    return glNames[0];
}

unsigned int Texture::getName(int u, int v)
{
    assert(glNames != NULL);
    assert(u < uSplit && v < vSplit);
    return glNames[v * uSplit + u];
}


void Texture::bind() const
{
    assert(glNames != NULL);
    glBindTexture(cubeMap ? GL_TEXTURE_CUBE_MAP_EXT : GL_TEXTURE_2D,
                  glNames[0]);
}

void Texture::bind(int u, int v) const
{
    assert(glNames != NULL);
    assert(u < uSplit && v < vSplit);
    glBindTexture(cubeMap ? GL_TEXTURE_CUBE_MAP_EXT : GL_TEXTURE_2D,
                  glNames[v * uSplit + u]);
}


int Texture::getWidth() const
{
    return width;
}

int Texture::getHeight() const
{
    return height;
}

int Texture::getComponents() const
{
    return components;
}

bool Texture::hasAlpha() const
{
    switch (format)
    {
    case GL_RGBA:
    case GL_ALPHA:
    case GL_LUMINANCE_ALPHA:
        return true;
    default:
        return false;
    }
}


int Texture::getUSubtextures() const
{
    return uSplit;
}

int Texture::getVSubtextures() const
{
    return vSplit;
}


void Texture::setMaxMipMapLevel(int level)
{
    maxMipMapLevel = level;
}


void Texture::setBorderColor(Color c)
{
    borderColor = c;
}


// Convert the texture to a normal map
void Texture::normalMap(float scale, bool wrap)
{
    // Make sure that we get the texture after it's been loaded with
    // data, but before bindName was called and texel data deleted.
    if (pixels == NULL)
    {
        DPRINTF(0, "Texture::normalMap: no texel data!\n");
        return;
    }

    unsigned char* npixels = new unsigned char[width * height * 4];

    // Compute normals using differences between adjacent texels.  Only
    // the value of the first channel is considered with computing
    // differences--this produces the expected results with greyscale
    // textures.
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            int i0 = i;
            int j0 = j;
            int i1 = i - 1;
            int j1 = j - 1;
            if (i1 < 0)
            {
                if (wrap)
                {
                    i1 = height - 1;
                }
                else
                {
                    i0++;
                    i1++;
                }   
            }
            if (j1 < 0)
            {
                if (wrap)
                {
                    j1 = width - 1;
                }
                else
                {
                    j0++;
                    j1++;
                }
            }

            int h00 = (int) pixels[(i0 * width + j0) * components];
            int h10 = (int) pixels[(i0 * width + j1) * components];
            int h01 = (int) pixels[(i1 * width + j0) * components];
            
            float dx = (float) (h00 - h10) * (1.0f / 255.0f) * scale;
            float dy = (float) (h00 - h01) * (1.0f / 255.0f) * scale;

            float mag = (float) sqrt(dx * dx + dy * dy + 1.0f);
            float rmag = 1.0f / mag;

            int n = (i * width + j) * 4;
            npixels[n]     = (unsigned char) (128 + 127 * dx * rmag);
            npixels[n + 1] = (unsigned char) (128 - 127 * dy * rmag);
            // npixels[n]     = (unsigned char) (128 + 127 * dy * rmag);
            // npixels[n + 1] = (unsigned char) (128 - 127 * dx * rmag);
            // npixels[n]     = (unsigned char) (128 - 127 * dx * rmag);
            // npixels[n + 1] = (unsigned char) (128 + 127 * dy * rmag);
            npixels[n + 2] = (unsigned char) (128 + 127 * rmag);
            npixels[n + 3] = 255;
        }
    }

    delete[] pixels;
    pixels = npixels;

    format = GL_RGBA;
    components = 4;
    isNormalMap = true;
}


Texture* CreateProceduralTexture(int width, int height,
                                 int format,
                                 ProceduralTexEval func)
{
    Texture* tex = new Texture(width, height, format);
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


Texture* CreateProceduralTexture(int width, int height,
                                 int format,
                                 TexelFunctionObject& func)
{
    Texture* tex = new Texture(width, height, format);
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


static bool isPow2(int x)
{
    return ((x & (x - 1)) == 0);
}

Texture* LoadTextureFromFile(const string& filename)
{
    ContentType type = DetermineFileType(filename);
    Texture* tex = NULL;

    switch (type)
    {
    case Content_JPEG:
        tex = CreateJPEGTexture(filename.c_str());
        break;
    case Content_BMP:
        tex = CreateBMPTexture(filename.c_str());
        break;
    case Content_PNG:
        tex = CreatePNGTexture(filename);
        break;
    case Content_DDS:
        tex = CreateDDSTexture(filename);
        break;
    default:
        DPRINTF(0, "Unrecognized or unsupported image file type.\n");
        break;
    }

    if (tex != NULL)
    {
        // Require dimensions of textures to be powers of two.
        if (!isPow2(tex->width) || !isPow2(tex->height))
        {
            DPRINTF(0, "Texture %s has non-power of two dimensions.\n",
                    filename.c_str());
            delete tex;
            tex = NULL;
        }
    }

    return tex;
}



#ifdef IJG_JPEG_SUPPORT

struct my_error_mgr
{
    struct jpeg_error_mgr pub;	// "public" fields
    jmp_buf setjmp_buffer;      // for return to caller
};

typedef struct my_error_mgr *my_error_ptr;

METHODDEF(void) my_error_exit(j_common_ptr cinfo)
{
    // cinfo->err really points to a my_error_mgr struct, so coerce pointer
    my_error_ptr myerr = (my_error_ptr) cinfo->err;

    // Always display the message.
    // We could postpone this until after returning, if we chose.
    (*cinfo->err->output_message) (cinfo);

    // Return control to the setjmp point
    longjmp(myerr->setjmp_buffer, 1);
}

#endif // IJG_JPEG_SUPPORT



Texture* CreateJPEGTexture(const char* filename,
                            int channels)
{
#ifdef IJG_JPEG_SUPPORT
    Texture* tex = NULL;

    // This struct contains the JPEG decompression parameters and pointers to
    // working space (which is allocated as needed by the JPEG library).
    struct jpeg_decompress_struct cinfo;

    // We use our private extension JPEG error handler.
    // Note that this struct must live as long as the main JPEG parameter
    // struct, to avoid dangling-pointer problems.
    struct my_error_mgr jerr;
    // More stuff
    JSAMPARRAY buffer;		// Output row buffer
    int row_stride;		// physical row width in output buffer
    long cont;

    // In this example we want to open the input file before doing anything else,
    // so that the setjmp() error recovery below can assume the file is open.
    // VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
    // requires it in order to read binary files.

    FILE *in;
    in = fopen(filename, "rb");
    if (in == NULL)
        return NULL;

    // Step 1: allocate and initialize JPEG decompression object
    // We set up the normal JPEG error routines, then override error_exit.
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    // Establish the setjmp return context for my_error_exit to use.
    if (setjmp(jerr.setjmp_buffer))
    {
        // If we get here, the JPEG code has signaled an error.
        // We need to clean up the JPEG object, close the input file, and return.
        jpeg_destroy_decompress(&cinfo);
        fclose(in);
        if (tex != NULL)
            delete tex;

        return NULL;
    }

    // Now we can initialize the JPEG decompression object.
    jpeg_create_decompress(&cinfo);

    // Step 2: specify data source (eg, a file)
    jpeg_stdio_src(&cinfo, in);

    // Step 3: read file parameters with jpeg_read_header()
    (void) jpeg_read_header(&cinfo, TRUE);

    // We can ignore the return value from jpeg_read_header since
    //  (a) suspension is not possible with the stdio data source, and
    //  (b) we passed TRUE to reject a tables-only JPEG file as an error.

    // Step 4: set parameters for decompression

    // In this example, we don't need to change any of the defaults set by
    // jpeg_read_header(), so we do nothing here.

    // Step 5: Start decompressor

    (void) jpeg_start_decompress(&cinfo);
    // We can ignore the return value since suspension is not possible
    // with the stdio data source.

    // We may need to do some setup of our own at this point before reading
    // the data.  After jpeg_start_decompress() we have the correct scaled
    // output image dimensions available, as well as the output colormap
    // if we asked for color quantization.
    // In this example, we need to make an output work buffer of the right size.
    // JSAMPLEs per row in output buffer
    row_stride = cinfo.output_width * cinfo.output_components;
    // Make a one-row-high sample array that will go away when done with image
    buffer = (*cinfo.mem->alloc_sarray)
        ((j_common_ptr) & cinfo, JPOOL_IMAGE, row_stride, 1);

    // Step 6: while (scan lines remain to be read)
    //             jpeg_read_scanlines(...);

    // Here we use the library's state variable cinfo.output_scanline as the
    // loop counter, so that we don't have to keep track ourselves.

    int format = GL_RGB;
    if (cinfo.output_components == 1)
        format = GL_LUMINANCE;

    tex = new Texture(cinfo.image_width, cinfo.image_height, format);

    // cont = cinfo.output_height - 1;
    cont = 0;
    while (cinfo.output_scanline < cinfo.output_height)
    {
        // jpeg_read_scanlines expects an array of pointers to scanlines.
        // Here the array is only one element long, but you could ask for
        // more than one scanline at a time if that's more convenient.
        (void) jpeg_read_scanlines(&cinfo, buffer, 1);

        // Assume put_scanline_someplace wants a pointer and sample count.
        // put_scanline_someplace(buffer[0], row_stride);
        memcpy(tex->pixels +
               cinfo.image_width * cinfo.output_components * cont,
               buffer[0], row_stride);
        cont++;
    }

    // Step 7: Finish decompression

    (void) jpeg_finish_decompress(&cinfo);
    // We can ignore the return value since suspension is not possible
    // with the stdio data source.

    // Step 8: Release JPEG decompression object

    // This is an important step since it will release a good deal of memory.
    jpeg_destroy_decompress(&cinfo);

    // After finish_decompress, we can close the input file.
    // Here we postpone it until after no more JPEG errors are possible,
    // so as to simplify the setjmp error logic above.  (Actually, I don't
    // think that jpeg_destroy can do an error exit, but why assume anything...

    fclose(in);

    // At this point you may want to check to see whether any corrupt-data
    // warnings occurred (test whether jerr.pub.num_warnings is nonzero).

    return tex;
    
#elif MACOSX

    Texture* tex = NULL;
    CGBuffer* cgJpegImage;
    size_t img_w, img_h, img_d;

    cgJpegImage = new CGBuffer(filename);
    if (cgJpegImage == NULL) {
        char tempcwd[2048];
        getcwd(tempcwd, sizeof(tempcwd));
        DPRINTF(0, "CGBuffer :: Error opening JPEG texture file %s/%s\n", tempcwd, filename);
        delete cgJpegImage;
        return NULL;
    }

    if (!cgJpegImage->LoadJPEG()) {
        char tempcwd[2048];
        getcwd(tempcwd, sizeof(tempcwd));
        DPRINTF(0, "CGBuffer :: Error loading JPEG texture file %s/%s\n", tempcwd, filename);
        delete cgJpegImage;
        return NULL;
    }
    
    cgJpegImage->Render();

    img_w = (size_t)cgJpegImage->image_size.width;
    img_h = (size_t)cgJpegImage->image_size.height;
    img_d = (size_t)((cgJpegImage->image_depth == 8) ? 1 : 4);

    //DPRINTF(0,"cgJpegImage :: %d x %d x %d [%d] bpp\n", img_w, img_h, (size_t)cgJpegImage->image_depth, img_d);


#ifdef MACOSX_ALPHA_JPEGS
    int format = (img_d == 1) ? GL_LUMINANCE : GL_RGBA;
#else
    int format = (img_d == 1) ? GL_LUMINANCE : GL_RGB;
#endif
    tex = new Texture(img_w, img_h, format);
    if (tex == NULL || tex->pixels == NULL) {
        DPRINTF(0, "Could not create Texture\n");
        delete cgJpegImage;
        return NULL;
    }
#ifndef MACOSX_ALPHA_JPEGS
    if (img_d == 1) {
#endif /* !MACOSX_ALPHA_JPEGS */
        memcpy((void*)tex->pixels,(void*)cgJpegImage->buffer->data,(unsigned int)(img_w * img_h * img_d));
        delete cgJpegImage;
        return tex;
#ifndef MACOSX_ALPHA_JPEGS
    }
    // this could probably use a duffs, but it's probably not a bottleneck
    unsigned char* bout = (unsigned char*)tex->pixels;
    unsigned char* bin  = (unsigned char*)cgJpegImage->buffer->data;
    unsigned int bcount = img_w * img_h * img_d;
    unsigned int i = 0;
    for (i=0; i<bcount; ++i) {
        if ((i&3)^3) {
            *bout++ = *bin++;
        } else {
            ++bin;
        }
    }
    return tex;
#endif /* !MACOSX_ALPHA_JPEGS */
    
#else
    return NULL;
#endif // IJG_JPEG_SUPPORT
}


#ifdef PNG_SUPPORT
void PNGReadData(png_structp png_ptr, png_bytep data, png_size_t length)
{
    FILE* fp = (FILE*) png_get_io_ptr(png_ptr);
    fread((void*) data, 1, length, fp);
}
#endif

Texture* CreatePNGTexture(const string& filename)
{
#ifndef PNG_SUPPORT
    return NULL;
/*
#if MACOSX
    FSRef fsr;
    FSSpec fss;
    GraphicsImportComponent gi;
    ImageDescriptionHandle imageDescH = NULL;
    ImageDescription* desc = NULL;
    ComponentResult result;
    Texture* tex = NULL;
    size_t img_w, img_h, img_d;
    
    if (FSPathMakeRef(filename.c_str(), &fsr, false) != noErr) {
        DPRINTF(0,"CreatePNGTexture :: Could not FSPathMakeRef for %s\n",filename.c_str());
        return NULL;
    }
    
    if (FSGetCatalogInfo(&fsr, kFSCatInfoNone, NULL, NULL, &fss, NULL) != noErr) {
        DPRINTF(0,"CreatePNGTexture :: Could not FSRef -> FSSpec\n");
        return NULL;
    }
        
    if (GetGraphicsImporterForFile(&fss, &gi) != noErr) {
        DPRINTF(0,"CreatePNGTexture :: Could not GetGraphicsImporterForFile\n");
        return NULL;
    }
    
    result = GraphicsImportGetImageDescription(gi, &imageDescH);
    if( noErr != result || imageDescH == NULL ) {
            DPRINTF(0,"CreatePNGTexture :: Error while retrieving image description\n");
            return NULL;
    }
    
    desc = *imageDescH;
    img_w = (size_t)desc->width;
    img_h = (size_t)desc->height;
    img_d = (size_t)desc->depth;
    size_t data_size = img_w * img_h * (img_d >> 3);
    if( imageDescH != NULL)
            DisposeHandle((Handle)imageDescH);
    imageDescH = NULL;
    desc = NULL;

    GWorldPtr gWorld;
    QDErr err = noErr;
    Rect boundsRect = { 0, 0, (short)img_w, (short)img_h };

    MemoryBuffer* buf = MemoryBuffer::Create(data_size);
    
    if( buff == NULL ) {
            error("no bitmap buffer available");
            exit(1);
    }
    
    err = NewGWorldFromPtr( &gWorld, k32ARGBPixelFormat, &boundsRect, NULL, NULL, 0, 
                                                    bi->data, bi->bytesPerRow );
    if (noErr != err) {
            error("error creating new gworld - %d", err);
            exit(1);
    }
    
    if( (result = GraphicsImportSetGWorld(gi, gWorld, NULL)) != noErr ) {
            error("error while setting gworld");
            exit(1);
    }
    
    if( (result = GraphicsImportDraw(gi)) != noErr ) {
            error("error while drawing image through qt");
            exit(1);
    }
    
    DisposeGWorld(gWorld);	
*/
#else
    char header[8];
    png_structp png_ptr;
    png_infop info_ptr;
    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type;
    FILE* fp = NULL;
    Texture* tex = NULL;
    png_bytep* row_pointers = NULL;

    fp = fopen(filename.c_str(), "rb");
    if (fp == NULL)
    {
        DPRINTF(0, "Error opening texture file %s\n", filename.c_str());
        return NULL;
    }
   
    fread(header, 1, sizeof(header), fp);
    if (png_sig_cmp((unsigned char*) header, 0, sizeof(header)))
    {
        DPRINTF(0, "Error: %s is not a PNG file.\n", filename.c_str());
        fclose(fp);
        return NULL;
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                     NULL, NULL, NULL);
    if (png_ptr == NULL)
    {
        fclose(fp);
        return NULL;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
    {
        fclose(fp);
        png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
        return NULL;
    }
   
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        fclose(fp);
        if (tex != NULL)
            delete tex;
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
        DPRINTF(0, "Error reading PNG texture file %s\n", filename.c_str());
        return NULL;
    }

    // png_init_io(png_ptr, fp);
    png_set_read_fn(png_ptr, (void*) fp, PNGReadData);
    png_set_sig_bytes(png_ptr, sizeof(header));

    png_read_info(png_ptr, info_ptr);

    png_get_IHDR(png_ptr, info_ptr,
                 &width, &height, &bit_depth,
                 &color_type, &interlace_type,
                 NULL, NULL);

    GLenum glformat = GL_RGB;
    switch (color_type)
    {
    case PNG_COLOR_TYPE_GRAY:
        glformat = GL_LUMINANCE;
        break;
    case PNG_COLOR_TYPE_GRAY_ALPHA:
        glformat = GL_LUMINANCE_ALPHA;
        break;
    case PNG_COLOR_TYPE_RGB:
        glformat = GL_RGB;
        break;
    case PNG_COLOR_TYPE_PALETTE:
    case PNG_COLOR_TYPE_RGB_ALPHA:
        glformat = GL_RGBA;
        break;
    default:
        // badness
        break;
    }

    tex = new Texture(width, height, glformat);
    if (tex == NULL)
    {
        fclose(fp);
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
        return NULL;
    }

    // TODO: consider using paletted textures if they're available
    if (color_type == PNG_COLOR_TYPE_PALETTE)
    {
        png_set_palette_to_rgb(png_ptr);
    }

    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    {
        png_set_gray_1_2_4_to_8(png_ptr);
    }

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
    {
        png_set_tRNS_to_alpha(png_ptr);
    }

    // TODO: consider passing textures with < 8 bits/component to
    // GL without expanding
    if (bit_depth == 16)
        png_set_strip_16(png_ptr);
    else if (bit_depth < 8)
        png_set_packing(png_ptr);

    row_pointers = new png_bytep[height];
    for (unsigned int i = 0; i < height; i++)
        row_pointers[i] = (png_bytep) &tex->pixels[tex->components * width * i];

    png_read_image(png_ptr, row_pointers);

    delete[] row_pointers;

    png_read_end(png_ptr, NULL);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    return tex;
#endif
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


static Texture* CreateBMPTexture(ifstream& in)
{
    BMPFileHeader fileHeader;
    BMPImageHeader imageHeader;
    unsigned char* pixels;

    in >> fileHeader.b;
    in >> fileHeader.m;
    fileHeader.size = readInt(in);
    fileHeader.reserved = readInt(in);
    fileHeader.offset = readInt(in);

    if (fileHeader.b != 'B' || fileHeader.m != 'M')
        return NULL;

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

#if 0
    printf("%d Planes @ %d BPP\n", imageHeader.planes, imageHeader.bpp);
    printf("Size: %d\n", imageHeader.size);
    printf("Dimensions: %d x %d\n", imageHeader.width, imageHeader.height);
#endif

    if (imageHeader.width <= 0 || imageHeader.height <= 0)
        return NULL;

    // We currently don't support compressed BMPs
    if (imageHeader.compression != 0)
        return NULL;
    // We don't handle 1-, 2-, or 4-bpp images
    if (imageHeader.bpp != 8 && imageHeader.bpp != 24 && imageHeader.bpp != 32)
        return NULL;

#if 0
    printf("Image size: %d\n", imageHeader.imageSize);
    printf("Compression: %d\n", imageHeader.compression);
    printf("WidthPPM x HeightPPM: %d x %d\n", imageHeader.widthPPM, imageHeader.heightPPM);
#endif

    unsigned char* palette = NULL;
    if (imageHeader.bpp == 8)
    {
        printf("Reading %d color palette\n", imageHeader.colorsUsed);
        palette = new unsigned char[imageHeader.colorsUsed * 4];
        in.read(reinterpret_cast<char*>(palette), imageHeader.colorsUsed * 4);
    }

    in.seekg(fileHeader.offset, ios::beg);

    unsigned int bytesPerRow =
        (imageHeader.width * imageHeader.bpp / 8 + 1) & ~1;
    unsigned int imageBytes = bytesPerRow * imageHeader.height;

    // slurp the image data
    pixels = new unsigned char[imageBytes];
    in.read(reinterpret_cast<char*>(pixels), imageBytes);

    // check for truncated file

    Texture* tex = new Texture(imageHeader.width, imageHeader.height,
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


Texture* CreateBMPTexture(const char* filename)
{
    ifstream bmpFile(filename, ios::in | ios::binary);

    if (bmpFile.good())
    {
        Texture* tex = CreateBMPTexture(bmpFile);
        bmpFile.close();
        return tex;
    }
    else
    {
        return NULL;
    }
}


// Helper function for CreateNormalizationCubeMap
static Vec3f cubeVector(int face, float s, float t)
{
    Vec3f v;
    switch (face)
    {
    case 0:
        v = Vec3f(1.0f, -t, -s);
        break;
    case 1:
        v = Vec3f(-1.0f, -t, s);
        break;
    case 2:
        v = Vec3f(s, 1.0f, t);
        break;
    case 3:
        v = Vec3f(s, -1.0f, -t);
        break;
    case 4:
        v = Vec3f(s, -t, 1.0f);
        break;
    case 5:
        v = Vec3f(-s, -t, -1.0f);
        break;
    default:
        // assert(false);
        break;
    }

    v.normalize();

    return v;
}


// Build a normalization cube map.  This is used when bump mapping to keep
// the light vector unit length when interpolating.  bindName() need not
// (and must not) be called for a texture created with this method, as the
// name binding stuff all handled right here.
Texture* CreateNormalizationCubeMap(int size)
{
    // assert(ExtensionSupported("GL_EXT_texture_cube_map"));
    
    Texture* tex = new Texture(size, size, GL_RGB);
    if (tex == NULL)
        return NULL;

    glGenTextures(1, (GLuint*) &tex->glNames[0]);
    glBindTexture(GL_TEXTURE_CUBE_MAP_EXT, tex->glNames[0]);
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    for (int face = 0; face < 6; face++)
    {
        for (int y = 0; y < size; y++)
        {
            for (int x = 0; x < size; x++)
            {
                float s = ((float) x + 0.5f) / (float) size * 2 - 1;
                float t = ((float) y + 0.5f) / (float) size * 2 - 1;
                Vec3f v = cubeVector(face, s, t);
                tex->pixels[(y * size + x) * 3]     = 128 + (int) (127 * v.x);
                tex->pixels[(y * size + x) * 3 + 1] = 128 + (int) (127 * v.y);
                tex->pixels[(y * size + x) * 3 + 2] = 128 + (int) (127 * v.z);
            }
        }

        glTexImage2D((GLenum) ((int) GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT + face),
                     0, GL_RGB8,
                     size, size,
                     0, GL_RGB,
                     GL_UNSIGNED_BYTE,
                     tex->pixels);
    }

    return tex;
}


Texture* CreateDiffuseLightCubeMap(int size)
{
    // assert(ExtensionSupported("GL_EXT_texture_cube_map"));
    
    Texture* tex = new Texture(size, size, GL_RGB);
    if (tex == NULL)
        return NULL;

    GLuint tn;
    glGenTextures(1, &tn);
    glBindTexture(GL_TEXTURE_2D, tn);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    for (int face = 0; face < 6; face++)
    {
        for (int y = 0; y < size; y++)
        {
            for (int x = 0; x < size; x++)
            {
                float s = ((float) x + 0.5f) / (float) size * 2 - 1;
                float t = ((float) y + 0.5f) / (float) size * 2 - 1;
                Vec3f v = cubeVector(face, s, t);
                float Lz = v.z < 0.0f ? 0.0f : v.z;
                tex->pixels[(y * size + x) * 3]     = (int) (255.99f * Lz);
                tex->pixels[(y * size + x) * 3 + 1] = (int) (255.99f * Lz);
                tex->pixels[(y * size + x) * 3 + 2] = (int) (255.99f * Lz);
            }
        }

        glTexImage2D((GLenum) ((int) GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT + face),
                     0, GL_RGB8,
                     size, size,
                     0, GL_RGB,
                     GL_UNSIGNED_BYTE,
                     tex->pixels);
    }

    return tex;
}


Texture* CreateProceduralCubeMap(int size, int format,
                                 ProceduralTexEval func)
{
    Texture* tex = new Texture(size, size, format, true);
    if (tex == NULL)
        return NULL;

    for (int face = 0; face < 6; face++)
    {
        for (int y = 0; y < size; y++)
        {
            for (int x = 0; x < size; x++)
            {
                float s = ((float) x + 0.5f) / (float) size * 2 - 1;
                float t = ((float) y + 0.5f) / (float) size * 2 - 1;
                Vec3f v = cubeVector(face, s, t);
                func(v.x, v.y, v.z, tex->pixels + ((face * size + y) * size + x) * tex->components);
            }
        }
    }

    return tex;
}
