// destination.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>

#include <celengine/astro.h>
#include <celengine/hash.h>
#include <celengine/parser.h>
#include <celengine/value.h>
#include <celutil/logger.h>
#include <celutil/stringutils.h>
#include <celutil/tokenizer.h>
#include "destination.h"

using celestia::util::GetLogger;

DestinationList* ReadDestinationList(std::istream& in)
{
    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);
    auto* destinations = new DestinationList();

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        if (tokenizer.getTokenType() != Tokenizer::TokenBeginGroup)
        {
            GetLogger()->error("Error parsing destinations file.\n");
            std::for_each(destinations->begin(), destinations->end(), [](Destination* dest) { delete dest; });
            delete destinations;
            return nullptr;
        }
        tokenizer.pushBack();

        const Value destValue = parser.readValue();
        const Hash* destParams = destValue.getHash();
        if (destParams == nullptr)
        {
            GetLogger()->error("Error parsing destination.\n");
            std::for_each(destinations->begin(), destinations->end(), [](Destination* dest) { delete dest; });
            delete destinations;
            return nullptr;
        }

        Destination* dest = new Destination();

        if (const std::string* destName = destParams->getString("Name"); destName == nullptr)
        {
            GetLogger()->warn("Skipping unnamed destination\n");
            delete dest;
        }
        else
        {
            dest->name = *destName;
            if (const std::string* target = destParams->getString("Target"); target != nullptr)
                dest->target = *target;
            if (const std::string* description = destParams->getString("Description"); description != nullptr)
                dest->description = *description;
            if (auto distance = destParams->getNumber<double>("Distance"); distance.has_value())
                dest->distance = *distance;

            // Default unit of distance is the light year
            if (const std::string* distanceUnits = destParams->getString("DistanceUnits"); distanceUnits != nullptr)
            {
                if (!compareIgnoringCase(*distanceUnits, "km"))
                    dest->distance = astro::kilometersToLightYears(dest->distance);
                else if (!compareIgnoringCase(*distanceUnits, "au"))
                    dest->distance = astro::AUtoLightYears(dest->distance);
            }

            destinations->push_back(dest);
        }
    }

    return destinations;
}
