// destination.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <celutil/debug.h>
#include <celutil/util.h>
#include <celengine/celestia.h>
#include <celengine/astro.h>
#include <celengine/parser.h>
#include "destination.h"

using namespace std;


Destination::Destination() :
    name(""),
    target(""),
    distance(0.0),
    description("")
{
}


DestinationList* ReadDestinationList(istream& in)
{
    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);
    DestinationList* destinations = new DestinationList();

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        if (tokenizer.getTokenType() != Tokenizer::TokenBeginGroup)
        {
            DPRINTF(0, "Error parsing destinations file.\n");
            for_each(destinations->begin(), destinations->end(), deleteFunc<Destination*>());
            delete destinations;
            return NULL;
        }
        tokenizer.pushBack();

        Value* destValue = parser.readValue();
        if (destValue == NULL || destValue->getType() != Value::HashType)
        {
            DPRINTF(0, "Error parsing destination.\n");
            for_each(destinations->begin(), destinations->end(), deleteFunc<Destination*>());
            delete destinations;
            if (destValue != NULL)
                delete destValue;
            return NULL;
        }

        Hash* destParams = destValue->getHash();
        Destination* dest = new Destination();
        
        if (!destParams->getString("Name", dest->name))
        {
            DPRINTF(1, "Skipping unnamed destination\n");
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

            destinations->insert(destinations->end(), dest);
        }

        delete destValue;
    }

    return destinations;
}
