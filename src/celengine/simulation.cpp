// simulation.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// The core of Celestia--tracks an observer moving through a
// stars and their solar systems.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "simulation.h"

#include <algorithm>
#include <cstddef>

#include <celutil/strnatcmp.h>
#include "body.h"
#include "location.h"
#include "render.h"


Simulation::Simulation(Universe* _universe) :
    universe(_universe)
{
    activeObserver = new Observer();
    observers.push_back(activeObserver);
}


Simulation::~Simulation()
{
    for (const auto observer : observers)
        delete observer;
}


static const Star* getSun(const Body* body)
{
    const PlanetarySystem* system = body->getSystem();
    return system ? system->getStar() : nullptr;
}


void Simulation::render(Renderer& renderer)
{
    renderer.render(*activeObserver,
                    *universe,
                    faintestVisible,
                    selection);
}


void Simulation::render(Renderer& renderer, Observer& observer)
{
    renderer.render(observer,
                    *universe,
                    faintestVisible,
                    selection);
}


Universe* Simulation::getUniverse() const
{
    return universe;
}


// Get the time (Julian date)
double Simulation::getTime() const
{
    return activeObserver->getTime();
}

// Set the time to the specified Julian date
void Simulation::setTime(double jd)
{
    if (syncTime)
    {
        for (const auto observer : observers)
        {
            observer->setTime(jd);
        }
    }
    else
    {
        activeObserver->setTime(jd);
    }
}


// Get the clock time elapsed since the object was created
double Simulation::getRealTime() const
{
    return realTime;
}


double Simulation::getArrivalTime() const
{
    return activeObserver->getArrivalTime();
}


// Tick the simulation by dt seconds
void Simulation::update(double dt)
{
    realTime += dt;

    for (const auto observer : observers)
    {
        observer->update(dt, timeScale);
    }

    // Reset nearest solar system
    closestSolarSystem = std::nullopt;
}


Selection Simulation::getSelection() const
{
    return selection;
}


void Simulation::setSelection(const Selection& sel)
{
    selection = sel;
}


Selection Simulation::getTrackedObject() const
{
    return activeObserver->getTrackedObject();
}


void Simulation::setTrackedObject(const Selection& sel)
{
    activeObserver->setTrackedObject(sel);
}


Selection Simulation::pickObject(const Eigen::Vector3f& pickRay,
                                 RenderFlags renderFlags,
                                 float tolerance)
{
    return universe->pick(activeObserver->getPosition(),
                          activeObserver->getOrientationf().conjugate() * pickRay,
                          activeObserver->getTime(),
                          renderFlags,
                          faintestVisible,
                          tolerance);
}

void Simulation::reverseObserverOrientation()
{
    activeObserver->reverseOrientation();
}


Observer& Simulation::getObserver()
{
    return *activeObserver;
}


const Observer& Simulation::getObserver() const
{
    return *activeObserver;
}


Observer* Simulation::duplicateActiveObserver()
{
    return observers.emplace_back(new Observer(*getActiveObserver()));
}


void Simulation::removeObserver(Observer* o)
{
    auto iter = std::find(observers.begin(), observers.end(), o);
    if (iter != observers.end())
        observers.erase(iter);
}


Observer* Simulation::getActiveObserver()
{
    return activeObserver;
}


const Observer* Simulation::getActiveObserver() const
{
    return activeObserver;
}


void Simulation::setActiveObserver(Observer* o)
{
    auto iter = std::find(observers.begin(), observers.end(), o);
    if (iter != observers.end())
        activeObserver = o;
}


void Simulation::setObserverPosition(const UniversalCoord& pos)
{
    activeObserver->setPosition(pos);
}

void Simulation::setObserverOrientation(const Eigen::Quaternionf& orientation)
{
    activeObserver->setOrientation(orientation);
}


Observer::ObserverMode Simulation::getObserverMode() const
{
    return activeObserver->getMode();
}

void Simulation::setObserverMode(Observer::ObserverMode mode)
{
    activeObserver->setMode(mode);
}

void Simulation::setFrame(ObserverFrame::CoordinateSystem coordSys,
                          const Selection& refObject,
                          const Selection& targetObject)
{
    activeObserver->setFrame(coordSys, refObject, targetObject);
}

void Simulation::setFrame(ObserverFrame::CoordinateSystem coordSys,
                          const Selection& refObject)
{
    activeObserver->setFrame(coordSys, refObject);
}

const ObserverFrame::SharedConstPtr& Simulation::getFrame() const
{
    return activeObserver->getFrame();
}

// Rotate the observer about its center.
void Simulation::rotate(const Eigen::Quaternionf& q)
{
    activeObserver->rotate(q);
}

// Orbit around the selection (if there is one.)  This involves changing
// both the observer's position and orientation.
void Simulation::orbit(const Eigen::Quaternionf& q)
{
    activeObserver->orbit(selection, q);
}

// Orbit around the selection (if there is one.)  This involves changing
// both the observer's position and orientation.
bool Simulation::orbit(const Eigen::Vector3f& from, const Eigen::Vector3f& to)
{
    return activeObserver->orbit(selection, from, to);
}

// Exponential camera dolly--move toward or away from the selected object
// at a rate dependent on the observer's distance from the object.
void Simulation::changeOrbitDistance(float d)
{
    activeObserver->changeOrbitDistance(selection, d);
}

void Simulation::scaleOrbitDistance(float scale, const std::optional<Eigen::Vector3f>& focus)
{
    activeObserver->scaleOrbitDistance(selection, scale, focus);
}

void Simulation::setTargetSpeed(float s)
{
    activeObserver->setTargetSpeed(s);
}

float Simulation::getTargetSpeed() const
{
    return activeObserver->getTargetSpeed();
}

void Simulation::gotoSelection(double gotoTime,
                               const Eigen::Vector3f& up,
                               ObserverFrame::CoordinateSystem upFrame)
{
    if (selection.getType() == SelectionType::Location)
    {
        activeObserver->gotoSelectionGC(selection,
                                        gotoTime,
                                        up, upFrame);
    }
    else
    {
        activeObserver->gotoSelection(selection, gotoTime, up, upFrame);
    }
}

void Simulation::gotoSelection(double gotoTime,
                               double distance,
                               const Eigen::Vector3f& up,
                               ObserverFrame::CoordinateSystem upCoordSys)
{
    activeObserver->gotoSelection(selection, gotoTime, distance, up, upCoordSys);
}

void Simulation::gotoSelectionLongLat(double gotoTime,
                                      double distance,
                                      float longitude,
                                      float latitude,
                                      const Eigen::Vector3f& up)
{
    activeObserver->gotoSelectionLongLat(selection, gotoTime, distance,
                                         longitude, latitude, up);
}


void Simulation::gotoLocation(const UniversalCoord& position,
                              const Eigen::Quaterniond& orientation,
                              double duration)
{
    activeObserver->gotoLocation(position, orientation, duration);
}


void Simulation::getSelectionLongLat(double& distance,
                                     double& longitude,
                                     double& latitude)
{
    activeObserver->getSelectionLongLat(selection, distance, longitude, latitude);
}


void Simulation::gotoSurface(double duration)
{
    activeObserver->gotoSurface(selection, duration);
}


void Simulation::cancelMotion()
{
    activeObserver->cancelMotion();
}

void Simulation::centerSelection(double centerTime)
{
    activeObserver->centerSelection(selection, centerTime);
}

void Simulation::centerSelectionCO(double centerTime)
{
    activeObserver->centerSelectionCO(selection, centerTime);
}

void Simulation::follow()
{
    activeObserver->follow(selection);
}

void Simulation::geosynchronousFollow()
{
    activeObserver->geosynchronousFollow(selection);
}

void Simulation::phaseLock()
{
    activeObserver->phaseLock(selection);
}

void Simulation::chase()
{
    activeObserver->chase(selection);
}


// Choose a planet around a star given it's index in the planetary system.
// The planetary system is either the system of the selected object, or the
// nearest planetary system if no object is selected.  If index is less than
// zero, pick the star.  This function should probably be in celestiacore.cpp.
void Simulation::selectPlanet(int index)
{
    if (index < 0)
    {
        if (selection.getType() == SelectionType::Body)
        {
            PlanetarySystem* system = selection.body()->getSystem();
            if (system != nullptr)
                setSelection(system->getStar());
        }
    }
    else
    {
        const Star* star = nullptr;
        if (selection.getType() == SelectionType::Star)
            star = selection.star();
        else if (selection.getType() == SelectionType::Body)
            star = getSun(selection.body());

        SolarSystem* solarSystem = nullptr;
        if (star != nullptr)
            solarSystem = universe->getSolarSystem(star);
        else
            solarSystem = getNearestSolarSystem();

        if (solarSystem != nullptr &&
            index < solarSystem->getPlanets()->getSystemSize())
        {
            setSelection(Selection(solarSystem->getPlanets()->getBody(index)));
        }
    }
}

// Find an object from a path, for example Sol/Earth/Moon or Upsilon And/b
// Currently, 'absolute' paths starting with a / are not supported nor are
// paths that contain galaxies.
Selection Simulation::findObjectFromPath(std::string_view s, bool i18n) const
{
    Selection path[2];
    std::size_t nPathEntries = 0;

    if (!selection.empty())
        path[nPathEntries++] = selection;

    if (auto nearestSolarSystem = getNearestSolarSystem(); nearestSolarSystem != nullptr)
        path[nPathEntries++] = Selection(nearestSolarSystem->getStar());

    return universe->findPath(s, {path, nPathEntries}, i18n);
}


void Simulation::getObjectCompletion(std::vector<celestia::engine::Completion>& completion,
                                     std::string_view s,
                                     bool withLocations) const
{
    Selection path[2];
    std::size_t nPathEntries = 0;

    if (!selection.empty())
    {
        if (selection.getType() == SelectionType::Location)
        {
            path[nPathEntries++] = Selection(selection.location()->getParentBody());
        }
        else
        {
            path[nPathEntries++] = selection;
        }
    }

    if (auto nearestSolarSystem = getNearestSolarSystem();
        nearestSolarSystem != nullptr && nearestSolarSystem != universe->getSolarSystem(selection))
    {
        path[nPathEntries++] = Selection(nearestSolarSystem->getStar());
    }

    universe->getCompletionPath(completion, s, {path, nPathEntries}, withLocations);

    std::sort(completion.begin(), completion.end(),
              [](const celestia::engine::Completion &s1, const celestia::engine::Completion &s2) { return strnatcmp(s1.getName(), s2.getName()) < 0; });
}


double Simulation::getTimeScale() const
{
    return pauseState?storedTimeScale:timeScale;
}

void Simulation::setTimeScale(double _timeScale)
{
    if (pauseState)
    {
        storedTimeScale = _timeScale;
    }
    else
    {
        timeScale = _timeScale;
    }
}

bool Simulation::getSyncTime() const
{
    return syncTime;
}

void Simulation::setSyncTime(bool sync)
{
    syncTime = sync;
}

bool Simulation::getPauseState() const
{
    return pauseState;
}

void Simulation::setPauseState(bool state)
{
    if (pauseState == state) return;

    pauseState = state;
    if (pauseState)
    {
        storedTimeScale = timeScale;
        timeScale = 0.0;
    }
    else
    {
        timeScale = storedTimeScale;
    }
}

// Synchronize all observers to active observer time
void Simulation::synchronizeTime()
{
    for (const auto observer : observers)
    {
        observer->setTime(activeObserver->getTime());
    }
}


float Simulation::getFaintestVisible() const
{
    return faintestVisible;
}


void Simulation::setFaintestVisible(float magnitude)
{
    faintestVisible = magnitude;
}


SolarSystem* Simulation::getNearestSolarSystem() const
{
    if (!closestSolarSystem.has_value())
        closestSolarSystem = universe->getNearestSolarSystem(activeObserver->getPosition());
    return *closestSolarSystem;
}
