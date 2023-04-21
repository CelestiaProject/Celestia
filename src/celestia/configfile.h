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
#include <celengine/parser.h>
#include <celengine/star.h>
#include <celengine/value.h>
#include <celcompat/filesystem.h>


class CelestiaConfig
{
public:
    fs::path starDatabaseFile;
    fs::path starNamesFile;
    std::vector<fs::path> solarSystemFiles;
    std::vector<fs::path> starCatalogFiles;
    std::vector<fs::path> dsoCatalogFiles;
    std::vector<fs::path> extrasDirs;
    std::vector<fs::path> skipExtras;
    fs::path deepSkyCatalog;
    fs::path asterismsFile;
    fs::path boundariesFile;
    float faintestVisible;
    fs::path favoritesFile;
    fs::path initScriptFile;
    fs::path demoScriptFile;
    fs::path destinationsFile;
    std::string mainFont;
    std::string labelFont;
    std::string titleFont;
    fs::path logoTextureFile;
    std::string cursor;
    std::vector<std::string> ignoreGLExtensions;
    float rotateAcceleration;
    float mouseRotationSensitivity;
    bool  reverseMouseWheel;
    double orbitWindowEnd;
    double orbitPeriodsShown;
    double linearFadeFraction;
    fs::path scriptScreenshotDirectory;
    std::string scriptSystemAccessPolicy;
#ifdef CELX
    fs::path luaHook;
    Value configParams;
#endif

    fs::path HDCrossIndexFile;
    fs::path SAOCrossIndexFile;
    fs::path GlieseCrossIndexFile;

    StarDetails::StarTextureSet starTextures;

    // Renderer detail options
    unsigned int shadowTextureSize;
    unsigned int eclipseTextureSize;
    unsigned int orbitPathSamplePoints;

    unsigned int aaSamples;

    unsigned int consoleLogRows;

    float SolarSystemMaxDistance;
    unsigned ShadowMapSize;

    std::string projectionMode;
    std::string viewportEffect;
    std::string warpMeshFile;
    std::string measurementSystem;
    std::string temperatureScale;

    std::string x264EncoderOptions;
    std::string ffvhEncoderOptions;

    fs::path leapSecondsFile;

    std::string layoutDirection;
};

CelestiaConfig* ReadCelestiaConfig(const fs::path& filename, CelestiaConfig* config = nullptr);
