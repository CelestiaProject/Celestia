// pixelformat.h
//
// Copyright (C) 2021-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

namespace celestia::engine
{
enum class PixelFormat
{
    Invalid     = -1,
    // base 3&4 channels formats
    RGB         = 0x1907, // GL_RGB
    RGB8        = 0x8051, // GL_RGB8
    RGBA        = 0x1908, // GL_RGBA
    RGBA8       = 0x8058, // GL_RGBA8
    BGR         = 0x80E0, // GL_BGR
    BGR8        = 0x80E0, // GL_BGR, no such BGR8 format in OpenGL actually
    BGRA        = 0x80E1, // GL_BGRA
    BGRA8       = 0x93A1, // GL_BGRA8
    // base 1&2 channels formats
    LumAlpha    = 0x190A, // GL_LUMINANCE_ALPHA
    RG8         = 0x822B, // GL_RG8
    Alpha       = 0x1906, // GL_ALPHA
    Luminance   = 0x1909, // GL_LUMINANCE
    R8          = 0x8229, // GL_R8
    // compressed formats
    DXT1        = 0x83F1, // GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
    DXT3        = 0x83F2, // GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
    DXT5        = 0x83F3, // GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
    // sRGB formats
    sLumAlpha   = 0x8C44, // GL_SLUMINANCE_ALPHA
    sRG8        = 0x8FBE, // GL_SRG8
    sLuminance  = 0x8C46, // GL_SLUMINANCE
    sR8         = 0x8FBD, // GL_SR8
    sRGB        = 0x8C40, // GL_SRGB
    sRGB8       = 0x8C41, // GL_SRGB8
    sRGBA       = 0x8C42, // GL_SRGB_ALPHA
    sRGBA8      = 0x8C43, // GL_SRGB8_ALPHA8
    // compressed sRGB formats
    DXT1_sRGBA  = 0x8C4D, // GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT
    DXT3_sRGBA  = 0x8C4E, // GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT
    DXT5_sRGBA  = 0x8C4F, // GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT
};
}
