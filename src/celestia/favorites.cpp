// favorites.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <celutil/debug.h>
#include <celutil/util.h>
#include <celengine/cmdparser.h>
#include "favorites.h"

using namespace std;


FavoritesList* ReadFavoritesList(istream& in)
{
    FavoritesList* favorites = new FavoritesList();
    Tokenizer tokenizer(&in);
    Parser parser(&tokenizer);

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        if (tokenizer.getTokenType() != Tokenizer::TokenString)
        {
            DPRINTF("Error parsing favorites file.\n");
            for_each(favorites->begin(), favorites->end(), deleteFunc<FavoritesEntry*>());
            delete favorites;
            return NULL;
        }

        FavoritesEntry* fav = new FavoritesEntry();
        fav->name = tokenizer.getStringValue();

        Value* favParamsValue = parser.readValue();
        if (favParamsValue == NULL || favParamsValue->getType() != Value::HashType)
        {
            DPRINTF("Error parsing favorites entry %s\n", fav->name.c_str());
            for_each(favorites->begin(), favorites->end(), deleteFunc<FavoritesEntry*>());
            delete favorites;
            if (favParamsValue != NULL)
                delete favParamsValue;
            return NULL;
        }

        Hash* favParams = favParamsValue->getHash();

        // Get position
        Vec3d base(0.0, 0.0, 0.0);
        Vec3d offset(0.0, 0.0, 0.0);
        favParams->getVector("base", base);
        favParams->getVector("offset", offset);
        fav->position = UniversalCoord(Point3d(base.x, base.y, base.z)) + offset;

        // Get orientation
        Vec3d axis(1.0, 0.0, 0.0);
        double angle = 0.0;
        favParams->getVector("axis", axis);
        favParams->getNumber("angle", angle);
        fav->orientation.setAxisAngle(Vec3f((float) axis.x, (float) axis.y, (float) axis.z),
                                      (float) angle);

        // Get time
        fav->jd = 0.0;
        favParams->getNumber("time", fav->jd);

        favorites->insert(favorites->end(), fav);
    }

    return favorites;
}


void WriteFavoritesList(FavoritesList& favorites, ostream& out)
{
    for (FavoritesList::const_iterator iter = favorites.begin();
         iter != favorites.end(); iter++)
    {
        FavoritesEntry* fav = *iter;

        Vec3f axis;
        float angle = 0;
        fav->orientation.getAxisAngle(axis, angle);

        Point3d base = (Point3d) fav->position;
        Vec3d offset = fav->position - base;

        out << '"' << fav->name << "\" {\n";
        out << setprecision(16);
        out << "\tbase   [ " << base.x << ' ' << base.y << ' ' << base.z << " ]\n";
        out << "\toffset [ " << offset.x << ' ' << offset.y << ' ' << offset.z << " ]\n";
        out << setprecision(6);
        out << "\taxis   [ " << axis.x << ' ' << axis.y << ' ' << axis.z << " ]\n";
        out << "\tangle  " << angle << '\n';
        out << setprecision(16);
        out << "\ttime   " << fav->jd << '\n';
        out << "}\n\n";
    }
}


#if 0
FavoritesList* ReadFavoritesList(string filename)
{
    ifstream in(filename.c_str());

    if (!in.good())
        return NULL;

    FavoritesList* favorites = new FavoritesList();
    Tokenizer tokenizer(&in);
    CommandParser parser(tokenizer);

    while (tokenizer.nextToken() != Tokenizer::TokenEnd)
    {
        if (tokenizer.getTokenType() != Tokenizer::TokenString)
        {
            DPRINTF("Error parsing favorites file.\n");
            for_each(favorites->begin(), favorites->end(), deleteFunc<FavoritesEntry*>());
            delete favorites;
            return NULL;
        }

        string name = tokenizer.getStringValue();
        CommandSequence* cmdSeq = parser.parse();
        if (cmdSeq == NULL)
        {
            DPRINTF("Error parsing favorites entry %s\n", name.c_str());
            for_each(favorites->begin(), favorites->end(), deleteFunc<FavoritesEntry*>());
            delete favorites;
            return NULL;
        }

        favorites->insert(favorites->end(), new FavoritesEntry(name, cmdSeq));
    }

    return favorites;
}
#endif
