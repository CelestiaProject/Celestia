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
#include "execenv.h"
#include "astro.h"


class Command
{
 public:
    Command() {};
    virtual ~Command() {};
    virtual void process(ExecutionEnvironment&, double t, double dt) = 0;
    virtual double getDuration() const = 0;
};

typedef std::vector<Command*> CommandSequence;


class InstantaneousCommand : public Command
{
 public:
    InstantaneousCommand() {};
    virtual ~InstantaneousCommand() {};
    virtual double getDuration() const { return 0.0; };
    virtual void process(ExecutionEnvironment&) = 0;
    void process(ExecutionEnvironment& env, double t, double dt)
    {
        process(env);
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
    void process(ExecutionEnvironment&, double t, double dt);
};


class CommandSelect : public InstantaneousCommand
{
 public:
    CommandSelect(std::string _target);
    ~CommandSelect();
    void process(ExecutionEnvironment&);

 private:
    std::string target;
};


class CommandGoto : public InstantaneousCommand
{
 public:
    CommandGoto(double t, double dist,
                Vec3f _up, astro::CoordinateSystem _upFrame);
    ~CommandGoto();
    void process(ExecutionEnvironment&);

 private:
    double gotoTime;
    double distance;
    Vec3f up;
    astro::CoordinateSystem upFrame;
};


class CommandGotoLongLat : public InstantaneousCommand
{
 public:
    CommandGotoLongLat::CommandGotoLongLat(double t,
                                           double dist,
                                           float _longitude, float _latitude,
                                           Vec3f _up);
    ~CommandGotoLongLat();
    void process(ExecutionEnvironment&);

 private:
    double gotoTime;
    double distance;
    float longitude;
    float latitude;
    Vec3f up;
};


class CommandCenter : public InstantaneousCommand
{
 public:
    CommandCenter(double t);
    ~CommandCenter();
    void process(ExecutionEnvironment&);

 private:
    double centerTime;
};


class CommandFollow : public InstantaneousCommand
{
 public:
    CommandFollow();
    void process(ExecutionEnvironment&);

 private:
    int dummy;   // Keep the class from having zero size
};


class CommandSynchronous : public InstantaneousCommand
{
 public:
    CommandSynchronous();
    void process(ExecutionEnvironment&);

 private:
    int dummy;   // Keep the class from having zero size
};


class CommandCancel : public InstantaneousCommand
{
 public:
    CommandCancel();
    void process(ExecutionEnvironment&);

 private:
    int dummy;   // Keep the class from having zero size
};


class CommandPrint : public InstantaneousCommand
{
 public:
    CommandPrint(std::string);
    void process(ExecutionEnvironment&);

 private:
    std::string text;
};


class CommandClearScreen : public InstantaneousCommand
{
 public:
    CommandClearScreen();
    void process(ExecutionEnvironment&);

 private:
    int dummy;   // Keep the class from having zero size
};


class CommandSetTime : public InstantaneousCommand
{
 public:
    CommandSetTime(double _jd);
    void process(ExecutionEnvironment&);

 private:
    double jd;
};


class CommandSetTimeRate : public InstantaneousCommand
{
 public:
    CommandSetTimeRate(double);
    void process(ExecutionEnvironment&);

 private:
    double rate;
};


class CommandChangeDistance : public TimedCommand
{
 public:
    CommandChangeDistance(double duration, double rate);
    void process(ExecutionEnvironment&, double t, double dt);

 private:
    double rate;
};


class CommandOrbit : public TimedCommand
{
 public:
    CommandOrbit(double _duration, const Vec3f& axis, float rate);
    void process(ExecutionEnvironment&, double t, double dt);

 private:
    Vec3f spin;
};


class CommandRotate : public TimedCommand
{
 public:
    CommandRotate(double _duration, const Vec3f& axis, float rate);
    void process(ExecutionEnvironment&, double t, double dt);

 private:
    Vec3f spin;
};


class CommandMove : public TimedCommand
{
 public:
    CommandMove(double _duration, const Vec3d& _velocity);
    void process(ExecutionEnvironment&, double t, double dt);

 private:
    Vec3d velocity;
};


class CommandSetPosition : public InstantaneousCommand
{
 public:
    CommandSetPosition(const UniversalCoord&);
    void process(ExecutionEnvironment&);

 private:
    UniversalCoord pos;
};


class CommandSetOrientation : public InstantaneousCommand
{
 public:
    CommandSetOrientation(const Vec3f&, float);
    void process(ExecutionEnvironment&);

 private:
    Vec3f axis;
    float angle;
};


class CommandRenderFlags : public InstantaneousCommand
{
 public:
    CommandRenderFlags(int _setFlags, int _clearFlags);
    void process(ExecutionEnvironment&);

 private:
    int setFlags;
    int clearFlags;
};


class CommandLabels : public InstantaneousCommand
{
 public:
    CommandLabels(int _setFlags, int _clearFlags);
    void process(ExecutionEnvironment&);

 private:
    int setFlags;
    int clearFlags;
};


class CommandSetVisibilityLimit : public InstantaneousCommand
{
 public:
    CommandSetVisibilityLimit(double);
    void process(ExecutionEnvironment&);

 private:
    double magnitude;
};



#endif // _COMMAND_H_
