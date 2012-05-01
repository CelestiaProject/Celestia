// imagecapture.cpp
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstdio>
#include <celutil/debug.h>
#include <GL/glew.h>
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

// Define png_jmpbuf() in case we are using a pre-1.0.6 version of libpng
#ifndef png_jmpbuf
#define png_jmpbuf(png_ptr) png_ptr->jmpbuf
#endif

#if PNG_LIBPNG_VER < 10004
// Define various expansion transformations for old versions of libpng
#define png_set_palette_to_rgb(p)  png_set_expand(p)
#define png_set_gray_1_2_4_to_8(p) png_set_expand(p)
#define png_set_tRNS_to_alpha(p)   png_set_expand(p)
#elif PNG_LIBPNG_VER >= 10500
// libpng-1.5 include does not pull in zlib.h
#include "zlib.h"
#endif

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


void PNGWriteData(png_structp png_ptr, png_bytep data, png_size_t length)
{
    FILE* fp = (FILE*) png_get_io_ptr(png_ptr);
    fwrite((void*) data, 1, length, fp);
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

    FILE* out;
    out = fopen(filename.c_str(), "wb");
    if (out == NULL)
    {
        DPRINTF(0, "Can't open screen capture file '%s'\n", filename.c_str());
        delete[] pixels;
        return false;
    }

    png_bytep* row_pointers = new png_bytep[height];
    for (int i = 0; i < height; i++)
        row_pointers[i] = (png_bytep) &pixels[rowStride * (height - i - 1)];

    png_structp png_ptr;
    png_infop info_ptr;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                      NULL, NULL, NULL);

    if (png_ptr == NULL)
    {
        DPRINTF(0, "Screen capture: error allocating png_ptr\n");
        fclose(out);
        delete[] pixels;
        delete[] row_pointers;
        return false;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
    {
        DPRINTF(0, "Screen capture: error allocating info_ptr\n");
        fclose(out);
        delete[] pixels;
        delete[] row_pointers;
        png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        DPRINTF(0, "Error writing PNG file '%s'\n", filename.c_str());
        fclose(out);
        delete[] pixels;
        delete[] row_pointers;
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return false;
    }

    // png_init_io(png_ptr, out);
    png_set_write_fn(png_ptr, (void*) out, PNGWriteData, NULL);

    png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
    png_set_IHDR(png_ptr, info_ptr,
                 width, height,
                 8,
                 PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);
    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, info_ptr);

    // Clean up everything . . .
    png_destroy_write_struct(&png_ptr, &info_ptr);
    delete[] row_pointers;
    delete[] pixels;
    fclose(out);

    return true;
}


