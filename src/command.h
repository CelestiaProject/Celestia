// command.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "simulation.h"
#include "render.h"

class Command
{
 public:
    Command() {};
    virtual ~Command() {};
    virtual void process(Simulation* sim, Renderer* renderer, double t) = 0;
    virtual double getDuration() const = 0;
};

typedef std::vector<Command*> CommandSequence;


class InstantaneousCommand : public Command
{
 public:
    InstantaneousCommand() {};
    virtual ~InstantaneousCommand() {};
    virtual double getDuration() const { return 0.0; };
    virtual void process(Simulation* sim, Renderer* renderer) = 0;
    void process(Simulation* sim, Renderer* renderer, double t)
    {
        process(sim, renderer);
    };
};


class TimedCommand : public Command
{
 public:
    TimedCommand(double _duration) : duration(_duration) {};
    virtual ~TimedCommand() {};
    double getDuration() const { return duration; };

 private:
    double duration;
};


class CommandWait : public TimedCommand
{
 public:
    CommandWait(double _duration);
    ~CommandWait();
    void process(Simulation* sim, Renderer* renderer, double t);
};


class CommandSelect : public InstantaneousCommand
{
 public:
    CommandSelect(std::string _target);
    ~CommandSelect();
    void process(Simulation* sim, Renderer* renderer);

 private:
    string target;
};


class CommandGoto : public InstantaneousCommand
{
 public:
    CommandGoto(double t);
    ~CommandGoto();
    void process(Simulation* sim, Renderer* renderer);

 private:
    double gotoTime;
};


class CommandCenter : public InstantaneousCommand
{
 public:
    CommandCenter(double t);
    ~CommandCenter();
    void process(Simulation* sim, Renderer* renderer);

 private:
    double centerTime;
};

#endif // _COMMAND_H_
