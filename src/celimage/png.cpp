// png.cpp
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <csetjmp>
#include <cstdint>
#include <cstdio>
#include <cstddef>

#include <png.h>
#include <zlib.h>

#include <celcompat/filesystem.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include "image.h"
#include "pixelformat.h"

namespace celestia::engine
{
namespace
{

// WARNING: Calls to libpng functions may invoke longjmp - be very cautious
// when using types that have non-trivial destructors in the code below.

[[noreturn]] void
PNGError(png_structp pngPtr, png_const_charp error) //NOSONAR
{
    auto filename = static_cast<const fs::path*>(png_get_error_ptr(pngPtr));
    util::GetLogger()->error(_("PNG error in '{}': {}\n"), *filename, error);
    png_longjmp(pngPtr, static_cast<int>(true));
}

void
PNGWarn(png_structp pngPtr, png_const_charp warning) //NOSONAR
{
    auto filename = static_cast<const fs::path*>(png_get_error_ptr(pngPtr));
    util::GetLogger()->warn(_("PNG warning in '{}': {}\n"), *filename, warning);
}

PixelFormat
GetPixelFormat(png_structp pngPtr, png_const_infop infoPtr, int bitDepth, int colorType)
{
    if (colorType == PNG_COLOR_TYPE_PALETTE)
    {
        png_set_palette_to_rgb(pngPtr);
        if (png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS))
        {
            png_set_tRNS_to_alpha(pngPtr);
            return PixelFormat::RGBA;
        }

        return PixelFormat::RGB;
    }

    if (bitDepth < 8)
    {
        png_set_packing(pngPtr);
        if (colorType == PNG_COLOR_TYPE_GRAY)
            png_set_expand_gray_1_2_4_to_8(pngPtr);
    }
    else if (bitDepth == 16)
    {
#if PNG_LIBPNG_VER >= 10504
        png_set_scale_16(pngPtr);
#else
        png_set_strip_16(pngPtr);
#endif
    }

    if (png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS))
    {
        png_set_tRNS_to_alpha(pngPtr);
        colorType |= PNG_COLOR_MASK_ALPHA;
    }

    switch (colorType)
    {
    case PNG_COLOR_TYPE_GRAY:
        return PixelFormat::Luminance;
    case PNG_COLOR_TYPE_GRAY_ALPHA:
        return PixelFormat::LumAlpha;
    case PNG_COLOR_TYPE_RGB:
        return PixelFormat::RGB;
    case PNG_COLOR_TYPE_RGB_ALPHA:
        return PixelFormat::RGBA;
    default:
        png_error(pngPtr, _("Unsupported color type"));
    }
}

Image*
LoadPNGImage(std::FILE* in, const fs::path& filename)
{
    constexpr std::size_t headerSize = 8;
    png_byte header[headerSize]; //NOSONAR
    png_structp pngPtr = nullptr;
    png_infop infoPtr = nullptr;
    png_uint_32 width;
    png_uint_32 height;
    int bitDepth;
    int colorType;
    PixelFormat format;
    int passes;
    std::int32_t pitch;
    Image* img = nullptr;

    if (std::fread(header, 1, headerSize, in) != headerSize ||
        png_sig_cmp(header, 0, headerSize) != 0)
    {
        util::GetLogger()->error(_("Error: {} is not a PNG file.\n"), filename);
        return nullptr;
    }

    pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                    const_cast<fs::path*>(&filename), //NOSONAR
                                    &PNGError,
                                    &PNGWarn);
    if (pngPtr == nullptr)
    {
        util::GetLogger()->error(_("Error allocating PNG read struct.\n"));
        return nullptr;
    }

    infoPtr = png_create_info_struct(pngPtr);
    if (infoPtr == nullptr)
    {
        util::GetLogger()->error(_("Error allocating PNG info struct.\n"));
        png_destroy_read_struct(&pngPtr, nullptr, nullptr);
        return nullptr;
    }

    if (setjmp(png_jmpbuf(pngPtr)))
    {
        delete img; //NOSONAR
        png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);
        return nullptr;
    }

    png_init_io(pngPtr, in);
    png_set_sig_bytes(pngPtr, headerSize);

    png_read_info(pngPtr, infoPtr);
    if (png_get_IHDR(pngPtr, infoPtr, &width, &height, &bitDepth, &colorType, nullptr, nullptr, nullptr) == 0)
        png_error(pngPtr, _("Failed to read IHDR chunk"));

    if (width == 0 || width > Image::MAX_DIMENSION)
        png_error(pngPtr, _("Image width out of range"));
    if (height == 0 || height > Image::MAX_DIMENSION)
        png_error(pngPtr, _("Image height out of range"));

    format = GetPixelFormat(pngPtr, infoPtr, bitDepth, colorType);
    img = new Image(format, static_cast<std::int32_t>(width), static_cast<std::int32_t>(height)); //NOSONAR
    pitch = img->getPitch();

    passes = png_set_interlace_handling(pngPtr);
    for (int pass = 0; pass < passes; ++pass)
    {
        png_bytep rowPtr = img->getPixels();
        for (png_uint_32 row = 0; row < height; ++row)
        {
            png_read_row(pngPtr, rowPtr, nullptr);
            rowPtr += pitch;
        }
    }

    png_read_end(pngPtr, nullptr);
    png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);
    return img;
}

bool
SavePNGImage(std::FILE* out,
             const fs::path& filename,
             std::int32_t width,
             std::int32_t height,
             std::int32_t rowStride,
             const std::uint8_t* pixels,
             bool removeAlpha)
{
    png_structp pngPtr = nullptr;
    png_infop infoPtr = nullptr;

    pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                     const_cast<fs::path*>(&filename), //NOSONAR
                                     &PNGError,
                                     &PNGWarn);
    if (pngPtr == nullptr)
    {
        util::GetLogger()->error(_("Error allocating PNG write struct.\n"));
        return false;
    }

    infoPtr = png_create_info_struct(pngPtr);
    if (infoPtr == nullptr)
    {
        util::GetLogger()->error(_("Error allocating PNG info struct.\n"));
        png_destroy_write_struct(&pngPtr, nullptr);
        return false;
    }

    if (setjmp(png_jmpbuf(pngPtr)))
    {
        png_destroy_write_struct(&pngPtr, &infoPtr);
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

    if (removeAlpha)
        png_set_filler(pngPtr, 0, PNG_FILLER_AFTER);

    // We do not use interlacing so we can just write the rows out in order
    for (std::int32_t row = 0; row < height; ++row)
    {
        png_write_row(pngPtr, pixels);
        pixels += rowStride;
    }

    png_write_end(pngPtr, infoPtr);

    png_destroy_write_struct(&pngPtr, &infoPtr);
    return true;
}

} // end unnamed namespace

Image* LoadPNGImage(const fs::path& filename)
{
#ifdef _WIN32
    std::FILE* fp = _wfopen(filename.c_str(), L"rb");
#else
    std::FILE* fp = std::fopen(filename.c_str(), "rb");
#endif
    if (fp == nullptr)
    {
        util::GetLogger()->error(_("Error opening image file {}.\n"), filename);
        return nullptr;
    }

    Image* img = LoadPNGImage(fp, filename);

    std::fclose(fp);
    return img;
}

bool SavePNGImage(const fs::path& filename, const Image& image)
{
    if (auto format = image.getFormat();
        format != PixelFormat::RGB && format != PixelFormat::RGBA)
    {
        util::GetLogger()->error(_("Can only save RGB or RGBA images\n"));
        return false;
    }

#ifdef _WIN32
    std::FILE* out = _wfopen(filename.c_str(), L"wb");
#else
    std::FILE* out = std::fopen(filename.c_str(), "wb");
#endif
    if (out == nullptr)
    {
        util::GetLogger()->error(_("Can't open screen capture file '{}'\n"), filename);
        return false;
    }

    auto result = SavePNGImage(out, filename,
                               image.getWidth(),
                               image.getHeight(),
                               image.getPitch(),
                               image.getPixels(),
                               image.hasAlpha());

    std::fclose(out);
    return result;
}

} // end namespace celestia::engine
