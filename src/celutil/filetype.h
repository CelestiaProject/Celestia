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
    Content_JPEG                   = 1,
    Content_BMP                    = 2,
    Content_GIF                    = 3,
    Content_PNG                    = 4,
    Content_Targa                  = 5,
    Content_CelestiaTexture        = 6,
    Content_3DStudio               = 7,
    Content_CelestiaMesh           = 8,
    Content_AVI                    = 9,
    Content_CelestiaCatalog        = 10,
    Content_DDS                    = 11,
    Content_CelestiaStarCatalog    = 12,
    Content_CelestiaDeepSkyCatalog = 13,
    Content_CelestiaScript         = 14,
    Content_CelestiaLegacyScript   = 15,
    Content_CelestiaModel          = 16,
    Content_DXT5NormalMap          = 17,
    Content_CelestiaXYZTrajectory  = 18,
    Content_CelestiaXYZVTrajectory = 19,
    Content_CelestiaParticleSystem = 20,
    Content_Unknown                = -1,
};

ContentType DetermineFileType(const std::string& filename);

#endif // _FILETYPE_H_
