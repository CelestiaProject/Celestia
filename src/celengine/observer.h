// observer.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Because of the vastness of interstellar space, floats and doubles aren't
// sufficient when we need to represent distances to millimeter accuracy.
// BigFix is a high precision (128 bit) fixed point type used to represent
// the position of an observer in space.  However, it's not practical to use
// high-precision numbers for the positions of everything.  To get around
// this problem, object positions are stored at two different scales--light
// years for stars, and kilometers for objects within a star system.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_OBSERVER_H_
#define _CELENGINE_OBSERVER_H_

#include <celengine/frame.h>

class Observer
{
public:
    Observer();

    // The getPosition method returns the observer's position in micro light
    // years.
    UniversalCoord getPosition() const;

    // getRelativePosition returns in units of kilometers the difference
    // between the position of the observer and a location specified in
    // light years.
    Point3d       getRelativePosition(const Point3d&) const;

    Quatf         getOrientation() const;
    void          setOrientation(const Quatf&);
    void          setOrientation(const Quatd&);
    Vec3d         getVelocity() const;
    void          setVelocity(const Vec3d&);
    Vec3f         getAngularVelocity() const;
    void          setAngularVelocity(const Vec3f&);

    void          setPosition(BigFix x, BigFix y, BigFix z);
    void          setPosition(const UniversalCoord&);
    void          setPosition(const Point3d&);

    RigidTransform getSituation() const;
    void           setSituation(const RigidTransform&);

    float          getFOV() const;
    void           setFOV(float);

    void           update(double dt, double timeScale);

    Vec3f          getPickRay(float x, float y) const;


    void orbit(const Selection&, Quatf q);
    void rotate(Quatf q);
    void changeOrbitDistance(const Selection&, float d);
    void setTargetSpeed(float s);
    float getTargetSpeed();

    Selection getTrackedObject() const;
    void setTrackedObject(const Selection&);

    const std::string& getDisplayedSurface() const;
    void setDisplayedSurface(const std::string&);
    
    uint32 getLocationFilter() const;
    void setLocationFilter(uint32);

    void gotoSelection(const Selection&,
                       double gotoTime,
                       Vec3f up,
                       astro::CoordinateSystem upFrame);
    void gotoSelection(const Selection&,
                       double gotoTime,
                       double startInter,
                       double endInter,
                       Vec3f up,
                       astro::CoordinateSystem upFrame);
    void gotoSelectionGC(const Selection&,
                         double gotoTime,
                         double startInter,
                         double endInter,
                         Vec3f up,
                         astro::CoordinateSystem upFrame);
    void gotoSelection(const Selection&,
                       double gotoTime,
                       double distance,
                       Vec3f up,
                       astro::CoordinateSystem upFrame);
    void gotoSelectionLongLat(const Selection&,
                              double gotoTime,
                              double distance,
                              float longitude, float latitude,
                              Vec3f up);
    void gotoLocation(const RigidTransform& transform,
                      double duration);
    void getSelectionLongLat(const Selection&,
                             double& distance,
                             double& longitude,
                             double& latitude);
    void gotoSelectionGC(const Selection& selection,
                         double gotoTime,
                         double distance,
                         Vec3f up,
                         astro::CoordinateSystem upFrame);
    void gotoSurface(const Selection&, double duration);
    void centerSelection(const Selection&, double centerTime = 0.5);
    void centerSelectionCO(const Selection&, double centerTime = 0.5);
    void follow(const Selection&);
    void geosynchronousFollow(const Selection&);
    void phaseLock(const Selection&);
    void chase(const Selection&);
    void cancelMotion();

    void reverseOrientation();

    void setFrame(const FrameOfReference&);
    FrameOfReference getFrame() const;

    double getArrivalTime() const;

    double getTime() const;
    void setTime(double);

    enum ObserverMode {
        Free                    = 0,
        Travelling              = 1,
    };

    ObserverMode getMode() const;
    void setMode(ObserverMode);

    enum TrajectoryType {
        Linear        = 0,
        GreatCircle   = 1,
        CircularOrbit = 2,          
    };

    struct JourneyParams
    {
        double duration;
        double startTime;
        UniversalCoord from;
        UniversalCoord to;
        Quatf initialOrientation;
        Quatf finalOrientation;
        double startInterpolation; // start of orientation interpolation phase [0-1]
        double endInterpolation;   // end of orientation interpolation phase [0-1]
        double expFactor;
        double accelTime;
        Quatf rotation1; // rotation on the CircularOrbit around centerObject

        Selection centerObject;
        
        TrajectoryType traj;
    };

    void gotoJourney(const JourneyParams&);
    // void setSimulation(Simulation* _sim) { sim = _sim; };

 private:
    void computeGotoParameters(const Selection& sel,
                               JourneyParams& jparams,
                               double gotoTime,
                               double startInter,
                               double endInter,
                               Vec3d offset,
                               astro::CoordinateSystem offsetFrame,
                               Vec3f up,
                               astro::CoordinateSystem upFrame);
    void computeGotoParametersGC(const Selection& sel,
                                 JourneyParams& jparams,
                                 double gotoTime,
                                 double startInter,
                                 double endInter,
                                 Vec3d offset,
                                 astro::CoordinateSystem offsetFrame,
                                 Vec3f up,
                                 astro::CoordinateSystem upFrame,
                                 const Selection& centerObj);
    void computeCenterParameters(const Selection& sel,
                                 JourneyParams& jparams,
                                 double centerTime);
    void computeCenterCOParameters(const Selection& sel,
                                   JourneyParams& jparams,
                                   double centerTime);

 private:
    double         simTime;

    RigidTransform situation;
    Vec3d          velocity;
    Vec3f          angularVelocity;

    double         realTime;

    double         targetSpeed;
    Vec3d          targetVelocity;
    Vec3d          initialVelocity;
    double         beginAccelTime;

    ObserverMode     observerMode;
    JourneyParams    journey;
    FrameOfReference frame;
    Selection        trackObject;

    Quatf trackingOrientation;   // orientation prior to selecting tracking

    float fov;
    bool reverseFlag;

    uint32 locationFilter;
    std::string displayedSurface;
};

#endif // _CELENGINE_OBSERVER_H_
