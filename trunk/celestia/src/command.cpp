// command.cpp
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
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

void CommandWait::process(Simulation* sim, Renderer* renderer, double t, double dt)
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

void CommandSelect::process(Simulation* sim, Renderer* renderer)
{
    sim->selectBody(target);
}



////////////////
// Goto command: go to the selected body

CommandGoto::CommandGoto(double t) : gotoTime(t)
{
}

CommandGoto::~CommandGoto()
{
}

void CommandGoto::process(Simulation* sim, Renderer* renderer)
{
    sim->gotoSelection(gotoTime);
}



////////////////
// Center command: go to the selected body

CommandCenter::CommandCenter(double t) : centerTime(t)
{
}

CommandCenter::~CommandCenter()
{
}

void CommandCenter::process(Simulation* sim, Renderer* renderer)
{
    sim->centerSelection(centerTime);
}


////////////////
// Follow command: follow the selected body

CommandFollow::CommandFollow()
{
}

void CommandFollow::process(Simulation* sim, Renderer* renderer)
{
    sim->follow();
}


////////////////
// Cancel command: cancel a follow or goto command

CommandCancel::CommandCancel()
{
}

void CommandCancel::process(Simulation* sim, Renderer* renderer)
{
    sim->cancelMotion();
}


////////////////
// Print command: print text to the console

CommandPrint::CommandPrint(string _text) : text(_text)
{
}

void CommandPrint::process(Simulation* sim, Renderer* renderer)
{
}


////////////////
// Clear screen command: clear the console of all text

CommandClearScreen::CommandClearScreen()
{
}

void CommandClearScreen::process(Simulation* sim, Renderer* renderer)
{
}


////////////////
// Set time command: set the simulation time

CommandSetTime::CommandSetTime(double _jd) : jd(_jd)
{
}

void CommandSetTime::process(Simulation* sim, Renderer* renderer)
{
    sim->setTime(jd);
}



////////////////
// Set time rate command: set the simulation time rate

CommandSetTimeRate::CommandSetTimeRate(double _rate) : rate(_rate)
{
}

void CommandSetTimeRate::process(Simulation* sim, Renderer* renderer)
{
    sim->setTimeScale(rate);
}


////////////////
// Change distance command change the distance from the selected object

CommandChangeDistance::CommandChangeDistance(double _duration, double _rate) :
    TimedCommand(_duration),
    rate(_rate)
{
}

void CommandChangeDistance::process(Simulation* sim, Renderer* renderer, double t, double dt)
{
    sim->changeOrbitDistance((float) (rate * dt));
}


////////////////
// Set position command: set the position of the camera

CommandSetPosition::CommandSetPosition(const UniversalCoord& uc) : pos(uc)
{
}

void CommandSetPosition::process(Simulation* sim, Renderer* renderer)
{
    sim->getObserver().setPosition(pos);
}


////////////////
// Set position command: set the position of the camera

CommandSetOrientation::CommandSetOrientation(const Vec3f& _axis, float _angle) :
    axis(_axis), angle(_angle)
{
}

void CommandSetOrientation::process(Simulation* sim, Renderer* renderer)
{
    Quatf q(1);
    q.setAxisAngle(axis, angle);
    sim->getObserver().setOrientation(q);
}
