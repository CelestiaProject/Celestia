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
static const string AVIExt(".avi");
static const string DDSExt(".dds");


ContentType DetermineFileType(const string& filename)
{
    int extPos = filename.length() - 4;
    if (extPos < 0)
        extPos = 0;
    string ext = string(filename, extPos, 4);

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
    else if (compareIgnoringCase(AVIExt, ext) == 0)
        return Content_AVI;
    else if (compareIgnoringCase(DDSExt, ext) == 0)
        return Content_DDS;
    else
        return Content_Unknown;
}
