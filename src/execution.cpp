// execution.cpp
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "execution.h"

using namespace std;


Execution::Execution(CommandSequence& cmd, Simulation* s, Renderer* r) :
    currentCommand(cmd.begin()),
    finalCommand(cmd.end()),
    sim(s),
    renderer(r),
    commandTime(0.0)
{
}


bool Execution::tick(double dt)
{
    while (dt > 0.0 && currentCommand != finalCommand)
    {
        Command* cmd = *currentCommand;

        double timeLeft = cmd->getDuration() - commandTime;
        if (dt >= timeLeft)
        {
            cmd->process(sim, renderer, cmd->getDuration(), timeLeft);
            dt -= timeLeft;
            commandTime = 0.0;
            currentCommand++;
        }
        else
        {
            commandTime += dt;
            cmd->process(sim, renderer, commandTime, dt);
            dt = 0.0;
        }
    }

    return currentCommand == finalCommand;
}


