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

#define MAX_CONSTELLATIONS 100

#include <iostream>
#include <celengine/execenv.h>
#include <celengine/astro.h>
#include <celutil/color.h>


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
    void process(ExecutionEnvironment& env, double /*t*/, double /*dt*/)
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
    ~CommandWait() = default;
    void process(ExecutionEnvironment&, double t, double dt);
};


class CommandSelect : public InstantaneousCommand
{
 public:
    CommandSelect(std::string _target);
    ~CommandSelect() = default;
    void process(ExecutionEnvironment&);

 private:
    std::string target;
};


class CommandGoto : public InstantaneousCommand
{
 public:
    CommandGoto(double t, double dist,
#ifdef __CELVEC__
                Vec3f _up,
#else
                Eigen::Vector3f _up,
#endif
                ObserverFrame::CoordinateSystem _upFrame);
    ~CommandGoto() = default;
    void process(ExecutionEnvironment&);

 private:
    double gotoTime;
    double distance;
#ifdef __CELVEC__
    Vec3f up;
#else
    Eigen::Vector3f up;
#endif
    ObserverFrame::CoordinateSystem upFrame;
};


class CommandGotoLongLat : public InstantaneousCommand
{
 public:
    CommandGotoLongLat(double t,
                       double dist,
                       float _longitude, float _latitude,
#ifdef __CELVEC__
                       Vec3f _up);
#else
                       Eigen::Vector3f _up);
#endif
    ~CommandGotoLongLat() = default;
    void process(ExecutionEnvironment&);

 private:
    double gotoTime;
    double distance;
    float longitude;
    float latitude;
#ifdef __CELVEC__
    Vec3f up;
#else
    Eigen::Vector3f up;
#endif
};


class CommandGotoLocation : public InstantaneousCommand
{
 public:
    CommandGotoLocation(double t,
#ifdef __CELVEC__
                        const Point3d& translation, const Quatf& rotation);
#else
                        const Eigen::Vector3d& translation,
                        const Eigen::Quaterniond& rotation);
#endif

    ~CommandGotoLocation() = default;
    void process(ExecutionEnvironment&);

 private:
    double gotoTime;
#ifdef __CELVEC__
    Point3d translation;
    Quatf rotation;
#else
    Eigen::Vector3d translation;
    Eigen::Quaterniond rotation;
#endif
};

class CommandSetUrl : public InstantaneousCommand
{
 public:
    CommandSetUrl(std::string  _url);
    void process(ExecutionEnvironment&);

 private:
    std::string url;
};


class CommandCenter : public InstantaneousCommand
{
 public:
    CommandCenter(double t);
    ~CommandCenter() = default;
    void process(ExecutionEnvironment&);

 private:
    double centerTime;
};


class CommandFollow : public InstantaneousCommand
{
 public:
    CommandFollow() = default;
    void process(ExecutionEnvironment&);

 private:
    int dummy;   // Keep the class from having zero size
};


class CommandSynchronous : public InstantaneousCommand
{
 public:
    CommandSynchronous() = default;
    void process(ExecutionEnvironment&);

 private:
    int dummy;   // Keep the class from having zero size
};


class CommandLock : public InstantaneousCommand
{
 public:
    CommandLock() = default;
    void process(ExecutionEnvironment&);

 private:
    int dummy;
};


class CommandChase : public InstantaneousCommand
{
 public:
    CommandChase() = default;
    void process(ExecutionEnvironment&);

 private:
    int dummy;
};


class CommandTrack : public InstantaneousCommand
{
 public:
    CommandTrack() = default;
    void process(ExecutionEnvironment&);

 private:
    int dummy;
};


class CommandSetFrame : public InstantaneousCommand
{
 public:
    CommandSetFrame(ObserverFrame::CoordinateSystem,
                    std::string, std::string);
    void process(ExecutionEnvironment&);

 private:
    ObserverFrame::CoordinateSystem coordSys;
    std::string refObjectName;
    std::string targetObjectName;
};


class CommandSetSurface : public InstantaneousCommand
{
 public:
    CommandSetSurface(std::string);
    void process(ExecutionEnvironment&);

 private:
    std::string surfaceName;
};


class CommandCancel : public InstantaneousCommand
{
 public:
    CommandCancel() = default;
    void process(ExecutionEnvironment&);

 private:
    int dummy;   // Keep the class from having zero size
};


class CommandExit : public InstantaneousCommand
{
 public:
    CommandExit() = default;
    void process(ExecutionEnvironment&);

 private:
    int dummy;   // Keep the class from having zero size
};


class CommandPrint : public InstantaneousCommand
{
 public:
    CommandPrint(std::string, int horig, int vorig, int hoff, int voff,
                 double _duration);
    void process(ExecutionEnvironment&);

 private:
    std::string text;
    int hOrigin;
    int vOrigin;
    int hOffset;
    int vOffset;
    double duration;
};


class CommandClearScreen : public InstantaneousCommand
{
 public:
    CommandClearScreen() = default;
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
    CommandChangeDistance(double _duration, double _rate);
    void process(ExecutionEnvironment&, double t, double dt);

 private:
    double rate;
};


class CommandOrbit : public TimedCommand
{
 public:
#ifdef __CELVEC__
    CommandOrbit(double _duration, const Vec3f& axis, float rate);
#else
    CommandOrbit(double _duration, const Eigen::Vector3f& axis, float rate);
#endif
    void process(ExecutionEnvironment&, double t, double dt);

 private:
#ifdef __CELVEC__
    Vec3f spin;
#else
    Eigen::Vector3f spin;
#endif
};


class CommandRotate : public TimedCommand
{
 public:
#ifdef __CELVEC__
    CommandRotate(double _duration, const Vec3f& axis, float rate);
#else
    CommandRotate(double _duration, const Eigen::Vector3f& axis, float rate);
#endif
    void process(ExecutionEnvironment&, double t, double dt);

 private:
#ifdef __CELVEC__
    Vec3f spin;
#else
    Eigen::Vector3f spin;
#endif
};


class CommandMove : public TimedCommand
{
 public:
#ifdef __CELVEC__
    CommandMove(double _duration, const Vec3d& _velocity);
#else
    CommandMove(double _duration, const Eigen::Vector3d& _velocity);
#endif
    void process(ExecutionEnvironment&, double t, double dt);

 private:
#ifdef __CELVEC__
    Vec3d velocity;
#else
    Eigen::Vector3d velocity;
#endif
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
#ifdef __CELVEC__
    CommandSetOrientation(const Vec3f&, float);
#else
    CommandSetOrientation(const Eigen::Quaternionf&);
#endif
    void process(ExecutionEnvironment&);

 private:
#ifdef __CELVEC__
    Vec3f axis;
    float angle;
#else
    Eigen::Quaternionf orientation;
#endif
};

class CommandLookBack : public InstantaneousCommand
{
 public:
    CommandLookBack() = default;
    void process(ExecutionEnvironment&);

 private:
    int dummy;   // Keep the class from having zero size
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


class CommandConstellations : public InstantaneousCommand
{
    struct Flags
    {
        bool none:1;
        bool all:1;
    };

 public:
    CommandConstellations() = default;
    void process(ExecutionEnvironment&);
    void setValues(string cons, int act);

    Flags flags { false, false };

 private:
    struct Cons
    {
       std::string name;
       int active;
    };
    std::vector<Cons> constellations;
};


class CommandConstellationColor : public InstantaneousCommand
{
    struct Flags
    {
        bool none:1;
        bool all:1;
        bool unset:1;
    };

 public:
    CommandConstellationColor() = default;
    void process(ExecutionEnvironment&);
    void setConstellations(string cons);
    void setColor(float r, float g, float b);
    void unsetColor();

    Flags flags { false, false, false};

 private:
    std::vector<std::string> constellations;
    Color rgb;
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


class CommandOrbitFlags : public InstantaneousCommand
{
 public:
    CommandOrbitFlags(int _setFlags, int _clearFlags);
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

class CommandSetFaintestAutoMag45deg : public InstantaneousCommand
{
 public:
    CommandSetFaintestAutoMag45deg(double);
    void process(ExecutionEnvironment&);

 private:
    double magnitude;
};

class CommandSetAmbientLight : public InstantaneousCommand
{
 public:
    CommandSetAmbientLight(float);
    void process(ExecutionEnvironment&);

 private:
    float lightLevel;
};


class CommandSetGalaxyLightGain : public InstantaneousCommand
{
 public:
    CommandSetGalaxyLightGain(float);
    void process(ExecutionEnvironment&);

 private:
    float lightGain;
};


class CommandSet : public InstantaneousCommand
{
 public:
    CommandSet(std::string, double);
    void process(ExecutionEnvironment&);

 private:
    std::string name;
    double value;
};


class CommandPreloadTextures : public InstantaneousCommand
{
 public:
    CommandPreloadTextures(std::string);
    void process(ExecutionEnvironment&);

 private:
    std::string name;
};


class CommandMark : public InstantaneousCommand
{
 public:
    CommandMark(std::string, MarkerRepresentation, bool);
    void process(ExecutionEnvironment&);

 private:
    std::string target;
    MarkerRepresentation rep;
    bool occludable;
};


class CommandUnmark : public InstantaneousCommand
{
 public:
    CommandUnmark(std::string);
    void process(ExecutionEnvironment&);

 private:
    std::string target;
};


class CommandUnmarkAll : public InstantaneousCommand
{
 public:
    CommandUnmarkAll() = default;
    void process(ExecutionEnvironment&);

 private:
    int dummy;   // Keep the class from having zero size
};


class CommandCapture : public InstantaneousCommand
{
 public:
    CommandCapture(std::string, std::string);
    void process(ExecutionEnvironment&);

 private:
    std::string type;
    std::string filename;
};


class CommandSetTextureResolution : public InstantaneousCommand
{
 public:
    CommandSetTextureResolution(unsigned int);
    void process(ExecutionEnvironment&);

 private:
    unsigned int res;
};


class CommandRenderPath : public InstantaneousCommand
{
 public:
    CommandRenderPath(GLContext::GLRenderPath);
    void process(ExecutionEnvironment&);

 private:
    GLContext::GLRenderPath path;
};


class CommandSplitView : public InstantaneousCommand
{
 public:
    CommandSplitView(unsigned int, std::string, double);
    void process(ExecutionEnvironment&);

 private:
    unsigned int view;
    std::string splitType;
    double splitPos;
};


class CommandDeleteView : public InstantaneousCommand
{
 public:
    CommandDeleteView(unsigned int);
    void process(ExecutionEnvironment&);

 private:
    unsigned int view;
};


class CommandSingleView : public InstantaneousCommand
{
 public:
    CommandSingleView() = default;
    void process(ExecutionEnvironment&);
};


class CommandSetActiveView : public InstantaneousCommand
{
 public:
    CommandSetActiveView(unsigned int);
    void process(ExecutionEnvironment&);

 private:
    unsigned int view;
};


class CommandSetRadius : public InstantaneousCommand
{
 public:
    CommandSetRadius(std::string, double);
    void process(ExecutionEnvironment&);

 private:
    std::string object;
    double radius;
};


class CommandSetLineColor : public InstantaneousCommand
{
 public:
    CommandSetLineColor(std::string, Color);
    void process(ExecutionEnvironment&);

 private:
    std::string item;
    Color color;
};


class CommandSetLabelColor : public InstantaneousCommand
{
 public:
    CommandSetLabelColor(std::string, Color);
    void process(ExecutionEnvironment&);

 private:
    std::string item;
    Color color;
};


class CommandSetTextColor : public InstantaneousCommand
{
 public:
    CommandSetTextColor(Color);
    void process(ExecutionEnvironment&);

 private:
    Color color;
};


class Execution;

class RepeatCommand : public Command
{
 public:
    RepeatCommand(CommandSequence* _body, int _repeatCount);
    ~RepeatCommand();
    void process(ExecutionEnvironment&, double t, double dt) = 0;
    double getDuration() const;

 private:
    CommandSequence* body;
    double bodyDuration{ 0.0 };
    int repeatCount;
    Execution* execution{ nullptr };
};


class CommandScriptImage : public InstantaneousCommand
{
 public:
    CommandScriptImage(float _duration, float _xoffset, float _yoffset,
                       float _alpha, std::string, bool _fitscreen);
    void process(ExecutionEnvironment&);

 private:
    double duration;
    float xoffset;
    float yoffset;
    float alpha;
    std::string filename;
    int fitscreen;
};

class CommandVerbosity : public InstantaneousCommand
{
 public:
    CommandVerbosity(int _level);
    void process(ExecutionEnvironment&);

 private:
    int level;
};


class CommandSetWindowBordersVisible : public InstantaneousCommand
{
 public:
    CommandSetWindowBordersVisible(bool _visible) : visible(_visible) {};
    void process(ExecutionEnvironment&);

 private:
    bool visible;
};


class CommandSetRingsTexture : public InstantaneousCommand
{
 public:
    CommandSetRingsTexture(std::string, std::string, std::string);
    void process(ExecutionEnvironment&);

 private:
    std::string object, textureName, path;
};

#endif // _COMMAND_H_
