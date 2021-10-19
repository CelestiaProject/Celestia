// png.cpp
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include <png.h>
#include <zlib.h>
#include <fmt/printf.h>
#include <celengine/image.h>
#include <celutil/debug.h>
#include <celutil/gettext.h>

using std::cerr;
using std::clog;
using celestia::PixelFormat;

namespace
{
void PNGReadData(png_structp png_ptr, png_bytep data, png_size_t length)
{
    auto* fp = (FILE*) png_get_io_ptr(png_ptr);
    if (fread((void*) data, 1, length, fp) != length)
        cerr << "Error reading PNG data";
}

void PNGWriteData(png_structp png_ptr, png_bytep data, png_size_t length)
{
    auto* fp = (FILE*) png_get_io_ptr(png_ptr);
    fwrite((void*) data, 1, length, fp);
}
} // anonymous namespace

Image* LoadPNGImage(const fs::path& filename)
{
    char header[8];
    png_structp png_ptr;
    png_infop info_ptr;
    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type;
    Image* img = nullptr;
    png_bytep* row_pointers = nullptr;

#ifdef _WIN32
    FILE *fp = _wfopen(filename.c_str(), L"rb");
#else
    FILE *fp = fopen(filename.c_str(), "rb");
#endif
    if (fp == nullptr)
    {
        clog << fmt::sprintf(_("Error opening image file %s\n"), filename.string());
        return nullptr;
    }

    size_t elements_read;
    elements_read = fread(header, 1, sizeof(header), fp);
    if (elements_read == 0 || png_sig_cmp((unsigned char*) header, 0, sizeof(header)))
    {
        clog << fmt::sprintf(_("Error: %s is not a PNG file.\n"), filename.string());
        fclose(fp);
        return nullptr;
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                     nullptr, nullptr, nullptr);
    if (png_ptr == nullptr)
    {
        fclose(fp);
        return nullptr;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == nullptr)
    {
        fclose(fp);
        png_destroy_read_struct(&png_ptr, (png_infopp) nullptr, (png_infopp) nullptr);
        return nullptr;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        fclose(fp);
        delete img;
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) nullptr);
        clog << fmt::sprintf(_("Error reading PNG image file %s\n"), filename.string());
        return nullptr;
    }

    // png_init_io(png_ptr, fp);
    png_set_read_fn(png_ptr, (void*) fp, PNGReadData);
    png_set_sig_bytes(png_ptr, sizeof(header));

    png_read_info(png_ptr, info_ptr);

    png_get_IHDR(png_ptr, info_ptr,
                 &width, &height, &bit_depth,
                 &color_type, &interlace_type,
                 nullptr, nullptr);

    PixelFormat format = PixelFormat::RGB;
    switch (color_type)
    {
    case PNG_COLOR_TYPE_GRAY:
        format = PixelFormat::LUMINANCE;
        break;
    case PNG_COLOR_TYPE_GRAY_ALPHA:
        format = PixelFormat::LUM_ALPHA;
        break;
    case PNG_COLOR_TYPE_RGB:
        format = PixelFormat::RGB;
        break;
    case PNG_COLOR_TYPE_PALETTE:
    case PNG_COLOR_TYPE_RGB_ALPHA:
        format = PixelFormat::RGBA;
        break;
    default:
        // badness
        break;
    }

    img = new Image(format, width, height);

    // TODO: consider using paletted textures if they're available
    if (color_type == PNG_COLOR_TYPE_PALETTE)
    {
        png_set_palette_to_rgb(png_ptr);
    }

    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    {
        png_set_expand_gray_1_2_4_to_8(png_ptr);
    }

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
    {
        png_set_tRNS_to_alpha(png_ptr);
    }

    // TODO: consider passing images with < 8 bits/component to
    // GL without expanding
    if (bit_depth == 16)
        png_set_strip_16(png_ptr);
    else if (bit_depth < 8)
        png_set_packing(png_ptr);

    row_pointers = new png_bytep[height];
    for (unsigned int i = 0; i < height; i++)
        row_pointers[i] = (png_bytep) img->getPixelRow(i);

    png_read_image(png_ptr, row_pointers);

    delete[] row_pointers;

    png_read_end(png_ptr, nullptr);
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

    fclose(fp);

    return img;
}

bool SavePNGImage(const fs::path& filename,
                  int width, int height,
                  int rowStride,
                  unsigned char *pixels,
                  bool removeAlpha)
{
#ifdef _WIN32
    FILE* out = _wfopen(filename.c_str(), L"wb");
#else
    FILE* out = fopen(filename.c_str(), "wb");
#endif
    if (out == nullptr)
    {
        DPRINTF(LOG_LEVEL_ERROR, "Can't open screen capture file '%s'\n", filename);
        return false;
    }

    auto* row_pointers = new png_bytep[height];
    for (int i = 0; i < height; i++)
    {
        unsigned char *rowHead = &pixels[rowStride * i];
        // Strip alpha values if we are in RGBA format
        if (removeAlpha)
        {
            for (int x = 0; x < width; x++)
            {
                const unsigned char* pixelIn = &rowHead[x * 4];
                unsigned char* pixelOut = &rowHead[x * 3];
                pixelOut[0] = pixelIn[0];
                pixelOut[1] = pixelIn[1];
                pixelOut[2] = pixelIn[2];
            }
        }
        row_pointers[i] = (png_bytep) rowHead;
    }

    png_structp png_ptr;
    png_infop info_ptr;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                      nullptr, nullptr, nullptr);

    if (png_ptr == nullptr)
    {
        DPRINTF(LOG_LEVEL_ERROR, "Screen capture: error allocating png_ptr\n");
        fclose(out);
        delete[] row_pointers;
        return false;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == nullptr)
    {
        DPRINTF(LOG_LEVEL_ERROR, "Screen capture: error allocating info_ptr\n");
        fclose(out);
        delete[] row_pointers;
        png_destroy_write_struct(&png_ptr, (png_infopp) nullptr);
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        DPRINTF(LOG_LEVEL_ERROR, "Error writing PNG file '%s'\n", filename);
        fclose(out);
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
    fclose(out);

    return true;
}

bool SavePNGImage(const fs::path& filename, Image& image)
{
    return SavePNGImage(filename,
                        image.getWidth(),
                        image.getHeight(),
                        image.getPitch(),
                        image.getPixels(),
                        image.hasAlpha());
}

