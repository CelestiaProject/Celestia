// aboutdialog.cpp
//
// Copyright (C) 2025-present, the Celestia Development Team
//
// Based on the about dialog in the Qt interface
// Copyright (C) 2005-2008, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "aboutdialog.h"

#include "config.h"

#include <algorithm>
#include <csetjmp>
#include <string>
#include <type_traits>
#include <utility>

#include <celengine/glsupport.h>

#include <boost/version.hpp>
#include <Eigen/Core>
#include <fmt/format.h>
#include <freetype/freetype.h>
#include <imgui.h>
#include <jpeglib.h>
#include <png.h>
#include <SDL_version.h>

#ifdef CELX
#include <lua.hpp>
#endif

#ifdef HAVE_MESHOPTIMIZER
#include <meshoptimizer.h>
#endif

#ifdef USE_FFMPEG
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}
#endif

#ifdef USE_ICU
#include <celutil/includeicu.h>
#endif

#ifdef USE_LIBAVIF
#include <avif/avif.h>
#endif

#ifdef USE_MINIAUDIO
#include <miniaudio.h>
#endif

#ifdef USE_SPICE
#include <SpiceUsr.h>
#endif

#include <celutil/stringutils.h>

namespace celestia::sdl
{

namespace
{

std::string
getFreeTypeVersion()
{
    FT_Library ftLib = nullptr;
    if (FT_Init_FreeType(&ftLib) != 0)
        return {};

    FT_Int majorVersion;
    FT_Int minorVersion;
    FT_Int patchVersion;
    FT_Library_Version(ftLib, &majorVersion, &minorVersion, &patchVersion);
    FT_Done_FreeType(ftLib);
    return fmt::format("{}.{}.{}", majorVersion, minorVersion, patchVersion);
}

std::string
getSDLVersion()
{
    SDL_version version;
    SDL_GetVersion(&version);
    return fmt::format("{}.{}.{}", version.major, version.minor, version.patch);
}

#ifdef CELX
std::string
getLuaLibraryName()
{
#ifdef LUAJIT_VERSION
    return "luajit";
#else
    return "lua";
#endif
}

std::string
getLuaVersion()
{
#ifdef LUAJIT_VERSION
    std::string result = LUAJIT_VERSION;
    if (auto pos = result.rfind(' '); pos != std::string::npos)
        result.erase(0, pos + 1);
    return result;
#else
    // MacOS seems to have a non-standard return type for lua_version
    // So just use the static version
    return fmt::format("{}.{}", LUA_VERSION_NUM / 100, LUA_VERSION_NUM % 100);
#endif
}
#endif

#ifdef USE_FFMPEG
std::string
getFFMpegVersion(unsigned int version)
{
    constexpr unsigned int mask = (1U << 8) - 1U;
    return fmt::format("{}.{}.{}", version >> 16, (version >> 8) & mask, version & mask);
}
#endif

#ifdef USE_MINIAUDIO
std::string
getMiniaudioVersion()
{
    ma_uint32 major;
    ma_uint32 minor;
    ma_uint32 revision;
    ma_version(&major, &minor, &revision);
    return fmt::format("{}.{}.{}", major, minor, revision);
}
#endif

#ifdef USE_ICU
std::string
getICUVersion()
{
    UVersionInfo versionInfo;
    u_getVersion(versionInfo);
    return fmt::format("{}.{}.{}.{}", +versionInfo[0], +versionInfo[1], +versionInfo[2], +versionInfo[3]);
}
#endif

#ifdef USE_SPICE
std::string
getSpiceVersion()
{
    const char* spiceVersion = tkvrsn_c("toolkit");
    if (spiceVersion == nullptr)
        return {};

    std::string spiceVersionStr{ spiceVersion };
    if (auto pos = spiceVersionStr.rfind('_'); pos != std::string::npos)
        spiceVersionStr.erase(0, pos + 1);

    return spiceVersionStr;
}
#endif

} // end unnamed namespace

struct AboutDialog::LibraryInfo
{
    LibraryInfo(std::string&&, std::string&&, std::string&&);

    template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
    LibraryInfo(std::string&& _name, std::string&& _license, T major, T minor) :
        LibraryInfo(std::move(_name), std::move(_license), fmt::format("{}.{}", major, minor))
    {
    }

    template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
    LibraryInfo(std::string&& _name, std::string&& _license, T major, T minor, T patch) :
        LibraryInfo(std::move(_name), std::move(_license), fmt::format("{}.{}.{}", major, minor, patch))
    {
    }

    std::string name;
    std::string license;
    std::string version;
};

AboutDialog::LibraryInfo::LibraryInfo(std::string&& _name,
                                      std::string&& _license,
                                      std::string&& _version) :
    name(std::move(_name)),
    license(std::move(_license)),
    version(std::move(_version))
{
}

AboutDialog::AboutDialog()
{
    // We try to use runtime versions here in case of dynamic linkage with different
    // versions of the library from the one used at compile time, but some libraries
    // do not support this, so we use macro constants.

    m_libraries.emplace_back("boost", "BSL-1.0", BOOST_VERSION / 100000, (BOOST_VERSION / 100) % 1000, BOOST_VERSION % 100);
    m_libraries.emplace_back("eigen", "MPL-2.0", EIGEN_WORLD_VERSION, EIGEN_MAJOR_VERSION, EIGEN_MINOR_VERSION);
    m_libraries.emplace_back("fmt", "MIT", FMT_VERSION / 10000, (FMT_VERSION / 100) % 100, FMT_VERSION % 100);
    m_libraries.emplace_back("freetype", "FTL or GLPv2", getFreeTypeVersion());
    m_libraries.emplace_back("Dear Imgui", "MIT", ImGui::GetVersion());
    m_libraries.emplace_back("libepoxy", "MIT", std::string{});
    m_libraries.emplace_back("libjpeg", "IJG", JPEG_LIB_VERSION / 10, JPEG_LIB_VERSION % 10);
    m_libraries.emplace_back("libpng", "Libpng", PNG_LIBPNG_VER_STRING);
    m_libraries.emplace_back("r128", "Unlicense", "1.6.0"); // should match r128.h
    m_libraries.emplace_back("SDL", "Zlib", getSDLVersion());

#ifdef CELX
    m_libraries.emplace_back(getLuaLibraryName(), "MIT", getLuaVersion());
#endif
#ifdef ENABLE_NLS
    m_libraries.emplace_back("gettext", "LGPL-2.1", std::string{});
#endif
#ifdef HAVE_MESHOPTIMIZER
    m_libraries.emplace_back("meshoptimizer", "MIT", MESHOPTIMIZER_VERSION / 1000, (MESHOPTIMIZER_VERSION / 10) % 100, MESHOPTIMIZER_VERSION % 10);
#endif
#ifdef USE_FFMPEG
    m_libraries.emplace_back("libavcodec", avcodec_license(), getFFMpegVersion(avcodec_version()));
    m_libraries.emplace_back("libavformat", avformat_license(), getFFMpegVersion(avformat_version()));
    m_libraries.emplace_back("libavutil", avutil_license(), getFFMpegVersion(avutil_version()));
    m_libraries.emplace_back("libswscale", swscale_license(), getFFMpegVersion(swscale_version()));
#endif
#ifdef USE_ICU
    m_libraries.emplace_back("icu", "ICU", getICUVersion());
#endif
#ifdef USE_LIBAVIF
    m_libraries.emplace_back("libavif", "BSD", AVIF_VERSION_MAJOR, AVIF_VERSION_MINOR, AVIF_VERSION_PATCH);
#endif
#ifdef USE_MINIAUDIO
    m_libraries.emplace_back("miniaudio", "Unlicense OR MIT-0", getMiniaudioVersion());
#endif
#ifdef USE_SPICE
    m_libraries.emplace_back("cspice", "SPICE", getSpiceVersion());
#endif

    std::sort(m_libraries.begin(), m_libraries.end(),
              [](const auto& a, const auto& b)
              {
                  return compareIgnoringCase(a.name, b.name) < 0;
              });
}

AboutDialog::~AboutDialog() = default;

void
AboutDialog::show(bool* isOpen) const
{
    if (!*isOpen)
        return;

    if (ImGui::Begin("About Celestia", isOpen))
    {
        ImGui::Text("Celestia 1.7.0");
        ImGui::Text("Development snapshot, git commit %s", GIT_COMMIT);
        ImGui::Separator();
        ImGui::TextWrapped("Copyright (C) 2001-2025 by the Celestia Development Team.");
        ImGui::TextWrapped("Celestia is free software. You can redistribute it and/or modify "
                        "it under the terms of the GNU General Public License as published "
                        "by the Free Software Foundation; either version 2 of the License, "
                        "or (at your option) any later version.");
        ImGui::Separator();
        ImGui::Text("Third-party libraries");
        ImGui::BeginTable("libraryTable", 3);
        for (const auto& library : m_libraries)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s", library.name.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%s", library.version.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%s", library.license.c_str());
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

} // end namespace celestia::sdl
