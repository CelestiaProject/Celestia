// configfile.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <config.h>
#include <iostream>
#include <fstream>
#include <cassert>
#include <celutil/logger.h>
#include <celutil/fsutils.h>
#include <celengine/texmanager.h>
#include <celutil/tokenizer.h>
#include "configfile.h"

using namespace std;
using namespace celestia::util;


CelestiaConfig* ReadCelestiaConfig(const fs::path& filename, CelestiaConfig *config)
{
    ifstream configFile(filename);
    if (!configFile.good())
    {
        GetLogger()->error("Error opening config file '{}'.\n", filename);
        return config;
    }

    Tokenizer tokenizer(&configFile);
    Parser parser(&tokenizer);

    tokenizer.nextToken();
    if (auto tokenValue = tokenizer.getNameValue(); tokenValue != "Configuration")
    {
        GetLogger()->error("{}:{} 'Configuration' expected.\n", filename,
                           tokenizer.getLineNumber());
        return config;
    }

    const Value configParamsValue = parser.readValue();
    const Hash* configParams = configParamsValue.getHash();
    if (configParams == nullptr)
    {
        GetLogger()->error("{}: Bad configuration file.\n", filename);
        return config;
    }

    if (config == nullptr)
        config = new CelestiaConfig();

#ifdef CELX
    config->configParams = configParams;
    if (auto path = configParams->getPath("LuaHook"); path.has_value())
        config->luaHook = *path;
#endif

    config->faintestVisible = configParams->getNumber<float>("FaintestVisibleMagnitude").value_or(6.0f);
    if (auto path = configParams->getPath("FavoritesFile"); path.has_value())
        config->favoritesFile = *path;
    if (auto path = configParams->getPath("DestinationFile"); path.has_value())
        config->destinationsFile = *path;
    if (auto path = configParams->getPath("InitScript"); path.has_value())
        config->initScriptFile = *path;
    if (auto path = configParams->getPath("DemoScript"); path.has_value())
        config->demoScriptFile = *path;
    if (auto path = configParams->getPath("AsterismsFile"); path.has_value())
        config->asterismsFile = *path;
    if (auto path = configParams->getPath("BoundariesFile"); path.has_value())
        config->boundariesFile = *path;
    if (auto path = configParams->getPath("StarDatabase"); path.has_value())
        config->starDatabaseFile = *path;
    if (auto path = configParams->getPath("StarNameDatabase"); path.has_value())
        config->starNamesFile = *path;
    if (auto path = configParams->getPath("HDCrossIndex"); path.has_value())
        config->HDCrossIndexFile = *path;
    if (auto path = configParams->getPath("SAOCrossIndex"); path.has_value())
        config->SAOCrossIndexFile = *path;
    if (auto path = configParams->getPath("GlieseCrossIndex"); path.has_value())
        config->GlieseCrossIndexFile = *path;
    if (auto path = configParams->getPath("LeapSecondsFile"); path.has_value())
        config->leapSecondsFile = *path;
    if (const std::string* font = configParams->getString("Font"); font != nullptr)
        config->mainFont = *font;
    if (const std::string* labelFont = configParams->getString("LabelFont"); labelFont != nullptr)
        config->labelFont = *labelFont;
    if (const std::string* titleFont = configParams->getString("TitleFont"); titleFont != nullptr)
        config->titleFont = *titleFont;
    if (const std::string* logoTextureFile = configParams->getString("LogoTexture"); logoTextureFile != nullptr)
        config->logoTextureFile = *logoTextureFile;
    if (const std::string* cursor = configParams->getString("Cursor"); cursor != nullptr)
        config->cursor = *cursor;
    if (const std::string* projectionMode = configParams->getString("ProjectionMode"); projectionMode != nullptr)
        config->projectionMode = *projectionMode;
    if (const std::string* viewportEffect = configParams->getString("ViewportEffect"); viewportEffect != nullptr)
        config->viewportEffect = *viewportEffect;
    if (const std::string* warpMeshFile = configParams->getString("WarpMeshFile"); warpMeshFile != nullptr)
        config->warpMeshFile = *warpMeshFile;
    if (const std::string* x264EncoderOptions = configParams->getString("X264EncoderOptions"); x264EncoderOptions != nullptr)
        config->x264EncoderOptions = *x264EncoderOptions;
    if (const std::string* ffvhEncoderOptions = configParams->getString("FFVHEncoderOptions"); ffvhEncoderOptions != nullptr)
        config->ffvhEncoderOptions = *ffvhEncoderOptions;
    if (const std::string* measurementSystem = configParams->getString("MeasurementSystem"); measurementSystem != nullptr)
        config->measurementSystem = *measurementSystem;
    if (const std::string* temperatureScale = configParams->getString("TemperatureScale"); temperatureScale != nullptr)
        config->temperatureScale = *temperatureScale;

    auto maxDist = configParams->getNumber<float>("SolarSystemMaxDistance").value_or(1.0f);
    config->SolarSystemMaxDistance = std::clamp(maxDist, 1.0f, 10.0f);

    config->ShadowMapSize = configParams->getNumber<unsigned int>("ShadowMapSize").value_or(0u);

    config->aaSamples = configParams->getNumber<unsigned int>("AntialiasingSamples").value_or(1u);

    config->rotateAcceleration = configParams->getNumber<float>("RotateAcceleration").value_or(120.0f);
    config->mouseRotationSensitivity = configParams->getNumber<float>("MouseRotationSensitivity").value_or(1.0f);
    config->reverseMouseWheel = configParams->getBoolean("ReverseMouseWheel").value_or(false);
    if (auto path = configParams->getPath("ScriptScreenshotDirectory"); path.has_value())
        config->scriptScreenshotDirectory = *path;
    config->scriptSystemAccessPolicy = "ask";
    if (const std::string* scriptSystemAccessPolicy = configParams->getString("ScriptSystemAccessPolicy"); scriptSystemAccessPolicy != nullptr)
        config->scriptSystemAccessPolicy = *scriptSystemAccessPolicy;

    config->orbitWindowEnd = configParams->getNumber<float>("OrbitWindowEnd").value_or(0.5f);
    config->orbitPeriodsShown = configParams->getNumber<float>("OrbitPeriodsShown").value_or(1.0f);
    config->linearFadeFraction = configParams->getNumber<float>("LinearFadeFraction").value_or(0.0f);

    config->orbitPathSamplePoints = configParams->getNumber<unsigned int>("OrbitPathSamplePoints").value_or(100u);
    config->shadowTextureSize = configParams->getNumber<unsigned int>("ShadowTextureSize").value_or(256u);
    config->eclipseTextureSize = configParams->getNumber<unsigned int>("EclipseTextureSize").value_or(128u);

    config->consoleLogRows = configParams->getNumber<unsigned int>("LogSize").value_or(200u);

    if (const Value* solarSystemsVal = configParams->getValue("SolarSystemCatalogs"); solarSystemsVal != nullptr)
    {
        if (const ValueArray* solarSystems = solarSystemsVal->getArray(); solarSystems == nullptr)
        {
            GetLogger()->error("{}: SolarSystemCatalogs must be an array.\n", filename);
        }
        else
        {
            for (const auto& catalogNameVal : *solarSystems)
            {
                const std::string* catalogName = catalogNameVal.getString();
                if (catalogName == nullptr)
                {
                    GetLogger()->error("{}: Solar system catalog name must be a string.\n", filename);
                    continue;
                }

                config->solarSystemFiles.push_back(PathExp(*catalogName));
            }
        }
    }

    if (const Value* starCatalogsVal = configParams->getValue("StarCatalogs"); starCatalogsVal != nullptr)
    {
        if (const ValueArray* starCatalogs = starCatalogsVal->getArray(); starCatalogs == nullptr)
        {
            GetLogger()->error("{}: StarCatalogs must be an array.\n", filename);
        }
        else
        {
            for (const auto& catalogNameVal : *starCatalogs)
            {
                const std::string* catalogName = catalogNameVal.getString();
                if (catalogName == nullptr)
                {
                    GetLogger()->error("{}: Star catalog name must be a string.\n", filename);
                    continue;
                }

                config->starCatalogFiles.push_back(PathExp(*catalogName));
            }
        }
    }

    if (const Value* dsoCatalogsVal = configParams->getValue("DeepSkyCatalogs"); dsoCatalogsVal != nullptr)
    {
        if (const ValueArray* dsoCatalogs = dsoCatalogsVal->getArray(); dsoCatalogs == nullptr)
        {
            GetLogger()->error("{}: DeepSkyCatalogs must be an array.\n", filename);
        }
        else
        {
            for (const auto& catalogNameVal : *dsoCatalogs)
            {
                const std::string* catalogName = catalogNameVal.getString();
                if (catalogName == nullptr)
                {
                    GetLogger()->error("{}: DeepSky catalog name must be a string.\n", filename);
                    continue;
                }

                config->dsoCatalogFiles.push_back(PathExp(*catalogName));
            }
        }
    }

    if (const Value* extrasDirsVal = configParams->getValue("ExtrasDirectories"); extrasDirsVal != nullptr)
    {
        if (const ValueArray* extrasDirs = extrasDirsVal->getArray(); extrasDirs != nullptr)
        {
            for (const auto& dirNameVal : *extrasDirs)
            {
                const std::string* dirName = dirNameVal.getString();
                if (dirName == nullptr)
                {
                    GetLogger()->error("{}: Extras directory name must be a string.\n", filename);
                    continue;
                }

                config->extrasDirs.push_back(PathExp(*dirName));
            }
        }
        else if (const std::string* dirName = extrasDirsVal->getString(); dirName != nullptr)
        {
            config->extrasDirs.push_back(PathExp(*dirName));
        }
        else
        {
            GetLogger()->error("{}: ExtrasDirectories must be an array or a string.\n", filename);
        }
    }

    if (const Value* skipExtrasVal = configParams->getValue("SkipExtras"); skipExtrasVal != nullptr)
    {
        if (const ValueArray* skipExtras = skipExtrasVal->getArray(); skipExtras != nullptr)
        {
            for (const auto& fileNameVal : *skipExtras)
            {
                const std::string* fileName = fileNameVal.getString();
                if (fileName == nullptr)
                {
                    GetLogger()->error("{}: Skipped file name must be a string.\n", filename);
                    continue;
                }

                config->skipExtras.push_back(PathExp(*fileName));
            }
        }
        else if (const std::string* fileName = skipExtrasVal->getString(); fileName != nullptr)
        {
            config->skipExtras.push_back(PathExp(*fileName));
        }
        else
        {
            GetLogger()->error("{}: SkipExtras must be an array or a string.\n", filename);
        }
    }

    if (const Value* ignoreExtVal = configParams->getValue("IgnoreGLExtensions"); ignoreExtVal != nullptr)
    {
        if (const ValueArray* ignoreExt = ignoreExtVal->getArray(); ignoreExt == nullptr)
        {
            GetLogger()->error("{}: IgnoreGLExtensions must be an array.\n", filename);
        }
        else
        {
            for (const auto& extVal : *ignoreExt)
            {
                const std::string* ext = extVal.getString();
                if (ext == nullptr)
                {
                    GetLogger()->error("{}: extension name must be a string.\n", filename);
                    continue;
                }

                config->ignoreGLExtensions.push_back(*ext);
            }
        }
    }

    if (const Value* starTexValue = configParams->getValue("StarTextures"); starTexValue != nullptr)
    {
        if (const Hash* starTexTable = starTexValue->getHash(); starTexTable == nullptr)
        {
            GetLogger()->error("{}: StarTextures must be a property list.\n", filename);
        }
        else
        {
            string starTexNames[StellarClass::Spectral_Count];
            if (const std::string* texName = starTexTable->getString("O"); texName != nullptr)
                starTexNames[StellarClass::Spectral_O] = *texName;
            if (const std::string* texName = starTexTable->getString("B"); texName != nullptr)
                starTexNames[StellarClass::Spectral_B] = *texName;
            if (const std::string* texName = starTexTable->getString("A"); texName != nullptr)
                starTexNames[StellarClass::Spectral_A] = *texName;
            if (const std::string* texName = starTexTable->getString("F"); texName != nullptr)
                starTexNames[StellarClass::Spectral_F] = *texName;
            if (const std::string* texName = starTexTable->getString("G"); texName != nullptr)
                starTexNames[StellarClass::Spectral_G] = *texName;
            if (const std::string* texName = starTexTable->getString("K"); texName != nullptr)
                starTexNames[StellarClass::Spectral_K] = *texName;
            if (const std::string* texName = starTexTable->getString("M"); texName != nullptr)
                starTexNames[StellarClass::Spectral_M] = *texName;
            if (const std::string* texName = starTexTable->getString("R"); texName != nullptr)
                starTexNames[StellarClass::Spectral_R] = *texName;
            if (const std::string* texName = starTexTable->getString("S"); texName != nullptr)
                starTexNames[StellarClass::Spectral_S] = *texName;
            if (const std::string* texName = starTexTable->getString("N"); texName != nullptr)
                starTexNames[StellarClass::Spectral_N] = *texName;
            if (const std::string* texName = starTexTable->getString("WC"); texName != nullptr)
                starTexNames[StellarClass::Spectral_WC] = *texName;
            if (const std::string* texName = starTexTable->getString("WN"); texName != nullptr)
                starTexNames[StellarClass::Spectral_WN] = *texName;
            if (const std::string* texName = starTexTable->getString("WO"); texName != nullptr)
                starTexNames[StellarClass::Spectral_WO] = *texName;
            if (const std::string* texName = starTexTable->getString("Unknown"); texName != nullptr)
                starTexNames[StellarClass::Spectral_Unknown] = *texName;
            if (const std::string* texName = starTexTable->getString("L"); texName != nullptr)
                starTexNames[StellarClass::Spectral_L] = *texName;
            if (const std::string* texName = starTexTable->getString("T"); texName != nullptr)
                starTexNames[StellarClass::Spectral_T] = *texName;
            if (const std::string* texName = starTexTable->getString("Y"); texName != nullptr)
                starTexNames[StellarClass::Spectral_Y] = *texName;
            if (const std::string* texName = starTexTable->getString("C"); texName != nullptr)
                starTexNames[StellarClass::Spectral_C] = *texName;

            // One texture for all white dwarf types; not sure if this needs to be
            // changed. White dwarfs vary widely in temperature, so texture choice
            // should probably be based on that instead of spectral type.
            if (const std::string* texName = starTexTable->getString("WD"); texName != nullptr)
                starTexNames[StellarClass::Spectral_D] = *texName;

            if (const std::string* texName = starTexTable->getString("NeutronStar"); texName != nullptr)
                config->starTextures.neutronStarTex.setTexture(*texName, "textures");

            if (const std::string* texName = starTexTable->getString("Default"); texName != nullptr)
                config->starTextures.defaultTex.setTexture(*texName, "textures");

            for (unsigned int i = 0; i < (unsigned int) StellarClass::Spectral_Count; i++)
            {
                if (starTexNames[i] != "")
                {
                    config->starTextures.starTex[i].setTexture(starTexNames[i], "textures");
                }
            }
        }
    }

    // TODO: not cleaning up properly here--we're just saving the hash, not the instance of Value
    config->params = configParams;

#ifndef CELX
     delete configParamsValue;
#endif

    return config;
}
