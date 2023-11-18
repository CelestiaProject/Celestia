// image.h
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>
#include <memory>

#include <celcompat/filesystem.h>
#include <celutil/filetype.h>
#include "pixelformat.h"

namespace celestia::engine
{

/**
 * The image class supports multiple GL formats, including compressed ones.
 * Mipmaps may be stored within an image as well.  The mipmaps are stored in
 * one contiguous block of memory (i.e. there's not an instance of Image per
 * mipmap.)  Mip levels are addressed such that zero is the base (largest) mip
 * level.
 */
class Image
{
public:
    Image(PixelFormat format, int w, int h, int mip = 1);
    ~Image() = default;
    Image(Image&&) = default;
    Image(const Image&) = delete;
    Image& operator=(Image&&) = default;
    Image& operator=(const Image&) = delete;

    bool isValid() const noexcept;
    int getWidth() const;
    int getHeight() const;
    int getPitch() const;
    int getMipLevelCount() const;
    PixelFormat getFormat() const;
    int getComponents() const;
    std::uint8_t* getPixels();
    const std::uint8_t* getPixels() const;
    std::uint8_t* getPixelRow(int row);
    std::uint8_t* getPixelRow(int mip, int row);
    std::uint8_t* getMipLevel(int mip);
    const std::uint8_t* getMipLevel(int mip) const;
    int getSize() const;
    int getMipLevelSize(int mip) const;

    bool isCompressed() const;
    bool hasAlpha() const;

    std::unique_ptr<Image> computeNormalMap(float scale, bool wrap) const;

    void forceLinear();

    static bool canSave(ContentType type);
    bool save(const fs::path &path, ContentType type) const;

    static std::unique_ptr<Image> load(const fs::path& filename);

private:
    int width;
    int height;
    int pitch;
    int mipLevels;
    int components;
    PixelFormat format;
    int size;
    std::unique_ptr<std::uint8_t[]> pixels;
};

} // namespace celestia::engine
