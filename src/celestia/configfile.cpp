// configfile.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "configfile.h"

#include <config.h>
#include <fstream>
#include <type_traits>

#include <celengine/hash.h>
#include <celengine/parser.h>
#include <celengine/texmanager.h>
#include <celengine/value.h>
#include <celutil/fsutils.h>
#include <celutil/logger.h>
#include <celutil/tokenizer.h>


using namespace std::string_view_literals;
using celestia::util::GetLogger;


namespace
{

void
applyBoolean(bool& target, const Hash& hash, std::string_view key)
{
    auto b = hash.getBoolean(key);
    if (b.has_value())
        target = *b;
}


template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
void
applyNumber(T& target, const Hash& hash, std::string_view key)
{
    auto number = hash.getNumber<T>(key);
    if (number.has_value())
        target = *number;
}


void
applyString(std::string& target, const Hash& hash, std::string_view key)
{
    auto str = hash.getString(key);
    if (str != nullptr)
        target = *str;
}


void
applyPath(fs::path& target, const Hash& hash, std::string_view key)
{
    auto path = hash.getPath(key);
    if (path.has_value())
        target = *path;
}


void
applyTexture(MultiResTexture& target, const Hash& hash, std::string_view key)
{
    auto source = hash.getPath(key);
    if (source.has_value())
        target.setTexture(*source, "textures");
}


void
applyStringArray(std::vector<std::string>& target, const Hash& hash, std::string_view key)
{
    auto value = hash.getValue(key);
    if (value == nullptr)
        return;

    if (auto array = value->getArray(); array != nullptr)
    {
        for (const auto& item : *array)
        {
            auto itemStr = item.getString();
            if (itemStr == nullptr)
            {
                GetLogger()->error("Found non-string value in {} array.\n", key);
                break;
            }

            target.emplace_back(*itemStr);
        }
    }
    else
        GetLogger()->error("{} must be an array of strings.\n", key);
}


void
applyPathArray(std::vector<fs::path>& target, const Hash& hash, std::string_view key)
{
    auto value = hash.getValue(key);
    if (value == nullptr)
        return;

    if (auto array = value->getArray(); array != nullptr)
    {
        for (const auto& item : *array)
        {
            auto itemStr = item.getString();
            if (itemStr == nullptr)
            {
                GetLogger()->error("Found non-string value in {} array.\n", key);
                break;
            }

            target.emplace_back(celestia::util::PathExp(fs::u8path(*itemStr)));
        }
    }
    else if (auto str = value->getString(); str != nullptr)
        target.emplace_back(celestia::util::PathExp(fs::u8path(*str)));
    else
        GetLogger()->error("{} must be a string or an array of strings.\n", key);
}


void
applyPaths(CelestiaConfig::Paths& paths, const Hash& hash)
{
    applyPath(paths.starDatabaseFile, hash, "StarDatabase"sv);
    applyPath(paths.starNamesFile, hash, "StarNameDatabase"sv);
    applyPathArray(paths.solarSystemFiles, hash, "SolarSystemCatalogs"sv);
    applyPathArray(paths.starCatalogFiles, hash, "StarCatalogs"sv);
    applyPathArray(paths.dsoCatalogFiles, hash, "DeepSkyCatalogs"sv);
    applyPathArray(paths.extrasDirs, hash, "ExtrasDirectories"sv);
    applyPathArray(paths.skipExtras, hash, "SkipExtras"sv);
    applyPath(paths.asterismsFile, hash, "AsterismsFile"sv);
    applyPath(paths.boundariesFile, hash, "BoundariesFile"sv);
    applyPath(paths.favoritesFile, hash, "FavoritesFile"sv);
    applyPath(paths.initScriptFile, hash, "InitScript"sv);
    applyPath(paths.demoScriptFile, hash, "DemoScript"sv);
    applyPath(paths.destinationsFile, hash, "DestinationFile"sv);
    applyPath(paths.HDCrossIndexFile, hash, "HDCrossIndex"sv);
    applyPath(paths.SAOCrossIndexFile, hash, "SAOCrossIndex"sv);
    applyPath(paths.warpMeshFile, hash, "WarpMeshFile"sv);
    applyPath(paths.leapSecondsFile, hash, "LeapSecondsFile"sv);
#ifdef CELX
    applyPath(paths.scriptScreenshotDirectory, hash, "ScriptScreenshotDirectory"sv);
    applyPath(paths.luaHook, hash, "LuaHook"sv);
#endif
}


void
applyFonts(CelestiaConfig::Fonts& fonts, const Hash& hash)
{
    applyPath(fonts.mainFont, hash, "Font"sv);
    applyPath(fonts.labelFont, hash, "LabelFont"sv);
    applyPath(fonts.titleFont, hash, "TitleFont"sv);
}


void
applyMouse(CelestiaConfig::Mouse& mouse, const Hash& hash)
{
    applyString(mouse.cursor, hash, "Cursor"sv);
    applyNumber(mouse.rotateAcceleration, hash, "RotateAcceleration"sv);
    applyNumber(mouse.rotationSensitivity, hash, "MouseRotationSensitivity"sv);
    applyBoolean(mouse.reverseWheel, hash, "ReverseMouseWheel"sv);
    applyBoolean(mouse.rayBasedDragging, hash, "RayBasedDragging"sv);
    applyBoolean(mouse.focusZooming, hash, "FocusZooming"sv);
}


void
applyRenderDetails(CelestiaConfig::RenderDetails& renderDetails, const Hash& hash)
{
    applyNumber(renderDetails.orbitWindowEnd, hash, "OrbitWindowEnd"sv);
    applyNumber(renderDetails.orbitPeriodsShown, hash, "OrbitPeriodsShown"sv);
    applyNumber(renderDetails.linearFadeFraction, hash, "LinearFadeFraction"sv);
    applyNumber(renderDetails.faintestVisible, hash, "FaintestVisibleMagnitude"sv);
    applyNumber(renderDetails.shadowTextureSize, hash, "ShadowTextureSize"sv);
    applyNumber(renderDetails.eclipseTextureSize, hash, "EclipseTextureSize"sv);
    applyNumber(renderDetails.orbitPathSamplePoints, hash, "OrbitPathSamplePoints"sv);
    applyNumber(renderDetails.aaSamples, hash, "AntialiasingSamples"sv);
    applyNumber(renderDetails.SolarSystemMaxDistance, hash, "SolarSystemMaxDistance"sv);
    renderDetails.SolarSystemMaxDistance = std::clamp(renderDetails.SolarSystemMaxDistance, 1.0f, 10.0f);
    applyNumber(renderDetails.ShadowMapSize, hash, "ShadowMapSize"sv);
    applyStringArray(renderDetails.ignoreGLExtensions, hash, "IgnoreGLExtensions"sv);
}


void
applyStarTextures(StarDetails::StarTextureSet& starTextures, const Hash& hash, std::string_view key)
{
    const Value* starTexValue = hash.getValue(key);
    if (starTexValue == nullptr)
        return;

    const Hash* starTexTable = starTexValue->getHash();
    if (starTexTable == nullptr)
    {
        GetLogger()->error("{} must be a property list.\n", key);
        return;
    }

    applyTexture(starTextures.starTex[StellarClass::Spectral_O], *starTexTable, "O"sv);
    applyTexture(starTextures.starTex[StellarClass::Spectral_B], *starTexTable, "B"sv);
    applyTexture(starTextures.starTex[StellarClass::Spectral_A], *starTexTable, "A"sv);
    applyTexture(starTextures.starTex[StellarClass::Spectral_F], *starTexTable, "F"sv);
    applyTexture(starTextures.starTex[StellarClass::Spectral_G], *starTexTable, "G"sv);
    applyTexture(starTextures.starTex[StellarClass::Spectral_K], *starTexTable, "K"sv);
    applyTexture(starTextures.starTex[StellarClass::Spectral_M], *starTexTable, "M"sv);
    applyTexture(starTextures.starTex[StellarClass::Spectral_R], *starTexTable, "R"sv);
    applyTexture(starTextures.starTex[StellarClass::Spectral_S], *starTexTable, "S"sv);
    applyTexture(starTextures.starTex[StellarClass::Spectral_N], *starTexTable, "N"sv);
    applyTexture(starTextures.starTex[StellarClass::Spectral_WC], *starTexTable, "WC"sv);
    applyTexture(starTextures.starTex[StellarClass::Spectral_WN], *starTexTable, "WN"sv);
    applyTexture(starTextures.starTex[StellarClass::Spectral_WO], *starTexTable, "WO"sv);
    applyTexture(starTextures.starTex[StellarClass::Spectral_Unknown], *starTexTable, "Unknown"sv);
    applyTexture(starTextures.starTex[StellarClass::Spectral_L], *starTexTable, "L"sv);
    applyTexture(starTextures.starTex[StellarClass::Spectral_T], *starTexTable, "T"sv);
    applyTexture(starTextures.starTex[StellarClass::Spectral_Y], *starTexTable, "Y"sv);
    applyTexture(starTextures.starTex[StellarClass::Spectral_C], *starTexTable, "C"sv);
    // One texture for all white dwarf types; not sure if this needs to be
    // changed. White dwarfs vary widely in temperature, so texture choice
    // should probably be based on that instead of spectral type.
    applyTexture(starTextures.starTex[StellarClass::Spectral_D], *starTexTable, "WD"sv);
    applyTexture(starTextures.neutronStarTex, *starTexTable, "NeutronStar"sv);
    applyTexture(starTextures.defaultTex, *starTexTable, "Default"sv);
}

} // end unnamed namespace


bool ReadCelestiaConfig(const fs::path& filename, CelestiaConfig& config)
{
    std::ifstream configFile(filename);
    if (!configFile.good())
    {
        GetLogger()->error("Error opening config file '{}'.\n", filename);
        return false;
    }

    Tokenizer tokenizer(&configFile);
    Parser parser(&tokenizer);

    tokenizer.nextToken();
    if (auto tokenValue = tokenizer.getNameValue(); tokenValue != "Configuration")
    {
        GetLogger()->error("{}:{} 'Configuration' expected.\n", filename,
                           tokenizer.getLineNumber());
        return false;
    }

    Value configParamsValue = parser.readValue();
    const Hash* configParams = configParamsValue.getHash();
    if (configParams == nullptr)
    {
        GetLogger()->error("{}: Bad configuration file.\n", filename);
        return false;
    }

    applyPaths(config.paths, *configParams);
    applyFonts(config.fonts, *configParams);
    applyMouse(config.mouse, *configParams);
    applyRenderDetails(config.renderDetails, *configParams);
    applyStarTextures(config.starTextures, *configParams, "StarTextures"sv);

    applyString(config.projectionMode, *configParams, "ProjectionMode"sv);
    applyString(config.viewportEffect, *configParams, "ViewportEffect"sv);
    applyString(config.x264EncoderOptions, *configParams, "X264EncoderOptions"sv);
    applyString(config.ffvhEncoderOptions, *configParams, "FFVHEncoderOptions"sv);
    applyString(config.measurementSystem, *configParams, "MeasurementSystem"sv);
    applyString(config.temperatureScale, *configParams, "TemperatureScale"sv);
    applyString(config.layoutDirection, *configParams, "LayoutDirection"sv);
    applyString(config.scriptSystemAccessPolicy, *configParams, "ScriptSystemAccessPolicy"sv);

    applyNumber(config.consoleLogRows, *configParams, "LogSize"sv);

#ifdef CELX
    // Move the value into the config object to retain ownership of the hash
    config.configParams = std::move(configParamsValue);
#endif

    return true;
}
