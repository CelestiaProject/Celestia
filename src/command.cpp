// command.cpp
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include "astro.h"
#include "command.h"

using namespace std;


////////////////
// Wait command: a no-op with no side effect other than its duration

CommandWait::CommandWait(double _duration) : TimedCommand(_duration)
{
}

CommandWait::~CommandWait()
{
}

void CommandWait::process(ExecutionEnvironment& env, double t, double dt)
{
}


////////////////
// Select command: select a body

CommandSelect::CommandSelect(string _target) : target(_target)
{
}

CommandSelect::~CommandSelect()
{
}

void CommandSelect::process(ExecutionEnvironment& env)
{
    Selection sel = env.getSimulation()->findObjectFromPath(target);
    env.getSimulation()->setSelection(sel);
}



////////////////
// Goto command: go to the selected body

CommandGoto::CommandGoto(double t,
                         double dist,
                         Vec3f _up,
                         astro::CoordinateSystem _upFrame) :
    gotoTime(t), distance(dist), up(_up), upFrame(_upFrame)
{
}

CommandGoto::~CommandGoto()
{
}

void CommandGoto::process(ExecutionEnvironment& env)
{
    Selection sel = env.getSimulation()->getSelection();
    env.getSimulation()->gotoSelection(gotoTime,
                                       astro::kilometersToLightYears(sel.radius() * distance),
                                       up, upFrame);
}


////////////////
// GotoLongLat command: go to the selected body and hover over 

CommandGotoLongLat::CommandGotoLongLat(double t,
                                       double dist,
                                       float _longitude,
                                       float _latitude,
                                       Vec3f _up) :
    gotoTime(t),
    distance(dist),
    longitude(_longitude),
    latitude(_latitude),
    up(_up)
{
}

CommandGotoLongLat::~CommandGotoLongLat()
{
}

void CommandGotoLongLat::process(ExecutionEnvironment& env)
{
    Selection sel = env.getSimulation()->getSelection();
    env.getSimulation()->gotoSelectionLongLat(gotoTime,
                                              astro::kilometersToLightYears(sel.radius() * distance),
                                              longitude, latitude,
                                              up);
}



////////////////
// Center command: go to the selected body

CommandCenter::CommandCenter(double t) : centerTime(t)
{
}

CommandCenter::~CommandCenter()
{
}

void CommandCenter::process(ExecutionEnvironment& env)
{
    env.getSimulation()->centerSelection(centerTime);
}


////////////////
// Follow command: follow the selected body

CommandFollow::CommandFollow()
{
}

void CommandFollow::process(ExecutionEnvironment& env)
{
    env.getSimulation()->follow();
}


////////////////
// Synchronous command: maintain the current position relative to the
// surface of the currently selected object.

CommandSynchronous::CommandSynchronous()
{
}

void CommandSynchronous::process(ExecutionEnvironment& env)
{
    env.getSimulation()->geosynchronousFollow();
}


////////////////
// Cancel command: cancel a follow or goto command

CommandCancel::CommandCancel()
{
}

void CommandCancel::process(ExecutionEnvironment& env)
{
    env.getSimulation()->cancelMotion();
}


////////////////
// Print command: print text to the console

CommandPrint::CommandPrint(string _text) : text(_text)
{
}

void CommandPrint::process(ExecutionEnvironment& env)
{
    env.showText(text);
}


////////////////
// Clear screen command: clear the console of all text

CommandClearScreen::CommandClearScreen()
{
}

void CommandClearScreen::process(ExecutionEnvironment& env)
{
}


////////////////
// Set time command: set the simulation time

CommandSetTime::CommandSetTime(double _jd) : jd(_jd)
{
}

void CommandSetTime::process(ExecutionEnvironment& env)
{
    env.getSimulation()->setTime(jd);
}



////////////////
// Set time rate command: set the simulation time rate

CommandSetTimeRate::CommandSetTimeRate(double _rate) : rate(_rate)
{
}

void CommandSetTimeRate::process(ExecutionEnvironment& env)
{
    env.getSimulation()->setTimeScale(rate);
}


////////////////
// Change distance command: change the distance from the selected object

CommandChangeDistance::CommandChangeDistance(double _duration, double _rate) :
    TimedCommand(_duration),
    rate(_rate)
{
}

void CommandChangeDistance::process(ExecutionEnvironment& env, double t, double dt)
{
    env.getSimulation()->changeOrbitDistance((float) (rate * dt));
}


////////////////
// Oribt command: rotate about the selected object

CommandOrbit::CommandOrbit(double _duration, const Vec3f& axis, float rate) :
    TimedCommand(_duration),
    spin(axis * rate)
{
}

void CommandOrbit::process(ExecutionEnvironment& env, double t, double dt)
{
    float v = spin.length();
    if (v != 0.0f)
    {
        Quatf q;
        q.setAxisAngle(spin / v, (float) (v * dt));
        env.getSimulation()->orbit(q);
    }
}


CommandRotate::CommandRotate(double _duration, const Vec3f& axis, float rate) :
    TimedCommand(_duration),
    spin(axis * rate)
{
}

void CommandRotate::process(ExecutionEnvironment& env, double t, double dt)
{
    float v = spin.length();
    if (v != 0.0f)
    {
        Quatf q;
        q.setAxisAngle(spin / v, (float) (v * dt));
        env.getSimulation()->rotate(q);
    }
}


CommandMove::CommandMove(double _duration, const Vec3d& _velocity) :
    TimedCommand(_duration),
    velocity(_velocity)
{
}

void CommandMove::process(ExecutionEnvironment& env, double t, double dt)
{
    Observer obs = env.getSimulation()->getObserver();
    obs.setPosition(obs.getPosition() + (velocity * dt));
}


////////////////
// Set position command: set the position of the camera

CommandSetPosition::CommandSetPosition(const UniversalCoord& uc) : pos(uc)
{
}

void CommandSetPosition::process(ExecutionEnvironment& env)
{
    env.getSimulation()->getObserver().setPosition(pos);
}


////////////////
// Set orientation command: set the orientation of the camera

CommandSetOrientation::CommandSetOrientation(const Vec3f& _axis, float _angle) :
    axis(_axis), angle(_angle)
{
}

void CommandSetOrientation::process(ExecutionEnvironment& env)
{
    Quatf q(1);
    q.setAxisAngle(axis, angle);
    env.getSimulation()->getObserver().setOrientation(q);
    // env.getSimulation()->setOrientation(q);
}


//////////////////
// Set render flags command

CommandRenderFlags::CommandRenderFlags(int _setFlags, int _clearFlags) :
    setFlags(_setFlags), clearFlags(_clearFlags)
{
}

void CommandRenderFlags::process(ExecutionEnvironment& env)
{
    Renderer* r = env.getRenderer();
    if (r != NULL)
    {
        r->setRenderFlags(r->getRenderFlags() | setFlags);
        r->setRenderFlags(r->getRenderFlags() & ~clearFlags);
    }
}


//////////////////
// Set labels command

CommandLabels::CommandLabels(int _setFlags, int _clearFlags) :
    setFlags(_setFlags), clearFlags(_clearFlags)
{
}

void CommandLabels::process(ExecutionEnvironment& env)
{
    Renderer* r = env.getRenderer();
    if (r != NULL)
    {
        r->setLabelMode(r->getLabelMode() | setFlags);
        r->setLabelMode(r->getLabelMode() & ~clearFlags);
    }
}


////////////////
// Set limiting magnitude command

CommandSetVisibilityLimit::CommandSetVisibilityLimit(double mag) :
    magnitude(mag)
{
}

void CommandSetVisibilityLimit::process(ExecutionEnvironment& env)
{
    Renderer* r = env.getRenderer();
    if (r != NULL)
    {
        r->setBrightnessBias(0.05f);
        r->setSaturationMagnitude(1.0f);
    }
    env.getSimulation()->setFaintestVisible(magnitude);
}
