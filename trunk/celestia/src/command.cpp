// command.cpp
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

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

void CommandWait::process(Simulation* sim, Renderer* renderer, double t)
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


