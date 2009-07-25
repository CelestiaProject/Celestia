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
#include <celengine/body.h>
#include <celengine/stardb.h>

class FrameTree;

class SolarSystem
{
 public:
    SolarSystem(Star*);

    Star* getStar() const;
    Eigen::Vector3f getCenter() const;
    PlanetarySystem* getPlanets() const;
    FrameTree* getFrameTree() const;

 private:
    Star* star;
    PlanetarySystem* planets;
    FrameTree* frameTree;
};

typedef std::map<uint32, SolarSystem*> SolarSystemCatalog;

class Universe;

bool LoadSolarSystemObjects(std::istream& in,
                            Universe& universe,
                            const std::string& dir = "");

#endif // _SOLARSYS_H_

