// imagecapture.cpp
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstdio>
#include "gl.h"
#include "celestia.h"
#include "imagecapture.h"

extern "C" {
#ifdef _WIN32
#include "jpeglib.h"
#else
#include <jpeglib.h>
#endif
}

#include "png.h"

// Define png_jmpbuf() in case we are using a pre-1.0.6 version of libpng
#ifndef png_jmpbuf
#define png_jmpbuf(png_ptr) png_ptr->jmpbuf
#endif

// Define various expansion transformations for old versions of libpng
#if PNG_LIBPNG_VER < 10004
#define png_set_palette_to_rgb(p)  png_set_expand(p)
#define png_set_gray_1_2_4_to_8(p) png_set_expand(p)
#define png_set_tRNS_to_alpha(p)   png_set_expand(p)
#endif

using namespace std;


bool CaptureGLBufferToJPEG(const string& filename,
                           int x, int y,
                           int width, int height)
{
    int rowStride = width * 3;
    int imageSize = height * rowStride;
    unsigned char* pixels = new unsigned char[imageSize];

    glReadPixels(x, y, width, height,
                 GL_RGB, GL_UNSIGNED_BYTE,
                 pixels);

    // TODO: Check for GL errors

    FILE* out;
    out = fopen(filename.c_str(), "wb");
    if (out == NULL)
    {
        DPRINTF("Can't open screen capture file '%s'\n", filename.c_str());
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

                           
