// imagecapture.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <csetjmp>
#include <cstdio>

#include <celutil/debug.h>
#include <celengine/gl.h>
#include <celengine/celestia.h>
#include "imagecapture.h"

extern "C" {
#ifdef _WIN32
#include "jpeglib.h"
#else
#ifdef MACOSX
#include "../celestia/Celestia.app.skel/Contents/Frameworks/Headers/jpeglib.h"
#else
#include <jpeglib.h>
#endif
#endif
}

#ifdef MACOSX
#include "../celestia/Celestia.app.skel/Contents/Frameworks/Headers/png.h"
#else
#include "png.h"
#endif
#include <zlib.h>

using namespace std;


bool CaptureGLBufferToJPEG(const string& filename,
                           int x, int y,
                           int width, int height)
{
    int rowStride = (width * 3 + 3) & ~0x3;
    int imageSize = height * rowStride;
    unsigned char* pixels = new unsigned char[imageSize];

    glReadBuffer(GL_BACK);
    glReadPixels(x, y, width, height,
                 GL_RGB, GL_UNSIGNED_BYTE,
                 pixels);

    // TODO: Check for GL errors

    FILE* out;
    out = fopen(filename.c_str(), "wb");
    if (out == NULL)
    {
        DPRINTF(0, "Can't open screen capture file '%s'\n", filename.c_str());
        delete[] pixels;
        return false;
    }

    struct jpeg_compress_struct cinfo;

    struct jpeg_error_mgr jerr;
    JSAMPROW row[1];

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    jpeg_stdio_dest(&cinfo, out);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);

    // jpeg_set_quality(&cinfo, quality, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    while (cinfo.next_scanline < cinfo.image_height)
    {
        row[0] = &pixels[rowStride * (cinfo.image_height - cinfo.next_scanline - 1)];
        (void) jpeg_write_scanlines(&cinfo, row, 1);
    }

    jpeg_finish_compress(&cinfo);
    fclose(out);
    jpeg_destroy_compress(&cinfo);

    delete[] pixels;

    return true;
}

static void
PNGError(png_structp pngPtr, png_const_charp error)
{
    auto filename = static_cast<const std::string*>(png_get_error_ptr(pngPtr));
    DPRINTF(0, "Screen capture error '%s': %s\n", filename->c_str(), error);
    png_longjmp(pngPtr, static_cast<int>(true));
}

static void
PNGWarn(png_structp pngPtr, png_const_charp warning)
{
    auto filename = static_cast<const std::string*>(png_get_error_ptr(pngPtr));
    DPRINTF(0, "Screen capture warning '%s': %s\n", filename->c_str(), warning);
}

static bool
WritePNGToFile(const std::string& filename,
               unsigned char* pixels,
               int width, int height, int rowStride)
{
    png_structp pngPtr = NULL;
    png_infop infoPtr = NULL;

    std::FILE* out = std::fopen(filename.c_str(), "wb");
    if (!out)
    {
        DPRINTF(0, "Can't open screen capture file '%s'\n", filename.c_str());
        return false;
    }

    pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                     const_cast<std::string*>(&filename),
                                     &PNGError,
                                     &PNGWarn);
    if (!pngPtr)
    {
        DPRINTF(0, "Screen capture: error allocating png_ptr\n");
        std::fclose(out);
        return false;
    }

    infoPtr = png_create_info_struct(pngPtr);
    if (!infoPtr)
    {
        DPRINTF(0, "Screen capture: error allocating info_ptr\n");
        png_destroy_write_struct(&pngPtr, NULL);
        std::fclose(out);
        return false;
    }

    if (setjmp(png_jmpbuf(pngPtr)))
    {
        png_destroy_write_struct(&pngPtr, &infoPtr);
        std::fclose(out);
        return false;
    }

    png_init_io(pngPtr, out);
    png_set_compression_level(pngPtr, Z_BEST_COMPRESSION);
    png_set_IHDR(pngPtr, infoPtr,
                 static_cast<png_uint_32>(width),
                 static_cast<png_uint_32>(height),
                 8,
                 PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    png_write_info(pngPtr, infoPtr);

    for (int row = 0; row < height; ++row)
    {
        png_write_row(pngPtr, pixels);
        pixels += rowStride;
    }

    png_write_end(pngPtr, infoPtr);

    png_destroy_write_struct(&pngPtr, &infoPtr);
    std::fclose(out);

    return true;
}

bool CaptureGLBufferToPNG(const string& filename,
                          int x, int y,
                          int width, int height)
{
    int rowStride = (width * 3 + 3) & ~0x3;
    int imageSize = height * rowStride;
    unsigned char* pixels = new unsigned char[imageSize];

    glReadBuffer(GL_BACK);
    glReadPixels(x, y, width, height,
                 GL_RGB, GL_UNSIGNED_BYTE,
                 pixels);

    // TODO: Check for GL errors
    auto result = WritePNGToFile(filename, pixels, width, height, rowStride);
    delete[] pixels;

    return result;
}
