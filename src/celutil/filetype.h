// filetype.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string>
#include <celcompat/filesystem.h>

enum class ContentType
{
    JPEG                   = 1,
    BMP                    = 2,
    GIF                    = 3,
    PNG                    = 4,
    Targa                  = 5,
    CelestiaTexture        = 6,
    _3DStudio              = 7,
    CelestiaMesh           = 8,
    MKV                    = 9,
    CelestiaCatalog        = 10,
    DDS                    = 11,
    CelestiaStarCatalog    = 12,
    CelestiaDeepSkyCatalog = 13,
    CelestiaScript         = 14,
    CelestiaLegacyScript   = 15,
    CelestiaModel          = 16,
    DXT5NormalMap          = 17,
    CelestiaXYZTrajectory  = 18,
    CelestiaXYZVTrajectory = 19,
 // CelestiaParticleSystem = 20,
    WarpMesh               = 21,
    CelestiaXYZVBinary     = 22,
#ifdef USE_LIBAVIF
    AVIF                   = 23,
#endif
    Unknown                = -1,
};

ContentType DetermineFileType(const fs::path& filename);
