// favorites.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _FAVORITES_H_
#define _FAVORITES_H_

#include <string>
#include <vector>
#include "command.h"


struct FavoritesEntry
{
    FavoritesEntry(std::string _name, CommandSequence* _cmdSeq) :
        name(_name), cmdSeq(_cmdSeq) {};
    std::string name;
    CommandSequence* cmdSeq;
};

typedef std::vector<FavoritesEntry*> FavoritesList;

FavoritesList* ReadFavoritesList(string filename);

#endif // _FAVORITES_H_
