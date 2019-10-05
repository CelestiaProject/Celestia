// universe.h
//
// Copyright (C) 2001-2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_UNIVERSE_H_
#define _CELENGINE_UNIVERSE_H_

#include <celengine/univcoord.h>
//#include <celengine/stardb.h>
//#include <celengine/dsodb.h>
#include <celengine/astrodb.h>
#include <celengine/solarsys.h>
#include <celengine/deepskyobj.h>
#include <celengine/marker.h>
#include <celengine/selection.h>
#include <celengine/asterism.h>
#include <vector>


class ConstellationBoundaries;

class Universe
{
 public:
    Universe();
    ~Universe();

    AstroDatabase& getDatabase() { return m_adb; }
    const AstroDatabase& getDatabase() const { return m_adb; }

    AsterismList* getAsterisms() const;
    void setAsterisms(AsterismList*);

    ConstellationBoundaries* getBoundaries() const;
    void setBoundaries(ConstellationBoundaries*);

    Selection pick(const UniversalCoord& origin,
                   const Eigen::Vector3f& direction,
                   double when,
                   uint64_t renderFlags,
                   float faintestMag,
                   float tolerance = 0.0f);


    Selection find(const std::string& s,
                   Selection* contexts = nullptr,
                   int nContexts = 0,
                   bool i18n = false) const;
    Selection findPath(const std::string& s,
                       Selection* contexts = nullptr,
                       int nContexts = 0,
                       bool i18n = false) const;
    Selection findChildObject(const Selection& sel,
                              const string& name,
                              bool i18n = false) const;
    Selection findObjectInContext(const Selection& sel,
                                  const string& name,
                                  bool i18n = false) const;

    std::vector<Name> getCompletion(const std::string& s,
                                           Selection* contexts = nullptr,
                                           int nContexts = 0,
                                           bool withLocations = false);
    std::vector<Name> getCompletionPath(const std::string& s,
                                               Selection* contexts = nullptr,
                                               int nContexts = 0,
                                               bool withLocations = false);


    SolarSystem* getNearestSolarSystem(const UniversalCoord& position) const;
    SolarSystem* getSolarSystem(const Star* star) const;
    SolarSystem* getSolarSystem(const Selection&) const;
    SolarSystem* createSolarSystem(Star* star);

    void getNearStars(const UniversalCoord& position,
                      float maxDistance,
                      std::vector<const Star*>& stars) const;

    void markObject(const Selection&,
                    const MarkerRepresentation& rep,
                    int priority,
                    bool occludable = true,
                    MarkerSizing sizing = ConstantSize);
    void unmarkObject(const Selection&, int priority);
    void unmarkAll();
    bool isMarked(const Selection&, int priority) const;
    MarkerList* getMarkers() const;

 private:
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
    AstroDatabase m_adb;
    AsterismList* asterisms{nullptr};
    ConstellationBoundaries* boundaries{nullptr};
    MarkerList* markers;

    std::vector<const Star*> closeStars;
};

#endif // _CELENGINE_UNIVERSE_H_
