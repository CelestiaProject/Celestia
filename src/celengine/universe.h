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
#include <celengine/univcoord.h>
#include <celengine/stardb.h>
#include <celengine/solarsys.h>
#include <celengine/galaxy.h>
#include <celengine/asterism.h>
#include <celengine/selection.h>


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
    AsterismList* getAsterisms() const;
    void setAsterisms(AsterismList*);

    Selection pick(const UniversalCoord& origin,
                   const Vec3f& direction,
                   double when,
                   float faintestMag,
                   float tolerance = 0.0f);
    Selection pickStar(const UniversalCoord&, const Vec3f&, float faintest,
                       float tolerance = 0.0f);
    Selection find(const std::string& s,
                   PlanetarySystem** solarSystems = NULL,
                   int nSolarSystems = 0);
    Selection findPath(const std::string& s,
                       PlanetarySystem** solarSystems = NULL,
                       int nSolarSystems = 0);

    SolarSystem* getNearestSolarSystem(const UniversalCoord& position) const;
    SolarSystem* getSolarSystem(const Star* star) const;
    SolarSystem* createSolarSystem(Star* star) const;

 private:
    Selection pickPlanet(SolarSystem& solarSystem,
                         const UniversalCoord&,
                         const Vec3f&,
                         double when,
                         float faintestMag,
                         float tolerance);

 private:
    StarDatabase* starCatalog;
    SolarSystemCatalog* solarSystemCatalog;
    GalaxyList* galaxyCatalog;
    AsterismList* asterisms;
};

#endif // UNIVERSE_H_
