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

#include <celmath/vecmath.h>
#include <celmath/quaternion.h>
#include <celengine/texture.h>
#include <celengine/mesh.h>
#include <celengine/universe.h>
#include <celengine/astro.h>
#include <celengine/galaxy.h>
#include <celengine/texmanager.h>
#include <celengine/render.h>


// TODO: Move FrameOfReference and RigidTransform out of simulation.h
// and into separate modules.  And think of a better name for RigidTransform.
struct FrameOfReference
{
    FrameOfReference() :
        coordSys(astro::Universal) {};
    FrameOfReference(astro::CoordinateSystem _coordSys, Body* _body) :
        coordSys(_coordSys), refObject(_body) {};
    FrameOfReference(astro::CoordinateSystem _coordSys, Star* _star) :
        coordSys(_coordSys), refObject(_star) {};
    FrameOfReference(astro::CoordinateSystem _coordSys, Galaxy* _galaxy) :
        coordSys(_coordSys), refObject(_galaxy) {};
    FrameOfReference(astro::CoordinateSystem _coordSys, const Selection& sel) :
        coordSys(_coordSys), refObject(sel) {};

    astro::CoordinateSystem coordSys;
    Selection refObject;
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
    Simulation(Universe*);
    ~Simulation();

    double getTime() const; // Time in seconds
    void setTime(double t);

    void update(double dt);
    void render(Renderer&);

    Selection pickObject(Vec3f pickRay, float tolerance = 0.0f);

    Universe* getUniverse() const;

    void orbit(Quatf q);
    void rotate(Quatf q);
    void changeOrbitDistance(float d);
    void setTargetSpeed(float s);
    float getTargetSpeed();

    Selection getSelection() const;
    void setSelection(const Selection&);
    Selection getTrackedObject() const;
    void setTrackedObject(const Selection&);

    void selectStar(uint32);
    void selectPlanet(int);
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
    void getSelectionLongLat(double& distance,
                             double& longitude,
                             double& latitude);
    void centerSelection(double centerTime = 0.5);
    void follow();
    void geosynchronousFollow();
    void cancelMotion();

    Observer& getObserver();
    void setObserverPosition(const UniversalCoord&);
    void setObserverOrientation(const Quatf&);

    SolarSystem* getNearestSolarSystem() const;

    double getTimeScale();
    void setTimeScale(double);

    float getFaintestVisible() const;
    void setFaintestVisible(float);

    enum ObserverMode {
        Free                    = 0,
        Travelling              = 1,
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
    SolarSystem* getSolarSystem(const Star* star);
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

    Universe* universe;

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
    Selection trackObject;

    float faintestVisible;

    Quatf trackingOrientation;   //Orientation prior to selecting tracking
};

#endif // SIMULATION_H_
