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
#include <fstream>
#include "util.h"
#include "cmdparser.h"
#include "favorites.h"

using namespace std;


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
