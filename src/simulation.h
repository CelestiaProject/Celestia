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


struct FrameOfReference
{
    FrameOfReference() :
        coordSys(astro::Universal), body(NULL), star(NULL), galaxy(NULL) {};
    FrameOfReference(astro::CoordinateSystem _coordSys, Body* _body) :
        coordSys(_coordSys), body(_body), star(NULL), galaxy(NULL) {};
    FrameOfReference(astro::CoordinateSystem _coordSys, Star* _star) :
        coordSys(_coordSys), body(NULL), star(_star), galaxy(NULL) {};
    FrameOfReference(astro::CoordinateSystem _coordSys, Galaxy* _galaxy) :
        coordSys(_coordSys), body(NULL), star(NULL), galaxy(_galaxy) {};

    astro::CoordinateSystem coordSys;
    Body* body;
    Star* star;
    Galaxy* galaxy;
};

struct RigidTransform
{
    RigidTransform() :
        translation(0.0, 0.0, 0.0), rotation(1, 0, 0, 0) {};
    RigidTransform(const UniversalCoord& uc) :
        translation(uc), rotation(1.0) {};
    RigidTransform(const UniversalCoord& uc, const Quatd& q) :
        translation(uc), rotation(q) {};
    RigidTransform(const UniversalCoord& uc, const Quatf& q) :
        translation(uc), rotation(q.w, q.x, q.y, q.z) {};
    UniversalCoord translation;
    Quatd rotation;
};

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

    enum ObserverMode {
        Free                    = 0,
        Travelling              = 1,
        Following               = 2,
        GeosynchronousFollowing = 3,
        Tracking                = 4,
    };

    void setObserverMode(ObserverMode);
    ObserverMode getObserverMode() const;

    void setFrame(astro::CoordinateSystem, const Selection&);
    FrameOfReference getFrame() const;

 private:
    struct JourneyParams
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
        double expFactor;
        double accelTime;
    };

 private:
    RigidTransform toUniversal(const FrameOfReference&,
                               const RigidTransform&,
                               double);
    RigidTransform fromUniversal(const FrameOfReference&,
                                 const RigidTransform&,
                                 double);

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

    void updateObserver();

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
    FrameOfReference frame;
    RigidTransform transform;

    float faintestVisible;
};

#endif // SIMULATION_H_
