// configfile.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <celengine/star.h>
#ifdef CELX
#include <celutil/associativearray.h>
#endif


struct CelestiaConfig
{
    struct Paths
    {
        std::filesystem::path starDatabaseFile{ };
        std::filesystem::path starNamesFile{ };
        std::vector<std::filesystem::path> solarSystemFiles{ };
        std::vector<std::filesystem::path> starCatalogFiles{ };
        std::vector<std::filesystem::path> dsoCatalogFiles{ };
        std::vector<std::filesystem::path> extrasDirs{ };
        std::vector<std::filesystem::path> skipExtras{ };
        std::filesystem::path asterismsFile{ };
        std::filesystem::path boundariesFile{ };
        std::filesystem::path favoritesFile{ };
        std::filesystem::path initScriptFile{ };
        std::filesystem::path demoScriptFile{ };
        std::filesystem::path destinationsFile{ };
        std::filesystem::path HDCrossIndexFile{ };
        std::filesystem::path SAOCrossIndexFile{ };
        std::filesystem::path warpMeshFile{ };
        std::filesystem::path leapSecondsFile{ };
#ifdef CELX
        std::filesystem::path scriptScreenshotDirectory{ };
        std::filesystem::path luaHook{ };
#endif
    };

    struct Fonts
    {
        std::filesystem::path mainFont{ };
        std::filesystem::path labelFont{ };
        std::filesystem::path titleFont{ };
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

    struct Observer
    {
        bool alignCameraToSurfaceOnLand{ false };
    };

    struct RenderDetails
    {
        double orbitWindowEnd{ 0.5 };
        double orbitPeriodsShown{ 1.0 };
        double linearFadeFraction{ 0.0 };
        float faintestVisible{ 6.0f };

        float renderAsterismsFadeStartDist{ 600.0f };
        float renderAsterismsFadeEndDist{ 6.52e4f };
        float renderBoundariesFadeStartDist{ 6.0f };
        float renderBoundariesFadeEndDist{ 20.0f };
        float labelConstellationsFadeStartDist{ 6.0f };
        float labelConstellationsFadeEndDist{ 20.0f };

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
    Observer observer{ };
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
    celestia::util::Value configParams{ };
#endif
};

bool ReadCelestiaConfig(const std::filesystem::path& filename, CelestiaConfig& config);
