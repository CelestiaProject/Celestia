// filetype.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "stringutils.h"
#include "filetype.h"

using namespace std;


static const char JPEGExt[] = ".jpeg";
static const char JPGExt[] = ".jpg";
static const char JFIFExt[] = ".jif";
static const char BMPExt[] = ".bmp";
static const char TargaExt[] = ".tga";
static const char PNGExt[] = ".png";
static const char ThreeDSExt[] = ".3ds";
static const char CelestiaTextureExt[] = ".ctx";
static const char CelestiaMeshExt[] = ".cms";
static const char CelestiaCatalogExt[] = ".ssc";
static const char CelestiaStarCatalogExt[] = ".stc";
static const char CelestiaDeepSkyCatalogExt[] = ".dsc";
static const char AVIExt[] = ".avi";
static const char DDSExt[] = ".dds";
static const char DXT5NormalMapExt[] = ".dxt5nm";
static const char CelestiaLegacyScriptExt[] = ".cel";
static const char CelestiaScriptExt[] = ".clx";
static const char CelestiaScriptExt2[] = ".celx";
static const char CelestiaModelExt[] = ".cmod";
static const char CelestiaParticleSystemExt[] = ".cpart";
static const char CelestiaXYZTrajectoryExt[] = ".xyz";
static const char CelestiaXYZVTrajectoryExt[] = ".xyzv";
static const char ContentXYZVBinaryExt[] = ".xyzvbin";
static const char ContentWarpMeshExt[] = ".map";

ContentType DetermineFileType(const fs::path& filename)
{
    const string ext = filename.extension().string();

    if (compareIgnoringCase(JPEGExt, ext) == 0 ||
        compareIgnoringCase(JPGExt, ext) == 0 ||
        compareIgnoringCase(JFIFExt, ext) == 0)
        return Content_JPEG;
    if (compareIgnoringCase(BMPExt, ext) == 0)
        return Content_BMP;
    if (compareIgnoringCase(TargaExt, ext) == 0)
        return Content_Targa;
    if (compareIgnoringCase(PNGExt, ext) == 0)
        return Content_PNG;
    if (compareIgnoringCase(ThreeDSExt, ext) == 0)
        return Content_3DStudio;
    if (compareIgnoringCase(CelestiaTextureExt, ext) == 0)
        return Content_CelestiaTexture;
    if (compareIgnoringCase(CelestiaMeshExt, ext) == 0)
        return Content_CelestiaMesh;
    if (compareIgnoringCase(CelestiaCatalogExt, ext) == 0)
        return Content_CelestiaCatalog;
    if (compareIgnoringCase(CelestiaStarCatalogExt, ext) == 0)
        return Content_CelestiaStarCatalog;
    if (compareIgnoringCase(CelestiaDeepSkyCatalogExt, ext) == 0)
        return Content_CelestiaDeepSkyCatalog;
    if (compareIgnoringCase(AVIExt, ext) == 0)
        return Content_AVI;
    if (compareIgnoringCase(DDSExt, ext) == 0)
        return Content_DDS;
    if (compareIgnoringCase(CelestiaLegacyScriptExt, ext) == 0)
        return Content_CelestiaLegacyScript;
    if (compareIgnoringCase(CelestiaScriptExt, ext) == 0 ||
        compareIgnoringCase(CelestiaScriptExt2, ext) == 0)
        return Content_CelestiaScript;
    if (compareIgnoringCase(CelestiaModelExt, ext) == 0)
        return Content_CelestiaModel;
    if (compareIgnoringCase(CelestiaParticleSystemExt, ext) == 0)
        return Content_CelestiaParticleSystem;
    if (compareIgnoringCase(DXT5NormalMapExt, ext) == 0)
        return Content_DXT5NormalMap;
    if (compareIgnoringCase(CelestiaXYZTrajectoryExt, ext) == 0)
        return Content_CelestiaXYZTrajectory;
    if (compareIgnoringCase(CelestiaXYZVTrajectoryExt, ext) == 0)
        return Content_CelestiaXYZVTrajectory;
    if (compareIgnoringCase(ContentWarpMeshExt, ext) == 0)
        return Content_WarpMesh;
    if (compareIgnoringCase(ContentXYZVBinaryExt, ext) == 0)
        return Content_CelestiaXYZVBinary;
    return Content_Unknown;
}
