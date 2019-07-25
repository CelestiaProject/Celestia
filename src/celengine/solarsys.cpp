// solarsys.cpp
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// Solar system catalog
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "frametree.h"
#include "solarsys.h"

using namespace Eigen;
using namespace std;
using namespace celmath;

SolarSystem::SolarSystem(Star* _star) :
    star(_star),
    planets(nullptr),
    frameTree(nullptr)
{
    planets = new PlanetarySystem(star);
    frameTree = new FrameTree(star);
}

SolarSystem::~SolarSystem()
{
    delete planets;
    delete frameTree;
}


Star* SolarSystem::getStar() const
{
    return star;
}

Vector3f SolarSystem::getCenter() const
{
    // TODO: This is a very simple method at the moment, but it will get
    // more complex when planets around multistar systems are supported
    // where the planets may orbit the center of mass of two stars.
    return star->getPosition().cast<float>();
}

PlanetarySystem* SolarSystem::getPlanets() const
{
    return planets;
}

FrameTree* SolarSystem::getFrameTree() const
{
    return frameTree;
}
