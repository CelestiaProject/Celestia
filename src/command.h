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

#include <iostream>
#include "simulation.h"
#include "render.h"

class Command
{
 public:
    Command() {};
    virtual ~Command() {};
    virtual void process(Simulation* sim, Renderer* renderer, double t, double dt) = 0;
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
    void process(Simulation* sim, Renderer* renderer, double t, double dt)
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
    void process(Simulation* sim, Renderer* renderer, double t, double dt);
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


class CommandFollow : public InstantaneousCommand
{
 public:
    CommandFollow();
    void process(Simulation*, Renderer*);

 private:
    int dummy;   // Keep the class from having zero size
};


class CommandCancel : public InstantaneousCommand
{
 public:
    CommandCancel();
    void process(Simulation*, Renderer*);

 private:
    int dummy;   // Keep the class from having zero size
};


class CommandPrint : public InstantaneousCommand
{
 public:
    CommandPrint(std::string);
    void process(Simulation*, Renderer*);

 private:
    std::string text;
};


class CommandClearScreen : public InstantaneousCommand
{
 public:
    CommandClearScreen();
    void process(Simulation*, Renderer*);

 private:
    int dummy;   // Keep the class from having zero size
};


class CommandSetTime : public InstantaneousCommand
{
 public:
    CommandSetTime(double _jd);
    void process(Simulation*, Renderer*);

 private:
    double jd;
};


class CommandSetTimeRate : public InstantaneousCommand
{
 public:
    CommandSetTimeRate(double);
    void process(Simulation*, Renderer*);

 private:
    double rate;
};


class CommandChangeDistance : public TimedCommand
{
 public:
    CommandChangeDistance(double duration, double rate);
    void process(Simulation*, Renderer*, double t, double dt);

 private:
    double rate;
};


class CommandSetPosition : public InstantaneousCommand
{
 public:
    CommandSetPosition(const UniversalCoord&);
    void process(Simulation*, Renderer*);

 private:
    UniversalCoord pos;
};


class CommandSetOrientation : public InstantaneousCommand
{
 public:
    CommandSetOrientation(const Vec3f&, float);
    void process(Simulation*, Renderer*);

 private:
    Vec3f axis;
    float angle;
};


#endif // _COMMAND_H_
