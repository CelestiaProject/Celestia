// filetype.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cctype>
#include "filetype.h"

using namespace std;


static const string JPEGExt(".jpg");
static const string BMPExt(".bmp");
static const string TargaExt(".tga");
static const string ThreeDSExt(".3ds");
static const string CelestiaTextureExt(".ctx");
static const string CelestiaMeshExt(".cms");


static int compareIgnoringCase(const string& s1, const string& s2)
{
    string::const_iterator i1 = s1.begin();
    string::const_iterator i2 = s2.begin();

    while (i1 != s1.end() && i2 != s2.end())
    {
        if (toupper(*i1) != toupper(*i2))
            return (toupper(*i1) < toupper(*i2)) ? -1 : 1;
        ++i1;
        ++i2;
    }

    return s2.size() - s1.size();
}


ContentType DetermineFileType(const string& filename)
{
    int extPos = filename.length() - 4;
    if (extPos < 0)
        extPos = 0;
    string ext = string(filename, extPos, 4);

    if (compareIgnoringCase(JPEGExt, ext) == 0)
        return Content_JPEG;
    else if (compareIgnoringCase(BMPExt, ext) == 0)
        return Content_BMP;
    else if (compareIgnoringCase(TargaExt, ext) == 0)
        return Content_Targa;
    else if (compareIgnoringCase(ThreeDSExt, ext) == 0)
        return Content_3DStudio;
    else if (compareIgnoringCase(CelestiaTextureExt, ext) == 0)
        return Content_CelestiaTexture;
    else if (compareIgnoringCase(CelestiaMeshExt, ext) == 0)
        return Content_CelestiaMesh;
    else
        return Content_Unknown;
}
