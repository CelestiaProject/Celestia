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
#include <iostream>
#include <celmath/vecmath.h>
#include <celmath/quaternion.h>
#include <celengine/observer.h>


struct FavoritesEntry
{
    std::string name;
    std::string selectionName;
    
    UniversalCoord position;
    Quatf orientation;
    double jd;
    bool isFolder;
    std::string parentFolder;

    ObserverFrame::CoordinateSystem coordSys;
};

typedef std::vector<FavoritesEntry*> FavoritesList;

FavoritesList* ReadFavoritesList(std::istream&);
void WriteFavoritesList(FavoritesList&, std::ostream&);

#endif // _FAVORITES_H_
