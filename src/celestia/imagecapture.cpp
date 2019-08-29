// imagecapture.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <config.h>
#include <celutil/debug.h>
#include "imagecapture.h"

extern "C" {
#include <jpeglib.h>
}
#include <png.h>
#include <zlib.h>

using namespace std;


bool CaptureGLBufferToJPEG(const fs::path& filename,
                           int x, int y,
                           int width, int height,
                           const Renderer *renderer)
{
    int rowStride = (width * 3 + 3) & ~0x3;
    int imageSize = height * rowStride;
    auto* pixels = new unsigned char[imageSize];

    if (!renderer->captureFrame(x, y, width, height,
                                Renderer::PixelFormat::RGB,
                                pixels, true))
    {
        return false;
    }

    FILE* out;
#ifdef _WIN32
    out = _wfopen(filename.c_str(), L"wb");
#else
    out = fopen(filename.c_str(), "wb");
#endif
    if (out == nullptr)
    {
        DPRINTF(LOG_LEVEL_ERROR, "Can't open screen capture file '%s'\n", filename);
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

    jpeg_set_quality(&cinfo, 90, TRUE);

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
    auto* fp = (FILE*) png_get_io_ptr(png_ptr);
    fwrite((void*) data, 1, length, fp);
}


bool CaptureGLBufferToPNG(const fs::path& filename,
                           int x, int y,
                           int width, int height,
                           const Renderer *renderer)
{
    int rowStride = (width * 3 + 3) & ~0x3;
    int imageSize = height * rowStride;
    auto* pixels = new unsigned char[imageSize];

    if (!renderer->captureFrame(x, y, width, height,
                                Renderer::PixelFormat::RGB,
                                pixels, true))
    {
        return false;
    }

#ifdef _WIN32
    FILE* out = _wfopen(filename.c_str(), L"wb");
#else
    FILE* out = fopen(filename.c_str(), "wb");
#endif
    if (out == nullptr)
    {
        DPRINTF(LOG_LEVEL_ERROR, "Can't open screen capture file '%s'\n", filename);
        delete[] pixels;
        return false;
    }

    auto* row_pointers = new png_bytep[height];
    for (int i = 0; i < height; i++)
        row_pointers[i] = (png_bytep) &pixels[rowStride * (height - i - 1)];

    png_structp png_ptr;
    png_infop info_ptr;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                      nullptr, nullptr, nullptr);

    if (png_ptr == nullptr)
    {
        DPRINTF(LOG_LEVEL_ERROR, "Screen capture: error allocating png_ptr\n");
        fclose(out);
        delete[] pixels;
        delete[] row_pointers;
        return false;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == nullptr)
    {
        DPRINTF(LOG_LEVEL_ERROR, "Screen capture: error allocating info_ptr\n");
        fclose(out);
        delete[] pixels;
        delete[] row_pointers;
        png_destroy_write_struct(&png_ptr, (png_infopp) nullptr);
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        DPRINTF(LOG_LEVEL_ERROR, "Error writing PNG file '%s'\n", filename);
        fclose(out);
        delete[] pixels;
        delete[] row_pointers;
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return false;
    }

    // png_init_io(png_ptr, out);
    png_set_write_fn(png_ptr, (void*) out, PNGWriteData, nullptr);

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
