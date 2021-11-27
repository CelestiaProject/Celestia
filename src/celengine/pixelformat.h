// pixelformat.h
//
// Copyright (C) 2021-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

namespace celestia
{
enum class PixelFormat
{
    INVALID     = -1,
    RGB         = 0x1907, // GL_RGB
    RGBA        = 0x1908, // GL_RGBA
    BGR         = 0x80E0, // GL_BGR
    BGRA        = 0x80E1, // GL_BGRA
    LUM_ALPHA   = 0x190A, // GL_LUMINANCE_ALPHA
    ALPHA       = 0x1906, // GL_ALPHA
    LUMINANCE   = 0x1909, // GL_LUMINANCE
    DXT1        = 0x83F1, // GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
    DXT3        = 0x83F2, // GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
    DXT5        = 0x83F3, // GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
    SLUM_ALPHA  = 0x8C44, // GL_SLUMINANCE_ALPHA
    SLUMINANCE  = 0x8C46, // GL_SLUMINANCE
    SRGB        = 0x8C40, // GL_SRGB
    SRGB8       = 0x8C41, // GL_SRGB8
    SRGBA       = 0x8C42, // GL_SRGB_ALPHA
    SRGBA8      = 0x8C43, // GL_SRGB8_ALPHA8
    DXT1_SRGBA  = 0x8C4D, // GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT
    DXT3_SRGBA  = 0x8C4E, // GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT
    DXT5_SRGBA  = 0x8C4F, // GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT
};
}
