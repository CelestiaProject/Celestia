// simulation.h
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _SIMULATION_H_
#define _SIMULATION_H_

#include <vector>

#include "vecmath.h"
#include "quaternion.h"
#include "texture.h"
#include "mesh.h"
#include "stardb.h"
#include "solarsys.h"
#include "astro.h"
#include "galaxy.h"
#include "texmanager.h"
#include "render.h"


class Simulation
{
 public:
    Simulation();
    ~Simulation();

    double getTime() const; // Time in seconds
    void setTime(double t);

    void update(double dt);
    void render(Renderer&);

    Selection pickObject(Vec3f pickRay);

    StarDatabase* getStarDatabase() const;
    SolarSystemCatalog* getSolarSystemCatalog() const;
    void setStarDatabase(StarDatabase* db,
                         SolarSystemCatalog* catalog,
                         GalaxyList*);

    Observer& getObserver();

    void orbit(Quatf q);
    void rotate(Quatf q);
    void changeOrbitDistance(float d);
    void setTargetSpeed(float s);
    float getTargetSpeed();

    Selection getSelection() const;
    void setSelection(const Selection&);
    void selectStar(uint32);
    void selectPlanet(int);
    UniversalCoord getSelectionPosition(Selection& sel, double when);
    Selection findObject(std::string s);
    Selection findObjectFromPath(std::string s);
    void gotoSelection(double gotoTime,
                       Vec3f up, astro::CoordinateSystem upFrame);
    void gotoSelection(double gotoTime, double distance,
                       Vec3f up, astro::CoordinateSystem upFrame);
    void gotoSelectionLongLat(double gotoTime,
                              double distance,
                              float longitude, float latitude,
                              Vec3f up);
    void centerSelection(double centerTime = 0.5);
    void follow();
    void geosynchronousFollow();
    void track();
    void cancelMotion();

    SolarSystem* getNearestSolarSystem() const;

    double getTimeScale();
    void setTimeScale(double);

    float getFaintestVisible() const;
    void setFaintestVisible(float);

    int getHUDDetail() const;
    void setHUDDetail(int);

    enum ObserverMode {
        Free                    = 0,
        Travelling              = 1,
        Following               = 2,
        GeosynchronousFollowing = 3,
        Tracking                = 4,
    };

    void setObserverMode(ObserverMode);
    ObserverMode getObserverMode() const;

 private:
    // Class-specific types
    typedef struct 
    {
        double duration;
        double startTime;
        UniversalCoord from;
        UniversalCoord to;
        UniversalCoord initialFocus;
        UniversalCoord finalFocus;
        Quatf initialOrientation;
        Quatf finalOrientation;
        Vec3f up;
    } JourneyParams;

    typedef struct
    {
        Body* body;
        Star* sun;
        Vec3d offset;
        Quatd offsetR;
    } FollowParams;

 private:
    SolarSystem* getSolarSystem(Star* star);
    Star* getSun(Body* body);
    Selection pickPlanet(Observer& observer,
                         Star& sun,
                         SolarSystem& solarSystem,
                         Vec3f pickRay);
    Selection pickStar(Vec3f pickRay);
    void computeGotoParameters(Selection& sel,
                               JourneyParams& jparams,
                               double gotoTime,
                               Vec3d offset, astro::CoordinateSystem offsetFrame,
                               Vec3f up, astro::CoordinateSystem upFrame);
    void computeCenterParameters(Selection& sel, JourneyParams& jparams, double centerTime);

 private:
    double realTime;
    double simTime;
    double timeScale;

    StarDatabase* stardb;
    SolarSystemCatalog* solarSystemCatalog;
    GalaxyList* galaxies;

    SolarSystem* closestSolarSystem;
    Star* selectedStar;
    Body* selectedBody;
    Selection selection;
    int selectedPlanet;

    Observer observer;

    double targetSpeed;
    Vec3d targetVelocity;
    Vec3d initialVelocity;
    double beginAccelTime;

    ObserverMode observerMode;
    JourneyParams journey;
    FollowParams followInfo;

    float faintestVisible;

    int hudDetail;
};

#endif // SIMULATION_H_
