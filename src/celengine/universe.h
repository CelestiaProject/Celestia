// universe.h
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _UNIVERSE_H_
#define _UNIVERSE_H_

#include <celmath/vecmath.h>
#include <celmath/quaternion.h>
#include <celengine/stardb.h>
#include <celengine/solarsys.h>
#include <celengine/galaxy.h>
#include <celengine/selection.h>
#include <celengine/univcoord.h>
#include <celengine/render.h>


class Universe
{
 public:
    Universe();
    ~Universe();

    StarDatabase* getStarCatalog() const;
    void setStarCatalog(StarDatabase*);
    SolarSystemCatalog* getSolarSystemCatalog() const;
    void setSolarSystemCatalog(SolarSystemCatalog*);
    GalaxyList* getGalaxyCatalog() const;
    void setGalaxyCatalog(GalaxyList*);

    void render(Renderer&);

    Selection pick(const UniversalCoord& origin,
                   const Vec3f& direction);
    Selection find(const std::string& s,
                   PlanetarySystem** solarSystems = NULL,
                   int nSolarSystems = 0);
    Selection findPath(const std::string& s,
                       PlanetarySystem** solarSystems = NULL,
                       int nSolarSystems = 0);

    SolarSystem* getNearestSolarSystem(const UniversalCoord& position);

 private:
    SolarSystem* getSolarSystem(const Star* star);
    Selection pickPlanet(SolarSystem& solarSystem,
                         const UniversalCoord&,
                         const Vec3f&,
                         double);
    Selection pickStar(const UniversalCoord&, const Vec3f&);

 private:
    StarDatabase* starCatalog;
    SolarSystemCatalog* solarSystemCatalog;
    GalaxyList* galaxyCatalog;
};

#endif // UNIVERSE_H_
