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
#include <celutil/debug.h>
#include <celutil/util.h>
#include <celengine/texmanager.h>
#include "configfile.h"

using namespace std;


static unsigned int getUint(Hash* params,
                            const string& paramName,
                            unsigned int defaultValue)
{
    double value = 0.0;
    if (params->getNumber(paramName, value))
        return (unsigned int) value;

    return defaultValue;
}


CelestiaConfig* ReadCelestiaConfig(const string& filename, CelestiaConfig *config)
{
    ifstream configFile(filename);
    if (!configFile.good())
    {
        DPRINTF(0, "Error opening config file '%s'.\n", filename.c_str());
        return config;
    }

    Tokenizer tokenizer(&configFile);
    Parser parser(&tokenizer);

    if (tokenizer.nextToken() != Tokenizer::TokenName)
    {
        DPRINTF(0, "%s:%d 'Configuration' expected.\n", filename.c_str(),
                tokenizer.getLineNumber());
        return config;
    }

    if (tokenizer.getStringValue() != "Configuration")
    {
        DPRINTF(0, "%s:%d 'Configuration' expected.\n", filename.c_str(),
                tokenizer.getLineNumber());
        return config;
    }

    Value* configParamsValue = parser.readValue();
    if (configParamsValue == nullptr || configParamsValue->getType() != Value::HashType)
    {
        DPRINTF(0, "%s: Bad configuration file.\n", filename.c_str());
        return config;
    }

    Hash* configParams = configParamsValue->getHash();

    if (config == nullptr)
        config = new CelestiaConfig();

#ifdef CELX
    config->configParams = configParams;
    configParams->getString("LuaHook", config->luaHook);
    config->luaHook = WordExp(config->luaHook);
#endif

    config->faintestVisible = 6.0f;
    configParams->getNumber("FaintestVisibleMagnitude", config->faintestVisible);
    configParams->getString("FavoritesFile", config->favoritesFile);
    config->favoritesFile = WordExp(config->favoritesFile);
    configParams->getString("DestinationFile", config->destinationsFile);
    config->destinationsFile = WordExp(config->destinationsFile);
    configParams->getString("InitScript", config->initScriptFile);
    config->initScriptFile = WordExp(config->initScriptFile);
    configParams->getString("DemoScript", config->demoScriptFile);
    config->demoScriptFile = WordExp(config->demoScriptFile);
    configParams->getString("AsterismsFile", config->asterismsFile);
    config->asterismsFile = WordExp(config->asterismsFile);
    configParams->getString("BoundariesFile", config->boundariesFile);
    config->boundariesFile = WordExp(config->boundariesFile);
    configParams->getString("StarDatabase", config->starDatabaseFile);
    config->starDatabaseFile = WordExp(config->starDatabaseFile);
    configParams->getString("StarNameDatabase", config->starNamesFile);
    config->starNamesFile = WordExp(config->starNamesFile);
    configParams->getString("HDCrossIndex", config->HDCrossIndexFile);
    config->HDCrossIndexFile = WordExp(config->HDCrossIndexFile);
    configParams->getString("SAOCrossIndex", config->SAOCrossIndexFile);
    config->SAOCrossIndexFile = WordExp(config->SAOCrossIndexFile);
    configParams->getString("GlieseCrossIndex", config->GlieseCrossIndexFile);
    config->GlieseCrossIndexFile = WordExp(config->GlieseCrossIndexFile);
    configParams->getString("Font", config->mainFont);
    configParams->getString("LabelFont", config->labelFont);
    configParams->getString("TitleFont", config->titleFont);
    configParams->getString("LogoTexture", config->logoTextureFile);
    configParams->getString("Cursor", config->cursor);

    float maxDist = 1.0;
    configParams->getNumber("SolarSystemMaxDistance", maxDist);
    config->SolarSystemMaxDistance = min(max(maxDist, 1.0f), 10.0f);

    double aaSamples = 1;
    configParams->getNumber("AntialiasingSamples", aaSamples);
    config->aaSamples = (unsigned int) aaSamples;

    config->hdr = false;
    configParams->getBoolean("HighDynamicRange", config->hdr);

    config->rotateAcceleration = 120.0f;
    configParams->getNumber("RotateAcceleration", config->rotateAcceleration);
    config->mouseRotationSensitivity = 1.0f;
    configParams->getNumber("MouseRotationSensitivity", config->mouseRotationSensitivity);
    config->reverseMouseWheel = false;
    configParams->getBoolean("ReverseMouseWheel", config->reverseMouseWheel);
    configParams->getString("ScriptScreenshotDirectory", config->scriptScreenshotDirectory);
    config->scriptScreenshotDirectory = WordExp(config->scriptScreenshotDirectory);
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

    Value* solarSystemsVal = configParams->getValue("SolarSystemCatalogs");
    if (solarSystemsVal != nullptr)
    {
        if (solarSystemsVal->getType() != Value::ArrayType)
        {
            DPRINTF(0, "%s: SolarSystemCatalogs must be an array.\n", filename.c_str());
        }
        else
        {
            Array* solarSystems = solarSystemsVal->getArray();
            // assert(solarSystems != nullptr);

            for (const auto catalogNameVal : *solarSystems)
            {
                // assert(catalogNameVal != nullptr);
                if (catalogNameVal->getType() == Value::StringType)
                {
                    config->solarSystemFiles.push_back(WordExp(catalogNameVal->getString()));
                }
                else
                {
                    DPRINTF(0, "%s: Solar system catalog name must be a string.\n",
                            filename.c_str());
                }
            }
        }
    }

    Value* starCatalogsVal = configParams->getValue("StarCatalogs");
    if (starCatalogsVal != nullptr)
    {
        if (starCatalogsVal->getType() != Value::ArrayType)
        {
            DPRINTF(0, "%s: StarCatalogs must be an array.\n",
                    filename.c_str());
        }
        else
        {
            Array* starCatalogs = starCatalogsVal->getArray();
            assert(starCatalogs != nullptr);

            for (const auto catalogNameVal : *starCatalogs)
            {
                assert(catalogNameVal != nullptr);

                if (catalogNameVal->getType() == Value::StringType)
                {
                    config->starCatalogFiles.push_back(WordExp(catalogNameVal->getString()));
                }
                else
                {
                    DPRINTF(0, "%s: Star catalog name must be a string.\n",
                            filename.c_str());
                }
            }
        }
    }

    Value* dsoCatalogsVal = configParams->getValue("DeepSkyCatalogs");
    if (dsoCatalogsVal != nullptr)
    {
        if (dsoCatalogsVal->getType() != Value::ArrayType)
        {
            DPRINTF(0, "%s: DeepSkyCatalogs must be an array.\n",
                    filename.c_str());
        }
        else
        {
            Array* dsoCatalogs = dsoCatalogsVal->getArray();
            assert(dsoCatalogs != nullptr);

            for (const auto catalogNameVal : *dsoCatalogs)
            {
                assert(catalogNameVal != nullptr);

                if (catalogNameVal->getType() == Value::StringType)
                {
                    config->dsoCatalogFiles.push_back(WordExp(catalogNameVal->getString()));
                }
                else
                {
                    DPRINTF(0, "%s: DeepSky catalog name must be a string.\n",
                            filename.c_str());
                }
            }
        }
    }

    Value* extrasDirsVal = configParams->getValue("ExtrasDirectories");
    if (extrasDirsVal != nullptr)
    {
        if (extrasDirsVal->getType() == Value::ArrayType)
        {
            Array* extrasDirs = extrasDirsVal->getArray();
            assert(extrasDirs != nullptr);

            for (const auto dirNameVal : *extrasDirs)
            {
                if (dirNameVal->getType() == Value::StringType)
                {
                    config->extrasDirs.push_back(WordExp(dirNameVal->getString()));
                }
                else
                {
                    DPRINTF(0, "%s: Extras directory name must be a string.\n",
                            filename.c_str());
                }
            }
        }
        else if (extrasDirsVal->getType() == Value::StringType)
        {
            config->extrasDirs.push_back(WordExp(extrasDirsVal->getString()));
        }
        else
        {
            DPRINTF(0, "%s: ExtrasDirectories must be an array or string.\n", filename.c_str());
        }
    }

    Value* ignoreExtVal = configParams->getValue("IgnoreGLExtensions");
    if (ignoreExtVal != nullptr)
    {
        if (ignoreExtVal->getType() != Value::ArrayType)
        {
            DPRINTF(0, "%s: IgnoreGLExtensions must be an array.\n",
                    filename.c_str());
        }
        else
        {
            Array* ignoreExt = ignoreExtVal->getArray();

            for (const auto extVal : *ignoreExt)
            {
                if (extVal->getType() == Value::StringType)
                {
                    config->ignoreGLExtensions.push_back(extVal->getString());
                }
                else
                {
                    DPRINTF(0, "%s: extension name must be a string.\n", filename.c_str());
                }
            }
        }
    }

    Value* starTexValue = configParams->getValue("StarTextures");
    if (starTexValue != nullptr)
    {
        if (starTexValue->getType() != Value::HashType)
        {
            DPRINTF(0, "%s: StarTextures must be a property list.\n", filename.c_str());
        }
        else
        {
            Hash* starTexTable = starTexValue->getHash();
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


const string
CelestiaConfig::getStringValue(const string& name)
{
    assert(params != nullptr);

    Value* v = params->getValue(name);
    if (v == nullptr || v->getType() != Value::StringType)
        return string("");

    return v->getString();
}
