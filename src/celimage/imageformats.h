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

#include <filesystem>

#include <celimage/image.h>

namespace celestia::engine
{

Image* LoadJPEGImage(const std::filesystem::path& filename);
Image* LoadBMPImage(const std::filesystem::path& filename);
Image* LoadPNGImage(const std::filesystem::path& filename);
Image* LoadDDSImage(const std::filesystem::path& filename);
#ifdef USE_LIBAVIF
Image* LoadAVIFImage(const std::filesystem::path& filename);
#endif

bool SaveJPEGImage(const std::filesystem::path& filename, const Image& image);
bool SavePNGImage(const std::filesystem::path& filename, const Image& image);

} // namespace celestia::engine
