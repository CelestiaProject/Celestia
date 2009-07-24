// favorites.h
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELESTIA_FAVORITES_H_
#define _CELESTIA_FAVORITES_H_

#include <string>
#include <vector>
#include <iostream>
#include <celengine/observer.h>


struct FavoritesEntry
{
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    std::string name;
    std::string selectionName;
    
    UniversalCoord position;
    Eigen::Quaternionf orientation;
    double jd;
    bool isFolder;
    std::string parentFolder;

    ObserverFrame::CoordinateSystem coordSys;
};

typedef std::vector<FavoritesEntry*> FavoritesList;

FavoritesList* ReadFavoritesList(std::istream&);
void WriteFavoritesList(FavoritesList&, std::ostream&);

#endif // _FAVORITES_H_
