// solarsys.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _SOLARSYS_H_
#define _SOLARSYS_H_

#include <vector>
#include <map>
#include <iostream>
#include "body.h"
#include "stardb.h"


class SolarSystem
{
 public:
    SolarSystem(uint32 _starNumber);

    uint32 getStarNumber() const;
    PlanetarySystem* getPlanets() const;
    
 private:
    PlanetarySystem* planets;
    uint32 starNumber;
};

typedef std::map<uint32, SolarSystem*> SolarSystemCatalog;

SolarSystemCatalog* ReadSolarSystemCatalog(std::istream&, StarDatabase&);
bool ReadSolarSystems(std::istream&, const StarDatabase&, SolarSystemCatalog&);

// Mercury, Venus, Earth, etc.
SolarSystem* CreateOurSolarSystem();

#endif // _SOLARSYS_H_

