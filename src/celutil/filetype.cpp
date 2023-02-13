// filetype.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "filetype.h"

#include <string>
#include <string_view>

#include "stringutils.h"

using namespace std::string_view_literals;

namespace
{

constexpr std::string_view JPEGExt = ".jpeg"sv;
constexpr std::string_view JPGExt = ".jpg"sv;
constexpr std::string_view JFIFExt = ".jif"sv;
constexpr std::string_view BMPExt = ".bmp"sv;
constexpr std::string_view TargaExt = ".tga"sv;
constexpr std::string_view PNGExt = ".png"sv;
#ifdef USE_LIBAVIF
constexpr std::string_view AVIFExt = ".avif"sv;
#endif
constexpr std::string_view ThreeDSExt = ".3ds"sv;
constexpr std::string_view CelestiaTextureExt = ".ctx"sv;
constexpr std::string_view CelestiaMeshExt = ".cms"sv;
constexpr std::string_view CelestiaCatalogExt = ".ssc"sv;
constexpr std::string_view CelestiaStarCatalogExt = ".stc"sv;
constexpr std::string_view CelestiaDeepSkyCatalogExt = ".dsc"sv;
constexpr std::string_view MKVExt = ".mkv"sv;
constexpr std::string_view DDSExt = ".dds"sv;
constexpr std::string_view DXT5NormalMapExt = ".dxt5nm"sv;
constexpr std::string_view CelestiaLegacyScriptExt = ".cel"sv;
constexpr std::string_view CelestiaScriptExt = ".clx"sv;
constexpr std::string_view CelestiaScriptExt2 = ".celx"sv;
constexpr std::string_view CelestiaModelExt = ".cmod"sv;
constexpr std::string_view CelestiaXYZTrajectoryExt = ".xyz"sv;
constexpr std::string_view CelestiaXYZVTrajectoryExt = ".xyzv"sv;
constexpr std::string_view ContentXYZVBinaryExt = ".xyzvbin"sv;
constexpr std::string_view ContentWarpMeshExt = ".map"sv;

} // end unnamed namespace

ContentType DetermineFileType(const fs::path& filename, bool isExtension)
{
    const std::string ext = isExtension
        ? filename.string()
        : filename.extension().string();

    if (compareIgnoringCase(JPEGExt, ext) == 0 ||
        compareIgnoringCase(JPGExt, ext) == 0 ||
        compareIgnoringCase(JFIFExt, ext) == 0)
        return ContentType::JPEG;
    if (compareIgnoringCase(BMPExt, ext) == 0)
        return ContentType::BMP;
    if (compareIgnoringCase(TargaExt, ext) == 0)
        return ContentType::Targa;
    if (compareIgnoringCase(PNGExt, ext) == 0)
        return ContentType::PNG;
#ifdef USE_LIBAVIF
    if (compareIgnoringCase(AVIFExt, ext) == 0)
        return ContentType::AVIF;
#endif
    if (compareIgnoringCase(ThreeDSExt, ext) == 0)
        return ContentType::_3DStudio;
    if (compareIgnoringCase(CelestiaTextureExt, ext) == 0)
        return ContentType::CelestiaTexture;
    if (compareIgnoringCase(CelestiaMeshExt, ext) == 0)
        return ContentType::CelestiaMesh;
    if (compareIgnoringCase(CelestiaCatalogExt, ext) == 0)
        return ContentType::CelestiaCatalog;
    if (compareIgnoringCase(CelestiaStarCatalogExt, ext) == 0)
        return ContentType::CelestiaStarCatalog;
    if (compareIgnoringCase(CelestiaDeepSkyCatalogExt, ext) == 0)
        return ContentType::CelestiaDeepSkyCatalog;
    if (compareIgnoringCase(MKVExt, ext) == 0)
        return ContentType::MKV;
    if (compareIgnoringCase(DDSExt, ext) == 0)
        return ContentType::DDS;
    if (compareIgnoringCase(CelestiaLegacyScriptExt, ext) == 0)
        return ContentType::CelestiaLegacyScript;
    if (compareIgnoringCase(CelestiaScriptExt, ext) == 0 ||
        compareIgnoringCase(CelestiaScriptExt2, ext) == 0)
        return ContentType::CelestiaScript;
    if (compareIgnoringCase(CelestiaModelExt, ext) == 0)
        return ContentType::CelestiaModel;
    if (compareIgnoringCase(DXT5NormalMapExt, ext) == 0)
        return ContentType::DXT5NormalMap;
    if (compareIgnoringCase(CelestiaXYZTrajectoryExt, ext) == 0)
        return ContentType::CelestiaXYZTrajectory;
    if (compareIgnoringCase(CelestiaXYZVTrajectoryExt, ext) == 0)
        return ContentType::CelestiaXYZVTrajectory;
    if (compareIgnoringCase(ContentWarpMeshExt, ext) == 0)
        return ContentType::WarpMesh;
    if (compareIgnoringCase(ContentXYZVBinaryExt, ext) == 0)
        return ContentType::CelestiaXYZVBinary;
    return ContentType::Unknown;
}
