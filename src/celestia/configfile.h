// configfile.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string>
#include <vector>

#include <celcompat/filesystem.h>
#include <celengine/star.h>
#ifdef CELX
#include <celengine/value.h>
#endif


struct CelestiaConfig
{
    struct Paths
    {
        fs::path starDatabaseFile{ };
        fs::path starNamesFile{ };
        std::vector<fs::path> solarSystemFiles{ };
        std::vector<fs::path> starCatalogFiles{ };
        std::vector<fs::path> dsoCatalogFiles{ };
        std::vector<fs::path> extrasDirs{ };
        std::vector<fs::path> skipExtras{ };
        fs::path asterismsFile{ };
        fs::path boundariesFile{ };
        fs::path favoritesFile{ };
        fs::path initScriptFile{ };
        fs::path demoScriptFile{ };
        fs::path destinationsFile{ };
        fs::path HDCrossIndexFile{ };
        fs::path SAOCrossIndexFile{ };
        fs::path warpMeshFile{ };
        fs::path leapSecondsFile{ };
#ifdef CELX
        fs::path scriptScreenshotDirectory{ };
        fs::path luaHook{ };
#endif
    };

    struct Fonts
    {
        fs::path mainFont{ };
        fs::path labelFont{ };
        fs::path titleFont{ };
    };

    struct Mouse
    {
        std::string cursor{ };
        float rotateAcceleration{ 120.0f };
        float rotationSensitivity{ 1.0f };
        bool reverseWheel{ false };
        bool rayBasedDragging{ false };
        bool focusZooming{ false };
    };

    struct RenderDetails
    {
        double orbitWindowEnd{ 0.5 };
        double orbitPeriodsShown{ 1.0 };
        double linearFadeFraction{ 0.0 };
        float faintestVisible{ 6.0f };
        unsigned int shadowTextureSize{ 256 };
        unsigned int eclipseTextureSize{ 128 };
        unsigned int orbitPathSamplePoints{ 100 };
        unsigned int aaSamples{ 1 };
        float SolarSystemMaxDistance{ 1.0f };
        unsigned int ShadowMapSize{ 0 };
        std::vector<std::string> ignoreGLExtensions{ };
    };

    CelestiaConfig() = default;
    ~CelestiaConfig() = default;
    CelestiaConfig(const CelestiaConfig&) = delete;
    CelestiaConfig& operator=(const CelestiaConfig&) = delete;
    CelestiaConfig(CelestiaConfig&&) = default;
    CelestiaConfig& operator=(CelestiaConfig&&) = default;

    Paths paths{ };
    Fonts fonts{ };
    Mouse mouse{ };
    RenderDetails renderDetails{ };
    StarDetails::StarTextureSet starTextures{ };

    std::string scriptSystemAccessPolicy{ };

    unsigned int consoleLogRows{ 200 };

    std::string projectionMode{ };
    std::string viewportEffect{ };
    std::string measurementSystem{ };
    std::string temperatureScale{ };

    std::string x264EncoderOptions{ };
    std::string ffvhEncoderOptions{ };

    std::string layoutDirection{ };

#ifdef CELX
    Value configParams{ };
#endif
};

bool ReadCelestiaConfig(const fs::path& filename, CelestiaConfig& config);
