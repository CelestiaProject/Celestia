// universe.h
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <celengine/univcoord.h>
#include <celengine/stardb.h>
#include <celengine/dsodb.h>
#include <celengine/solarsys.h>
#include <celengine/deepskyobj.h>
#include <celengine/marker.h>
#include <celengine/selection.h>
#include <celengine/asterism.h>
#include <celutil/array_view.h>


class ConstellationBoundaries;

class Universe
{
 public:
    Universe();
    ~Universe();

    StarDatabase* getStarCatalog() const;
    void setStarCatalog(std::unique_ptr<StarDatabase>&&);

    SolarSystemCatalog* getSolarSystemCatalog() const;
    void setSolarSystemCatalog(SolarSystemCatalog*);

    DSODatabase* getDSOCatalog() const;
    void setDSOCatalog(DSODatabase*);

    AsterismList* getAsterisms() const;
    void setAsterisms(std::unique_ptr<AsterismList>&&);

    ConstellationBoundaries* getBoundaries() const;
    void setBoundaries(ConstellationBoundaries*);

    Selection pick(const UniversalCoord& origin,
                   const Eigen::Vector3f& direction,
                   double when,
                   uint64_t renderFlags,
                   float faintestMag,
                   float tolerance = 0.0f);


    Selection find(std::string_view s,
                   celestia::util::array_view<const Selection> contexts,
                   bool i18n = false) const;
    Selection findPath(std::string_view s,
                       celestia::util::array_view<const Selection> contexts,
                       bool i18n = false) const;

    void getCompletionPath(std::vector<std::string>& completion,
                           std::string_view s,
                           bool i18n,
                           celestia::util::array_view<const Selection> contexts,
                           bool withLocations = false) const;


    SolarSystem* getNearestSolarSystem(const UniversalCoord& position) const;
    SolarSystem* getSolarSystem(const Star* star) const;
    SolarSystem* getSolarSystem(const Selection&) const;
    SolarSystem* createSolarSystem(Star* star) const;

    void getNearStars(const UniversalCoord& position,
                      float maxDistance,
                      std::vector<const Star*>& stars) const;

    void markObject(const Selection&,
                    const celestia::MarkerRepresentation& rep,
                    int priority,
                    bool occludable = true,
                    celestia::MarkerSizing sizing = celestia::ConstantSize);
    void unmarkObject(const Selection&, int priority);
    void unmarkAll();
    bool isMarked(const Selection&, int priority) const;
    celestia::MarkerList* getMarkers() const;

 private:
    void getCompletion(std::vector<std::string>& completion,
                       std::string_view s,
                       bool i18n,
                       celestia::util::array_view<const Selection> contexts,
                       bool withLocations = false) const;

    Selection findChildObject(const Selection& sel,
                              std::string_view name,
                              bool i18n = false) const;
    Selection findObjectInContext(const Selection& sel,
                                  std::string_view name,
                                  bool i18n = false) const;

    Selection pickPlanet(SolarSystem& solarSystem,
                         const UniversalCoord& origin,
                         const Eigen::Vector3f& direction,
                         double when,
                         float faintestMag,
                         float tolerance);

    Selection pickStar(const UniversalCoord& origin,
                       const Eigen::Vector3f& direction,
                       double when,
                       float faintest,
                       float tolerance = 0.0f);

    Selection pickDeepSkyObject(const UniversalCoord& origin,
                                const Eigen::Vector3f& direction,
                                uint64_t renderFlags,
                                float faintest,
                                float tolerance = 0.0f);

 private:
    std::unique_ptr<StarDatabase> starCatalog{nullptr};
    DSODatabase* dsoCatalog{nullptr};
    SolarSystemCatalog* solarSystemCatalog{nullptr};
    std::unique_ptr<AsterismList> asterisms{nullptr};
    ConstellationBoundaries* boundaries{nullptr};
    celestia::MarkerList* markers;

    std::vector<const Star*> closeStars;
};
