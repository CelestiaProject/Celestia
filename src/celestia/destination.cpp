// destination.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <config.h>
#include <algorithm>
#include <celutil/debug.h>
#include <celutil/stringutils.h>
#include <celutil/util.h>
#include <celengine/astro.h>
#include <celengine/parser.h>
#include <celengine/tokenizer.h>
#include "destination.h"

using namespace std;


DestinationList* ReadDestinationList(istream& in)
{
    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);
    auto* destinations = new DestinationList();

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        if (tokenizer.getTokenType() != Tokenizer::TokenBeginGroup)
        {
            DPRINTF(LOG_LEVEL_ERROR, "Error parsing destinations file.\n");
            for_each(destinations->begin(), destinations->end(), deleteFunc<Destination*>());
            delete destinations;
            return nullptr;
        }
        tokenizer.pushBack();

        Value* destValue = parser.readValue();
        if (destValue == nullptr || destValue->getType() != Value::HashType)
        {
            DPRINTF(LOG_LEVEL_ERROR, "Error parsing destination.\n");
            for_each(destinations->begin(), destinations->end(), deleteFunc<Destination*>());
            delete destinations;
            if (destValue != nullptr)
                delete destValue;
            return nullptr;
        }

        Hash* destParams = destValue->getHash();
        Destination* dest = new Destination();

        if (!destParams->getString("Name", dest->name))
        {
            DPRINTF(LOG_LEVEL_INFO, "Skipping unnamed destination\n");
            delete dest;
        }
        else
        {
            destParams->getString("Target", dest->target);
            destParams->getString("Description", dest->description);
            destParams->getNumber("Distance", dest->distance);

            // Default unit of distance is the light year
            string distanceUnits;
            if (destParams->getString("DistanceUnits", distanceUnits))
            {
                if (!compareIgnoringCase(distanceUnits, "km"))
                    dest->distance = astro::kilometersToLightYears(dest->distance);
                else if (!compareIgnoringCase(distanceUnits, "au"))
                    dest->distance = astro::AUtoLightYears(dest->distance);
            }

            destinations->push_back(dest);
        }

        delete destValue;
    }

    return destinations;
}
