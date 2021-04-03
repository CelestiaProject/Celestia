// imageformats.h
//
// Copyright (C) 2001-present, the Celestia Development Team
// Original version by Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <celengine/image.h>

Image* LoadJPEGImage(const fs::path& filename,
                     int channels = Image::ColorChannel);
Image* LoadBMPImage(const fs::path& filename);
Image* LoadPNGImage(const fs::path& filename);
Image* LoadDDSImage(const fs::path& filename);

bool SaveJPEGImage(const fs::path& filename,
                   int width, int height,
                   int rowStride,
                   unsigned char* pixels,
                   bool stripAlpha = false);
bool SavePNGImage(const fs::path& filename,
                  int width, int height,
                  int rowStride,
                  unsigned char* pixels,
                  bool stripAlpha = false);
