// filetype.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _FILETYPE_H_
#define _FILETYPE_H_

#include <string>

enum ContentType {
    Content_JPEG,
    Content_BMP,
    Content_GIF,
    Content_Targa,
    Content_CelestiaTexture,
    Content_3DStudio,
    Content_CelestiaMesh,
    Content_Unknown
};

ContentType DetermineFileType(const std::string& filename);

#endif // _FILETYPE_H_
