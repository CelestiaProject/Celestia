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

static unsigned int getUint(const Hash* params,
                            const string& paramName,
                            unsigned int defaultValue)
{
    double value = 0.0;
    if (params->getNumber(paramName, value))
        return (unsigned int) value;

    return defaultValue;
}


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
    configParams->getPath("LuaHook", config->luaHook);
#endif

    config->faintestVisible = 6.0f;
    configParams->getNumber("FaintestVisibleMagnitude", config->faintestVisible);
    configParams->getPath("FavoritesFile", config->favoritesFile);
    configParams->getPath("DestinationFile", config->destinationsFile);
    configParams->getPath("InitScript", config->initScriptFile);
    configParams->getPath("DemoScript", config->demoScriptFile);
    configParams->getPath("AsterismsFile", config->asterismsFile);
    configParams->getPath("BoundariesFile", config->boundariesFile);
    configParams->getPath("StarDatabase", config->starDatabaseFile);
    configParams->getPath("StarNameDatabase", config->starNamesFile);
    configParams->getPath("HDCrossIndex", config->HDCrossIndexFile);
    configParams->getPath("SAOCrossIndex", config->SAOCrossIndexFile);
    configParams->getPath("GlieseCrossIndex", config->GlieseCrossIndexFile);
    configParams->getPath("LeapSecondsFile", config->leapSecondsFile);
    configParams->getString("Font", config->mainFont);
    configParams->getString("LabelFont", config->labelFont);
    configParams->getString("TitleFont", config->titleFont);
    configParams->getPath("LogoTexture", config->logoTextureFile);
    configParams->getString("Cursor", config->cursor);
    configParams->getString("ProjectionMode", config->projectionMode);
    configParams->getString("ViewportEffect", config->viewportEffect);
    configParams->getString("WarpMeshFile", config->warpMeshFile);
    configParams->getString("X264EncoderOptions", config->x264EncoderOptions);
    configParams->getString("FFVHEncoderOptions", config->ffvhEncoderOptions);
    configParams->getString("MeasurementSystem", config->measurementSystem);
    configParams->getString("TemperatureScale", config->temperatureScale);

    float maxDist = 1.0;
    configParams->getNumber("SolarSystemMaxDistance", maxDist);
    config->SolarSystemMaxDistance = min(max(maxDist, 1.0f), 10.0f);

    config->ShadowMapSize = getUint(configParams, "ShadowMapSize", 0);

    double aaSamples = 1;
    configParams->getNumber("AntialiasingSamples", aaSamples);
    config->aaSamples = (unsigned int) aaSamples;

    config->rotateAcceleration = 120.0f;
    configParams->getNumber("RotateAcceleration", config->rotateAcceleration);
    config->mouseRotationSensitivity = 1.0f;
    configParams->getNumber("MouseRotationSensitivity", config->mouseRotationSensitivity);
    config->reverseMouseWheel = false;
    configParams->getBoolean("ReverseMouseWheel", config->reverseMouseWheel);
    configParams->getPath("ScriptScreenshotDirectory", config->scriptScreenshotDirectory);
    config->scriptSystemAccessPolicy = "ask";
    configParams->getString("ScriptSystemAccessPolicy", config->scriptSystemAccessPolicy);

    config->orbitWindowEnd = 0.5f;
    configParams->getNumber("OrbitWindowEnd", config->orbitWindowEnd);
    config->orbitPeriodsShown = 1.0f;
    configParams->getNumber("OrbitPeriodsShown", config->orbitPeriodsShown);
    config->linearFadeFraction = 0.0f;
    configParams->getNumber("LinearFadeFraction", config->linearFadeFraction);

    config->orbitPathSamplePoints = getUint(configParams, "OrbitPathSamplePoints", 100);
    config->shadowTextureSize = getUint(configParams, "ShadowTextureSize", 256);
    config->eclipseTextureSize = getUint(configParams, "EclipseTextureSize", 128);

    config->consoleLogRows = getUint(configParams, "LogSize", 200);

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
            starTexTable->getString("O", starTexNames[StellarClass::Spectral_O]);
            starTexTable->getString("B", starTexNames[StellarClass::Spectral_B]);
            starTexTable->getString("A", starTexNames[StellarClass::Spectral_A]);
            starTexTable->getString("F", starTexNames[StellarClass::Spectral_F]);
            starTexTable->getString("G", starTexNames[StellarClass::Spectral_G]);
            starTexTable->getString("K", starTexNames[StellarClass::Spectral_K]);
            starTexTable->getString("M", starTexNames[StellarClass::Spectral_M]);
            starTexTable->getString("R", starTexNames[StellarClass::Spectral_R]);
            starTexTable->getString("S", starTexNames[StellarClass::Spectral_S]);
            starTexTable->getString("N", starTexNames[StellarClass::Spectral_N]);
            starTexTable->getString("WC", starTexNames[StellarClass::Spectral_WC]);
            starTexTable->getString("WN", starTexNames[StellarClass::Spectral_WN]);
            starTexTable->getString("WO", starTexNames[StellarClass::Spectral_WO]);
            starTexTable->getString("Unknown", starTexNames[StellarClass::Spectral_Unknown]);
            starTexTable->getString("L", starTexNames[StellarClass::Spectral_L]);
            starTexTable->getString("T", starTexNames[StellarClass::Spectral_T]);
            starTexTable->getString("Y", starTexNames[StellarClass::Spectral_Y]);
            starTexTable->getString("C", starTexNames[StellarClass::Spectral_C]);

            // One texture for all white dwarf types; not sure if this needs to be
            // changed. White dwarfs vary widely in temperature, so texture choice
            // should probably be based on that instead of spectral type.
            starTexTable->getString("WD", starTexNames[StellarClass::Spectral_D]);

            string neutronStarTexName;
            if (starTexTable->getString("NeutronStar", neutronStarTexName))
            {
                config->starTextures.neutronStarTex.setTexture(neutronStarTexName, "textures");
            }

            string defaultTexName;
            if (starTexTable->getString("Default", defaultTexName))
            {
                config->starTextures.defaultTex.setTexture(defaultTexName, "textures");
            }

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


float
CelestiaConfig::getFloatValue(const string& name)
{
    assert(params != nullptr);

    double x = 0.0;
    params->getNumber(name, x);

    return (float) x;
}
