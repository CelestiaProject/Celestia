// config.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include <fstream>
#include "celestia.h"
#include "parser.h"
#include "config.h"

using namespace std;


CelestiaConfig* ReadCelestiaConfig(string filename)
{
    ifstream configFile(filename.c_str());
    if (!configFile.good())
    {
        DPRINTF("Error opening config file.");
        return NULL;
    }

    Tokenizer tokenizer(&configFile);
    Parser parser(&tokenizer);

    if (tokenizer.nextToken() != Tokenizer::TokenName)
    {
        DPRINTF("%s:%d 'Configuration' expected.\n", filename.c_str(),
                tokenizer.getLineNumber());
        return NULL;
    }

    if (tokenizer.getStringValue() != "Configuration")
    {
        DPRINTF("%s:%d 'Configuration' expected.\n", filename.c_str(),
                tokenizer.getLineNumber());
        return NULL;
    }

    Value* configParamsValue = parser.readValue();
    if (configParamsValue == NULL || configParamsValue->getType() != Value::HashType)
    {
        DPRINTF("%s: Bad configuration file.\n", filename.c_str());
        return NULL;
    }

    Hash* configParams = configParamsValue->getHash();

    CelestiaConfig* config = new CelestiaConfig();

    config->faintestVisible = 6.0f;
    configParams->getNumber("FaintestVisibleMagnitude", config->faintestVisible);
    configParams->getString("StarDatabase", config->starDatabaseFile);
    configParams->getString("StarNameDatabase", config->starNamesFile);

    Value* solarSystemsVal = configParams->getValue("SolarSystemCatalogs");
    if (solarSystemsVal != NULL)
    {
        if (solarSystemsVal->getType() != Value::ArrayType)
        {
            DPRINTF("%s: SolarSystemCatalogs must be an array.\n", filename.c_str());
        }
        else
        {
            Array* solarSystems = solarSystemsVal->getArray();
            // assert(solarSystems != NULL);

            for (Array::iterator iter = solarSystems->begin(); iter != solarSystems->end(); iter++)
            {
                Value* catalogNameVal = *iter;
                // assert(catalogNameVal != NULL);
                if (catalogNameVal->getType() == Value::StringType)
                {
                    config->solarSystemFiles.insert(config->solarSystemFiles.end(),
                                                    catalogNameVal->getString());
                }
                else
                {
                    DPRINTF("%s: Solar system catalog name must be a string.\n",
                            filename.c_str());
                }
            }
        }
    }

    Value* labelledStarsVal = configParams->getValue("LabelledStars");
    if (labelledStarsVal != NULL)
    {
        if (labelledStarsVal->getType() != Value::ArrayType)
        {
            DPRINTF("%s: LabelledStars must be an array.\n", filename.c_str());
        }
        else
        {
            Array* labelledStars = labelledStarsVal->getArray();
            // assert(labelledStars != NULL);

            for (Array::iterator iter = labelledStars->begin(); iter != labelledStars->end(); iter++)
            {
                Value* starNameVal = *iter;
                // assert(starNameVal != NULL);
                if (starNameVal->getType() == Value::StringType)
                {
                    config->labelledStars.insert(config->labelledStars.end(),
                                                 starNameVal->getString());
                }
                else
                {
                    DPRINTF("%s: Star name must be a string.\n", filename.c_str());
                }
            }
        }
    }

    delete configParamsValue;

    return config;
}
