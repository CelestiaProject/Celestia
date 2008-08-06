// filetype.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cctype>
#include <cstdlib>
#include "util.h"
#include "filetype.h"

using namespace std;


static const string JPEGExt(".jpeg");
static const string JPGExt(".jpg");
static const string JFIFExt(".jif");
static const string BMPExt(".bmp");
static const string TargaExt(".tga");
static const string PNGExt(".png");
static const string ThreeDSExt(".3ds");
static const string CelestiaTextureExt(".ctx");
static const string CelestiaMeshExt(".cms");
static const string CelestiaCatalogExt(".ssc");
static const string CelestiaStarCatalogExt(".stc");
static const string CelestiaDeepSkyCatalogExt(".dsc");
static const string AVIExt(".avi");
static const string DDSExt(".dds");
static const string DXT5NormalMapExt(".dxt5nm");
static const string CelestiaLegacyScriptExt(".cel");
static const string CelestiaScriptExt(".clx");
static const string CelestiaScriptExt2(".celx");
static const string CelestiaModelExt(".cmod");
static const string CelestiaParticleSystemExt(".cpart");
static const string CelestiaXYZTrajectoryExt(".xyz");
static const string CelestiaXYZVTrajectoryExt(".xyzv");

ContentType DetermineFileType(const string& filename)
{
    int extPos = filename.rfind('.');
    if (extPos == (int)string::npos)
        return Content_Unknown;
    string ext = string(filename, extPos, filename.length() - extPos + 1);

    if (compareIgnoringCase(JPEGExt, ext) == 0 ||
        compareIgnoringCase(JPGExt, ext) == 0 ||
        compareIgnoringCase(JFIFExt, ext) == 0)
        return Content_JPEG;
    else if (compareIgnoringCase(BMPExt, ext) == 0)
        return Content_BMP;
    else if (compareIgnoringCase(TargaExt, ext) == 0)
        return Content_Targa;
    else if (compareIgnoringCase(PNGExt, ext) == 0)
        return Content_PNG;
    else if (compareIgnoringCase(ThreeDSExt, ext) == 0)
        return Content_3DStudio;
    else if (compareIgnoringCase(CelestiaTextureExt, ext) == 0)
        return Content_CelestiaTexture;
    else if (compareIgnoringCase(CelestiaMeshExt, ext) == 0)
        return Content_CelestiaMesh;
    else if (compareIgnoringCase(CelestiaCatalogExt, ext) == 0)
        return Content_CelestiaCatalog;
    else if (compareIgnoringCase(CelestiaStarCatalogExt, ext) == 0)
        return Content_CelestiaStarCatalog;
    else if (compareIgnoringCase(CelestiaDeepSkyCatalogExt, ext) == 0)
        return Content_CelestiaDeepSkyCatalog;
    else if (compareIgnoringCase(AVIExt, ext) == 0)
        return Content_AVI;
    else if (compareIgnoringCase(DDSExt, ext) == 0)
        return Content_DDS;
    else if (compareIgnoringCase(CelestiaLegacyScriptExt, ext) == 0)
        return Content_CelestiaLegacyScript;
    else if (compareIgnoringCase(CelestiaScriptExt, ext) == 0 ||
             compareIgnoringCase(CelestiaScriptExt2, ext) == 0)
        return Content_CelestiaScript;
    else if (compareIgnoringCase(CelestiaModelExt, ext) == 0)
        return Content_CelestiaModel;
    else if (compareIgnoringCase(CelestiaParticleSystemExt, ext) == 0)
        return Content_CelestiaParticleSystem;
    else if (compareIgnoringCase(DXT5NormalMapExt, ext) == 0)
        return Content_DXT5NormalMap;
    else if (compareIgnoringCase(CelestiaXYZTrajectoryExt, ext) == 0)
        return Content_CelestiaXYZTrajectory;
    else if (compareIgnoringCase(CelestiaXYZVTrajectoryExt, ext) == 0)
        return Content_CelestiaXYZVTrajectory;
    else
        return Content_Unknown;
}
