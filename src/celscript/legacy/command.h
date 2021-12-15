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

#include <array>
#include <iosfwd>
#include <celcompat/string_view.h>
#include <celutil/color.h>
#include "execenv.h"


class Command
{
 public:
    Command() = default;
    virtual ~Command() = default;

    virtual void process(ExecutionEnvironment&, double t, double dt) = 0;
    virtual double getDuration() const = 0;
};

typedef std::vector<Command*> CommandSequence;


class InstantaneousCommand : public Command
{
 public:
    double getDuration() const override;
    void process(ExecutionEnvironment& env, double /*t*/, double /*dt*/) override;
    virtual void process(ExecutionEnvironment&) = 0;
};

class TimedCommand : public Command
{
 public:
    TimedCommand(double _duration);

    double getDuration() const override;

 private:
    double duration;
};


class CommandWait : public TimedCommand
{
 public:
    CommandWait(double _duration);

    void process(ExecutionEnvironment&, double t, double dt) override;
};


class CommandSelect : public InstantaneousCommand
{
 public:
    CommandSelect(std::string _target);

    void process(ExecutionEnvironment&) override;

 private:
    std::string target;
};


class CommandGoto : public InstantaneousCommand
{
 public:
    CommandGoto(double t,
                double dist,
                const Eigen::Vector3f &_up,
                ObserverFrame::CoordinateSystem _upFrame);

    void process(ExecutionEnvironment&) override;

 private:
    double gotoTime;
    double distance;
    Eigen::Vector3f up;
    ObserverFrame::CoordinateSystem upFrame;
};


class CommandGotoLongLat : public InstantaneousCommand
{
 public:
    CommandGotoLongLat(double t,
                       double dist,
                       float _longitude,
                       float _latitude,
                       const Eigen::Vector3f &_up);

    void process(ExecutionEnvironment&) override;

 private:
    double gotoTime;
    double distance;
    float longitude;
    float latitude;
    Eigen::Vector3f up;
};


class CommandGotoLocation : public InstantaneousCommand
{
 public:
    CommandGotoLocation(double t,
                        const Eigen::Vector3d& translation,
                        const Eigen::Quaterniond& rotation);

    void process(ExecutionEnvironment&) override;

 private:
    double gotoTime;
    Eigen::Vector3d translation;
    Eigen::Quaterniond rotation;
};

class CommandSetUrl : public InstantaneousCommand
{
 public:
    CommandSetUrl(std::string);

    void process(ExecutionEnvironment&) override;

 private:
    std::string url;
};


class CommandCenter : public InstantaneousCommand
{
 public:
    CommandCenter(double t);
    void process(ExecutionEnvironment&) override;

 private:
    double centerTime;
};


class CommandFollow : public InstantaneousCommand
{
 public:
    void process(ExecutionEnvironment&) override;
};


class CommandSynchronous : public InstantaneousCommand
{
 public:
    void process(ExecutionEnvironment&) override;
};


class CommandLock : public InstantaneousCommand
{
 public:
    void process(ExecutionEnvironment&) override;
};


class CommandChase : public InstantaneousCommand
{
 public:
    void process(ExecutionEnvironment&) override;
};


class CommandTrack : public InstantaneousCommand
{
 public:
    void process(ExecutionEnvironment&) override;
};


class CommandSetFrame : public InstantaneousCommand
{
 public:
    CommandSetFrame(ObserverFrame::CoordinateSystem,
                    std::string, std::string);

    void process(ExecutionEnvironment&) override;

 private:
    ObserverFrame::CoordinateSystem coordSys;
    std::string refObjectName;
    std::string targetObjectName;
};


class CommandSetSurface : public InstantaneousCommand
{
 public:
    CommandSetSurface(std::string);

    void process(ExecutionEnvironment&) override;

 private:
    std::string surfaceName;
};


class CommandCancel : public InstantaneousCommand
{
 public:
    void process(ExecutionEnvironment&) override;
};


class CommandExit : public InstantaneousCommand
{
 public:
    void process(ExecutionEnvironment&) override;
};


class CommandPrint : public InstantaneousCommand
{
 public:
    CommandPrint(std::string, int horig, int vorig, int hoff, int voff,
                 float _duration);

    void process(ExecutionEnvironment&) override;

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
    void process(ExecutionEnvironment&) override;
};


class CommandSetTime : public InstantaneousCommand
{
 public:
    CommandSetTime(double _jd);

    void process(ExecutionEnvironment&) override;

 private:
    double jd;
};


class CommandSetTimeRate : public InstantaneousCommand
{
 public:
    CommandSetTimeRate(double);

    void process(ExecutionEnvironment&) override;

 private:
    double rate;
};


class CommandChangeDistance : public TimedCommand
{
 public:
    CommandChangeDistance(double _duration, double _rate);

    void process(ExecutionEnvironment&, double t, double dt) override;

 private:
    double rate;
};


class CommandOrbit : public TimedCommand
{
 public:
    CommandOrbit(double _duration, const Eigen::Vector3f& axis, float rate);

    void process(ExecutionEnvironment&, double t, double dt) override;

 private:
    Eigen::Vector3f spin;
};


class CommandRotate : public TimedCommand
{
 public:
    CommandRotate(double _duration, const Eigen::Vector3f& axis, float rate);

    void process(ExecutionEnvironment&, double t, double dt) override;

 private:
    Eigen::Vector3f spin;
};


class CommandMove : public TimedCommand
{
 public:
    CommandMove(double _duration, const Eigen::Vector3d& _velocity);

    void process(ExecutionEnvironment&, double t, double dt) override;

 private:
    Eigen::Vector3d velocity;
};


class CommandSetPosition : public InstantaneousCommand
{
 public:
    CommandSetPosition(const UniversalCoord&);

    void process(ExecutionEnvironment&) override;

 private:
    UniversalCoord pos;
};


class CommandSetOrientation : public InstantaneousCommand
{
 public:
    CommandSetOrientation(const Eigen::Quaternionf&);

    void process(ExecutionEnvironment&) override;

 private:
    Eigen::Quaternionf orientation;
};

class CommandLookBack : public InstantaneousCommand
{
 public:
    void process(ExecutionEnvironment&) override;
};


class CommandRenderFlags : public InstantaneousCommand
{
 public:
    CommandRenderFlags(uint64_t _setFlags, uint64_t _clearFlags);

    void process(ExecutionEnvironment&) override;

 private:
    uint64_t setFlags;
    uint64_t clearFlags;
};


class CommandConstellations : public InstantaneousCommand
{
    struct Flags
    {
        bool none:1;
        bool all:1;
    };

 public:
    void process(ExecutionEnvironment&) override;

    void setValues(celestia::compat::string_view cons, bool act);

    Flags flags { false, false };

 private:
    struct Cons
    {
        Cons(std::string name, bool active) :
            name(std::move(name)),
            active(active)
        {}
        std::string name;
        bool active;
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
    void process(ExecutionEnvironment&) override;
    void setConstellations(celestia::compat::string_view cons);
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

    void process(ExecutionEnvironment&) override;

 private:
    int setFlags;
    int clearFlags;
};


class CommandOrbitFlags : public InstantaneousCommand
{
 public:
    CommandOrbitFlags(int _setFlags, int _clearFlags);

    void process(ExecutionEnvironment&) override;

 private:
    int setFlags;
    int clearFlags;
};


class CommandSetVisibilityLimit : public InstantaneousCommand
{
 public:
    CommandSetVisibilityLimit(double);

    void process(ExecutionEnvironment&) override;

 private:
    double magnitude;
};

class CommandSetFaintestAutoMag45deg : public InstantaneousCommand
{
 public:
    CommandSetFaintestAutoMag45deg(double);

    void process(ExecutionEnvironment&) override;

 private:
    double magnitude;
};

class CommandSetAmbientLight : public InstantaneousCommand
{
 public:
    CommandSetAmbientLight(float);

    void process(ExecutionEnvironment&) override;

 private:
    float lightLevel;
};


class CommandSetGalaxyLightGain : public InstantaneousCommand
{
 public:
    CommandSetGalaxyLightGain(float);

    void process(ExecutionEnvironment&) override;

 private:
    float lightGain;
};


class CommandSet : public InstantaneousCommand
{
 public:
    CommandSet(std::string, double);

    void process(ExecutionEnvironment&) override;

 private:
    std::string name;
    double value;
};


class CommandPreloadTextures : public InstantaneousCommand
{
 public:
    CommandPreloadTextures(std::string);

    void process(ExecutionEnvironment&) override;

 private:
    std::string name;
};


class CommandMark : public InstantaneousCommand
{
 public:
    CommandMark(std::string, celestia::MarkerRepresentation, bool);

    void process(ExecutionEnvironment&) override;

 private:
    std::string target;
    celestia::MarkerRepresentation rep;
    bool occludable;
};


class CommandUnmark : public InstantaneousCommand
{
 public:
    CommandUnmark(std::string);

    void process(ExecutionEnvironment&) override;

 private:
    std::string target;
};


class CommandUnmarkAll : public InstantaneousCommand
{
 public:
    void process(ExecutionEnvironment&) override;
};


class CommandCapture : public InstantaneousCommand
{
 public:
    CommandCapture(std::string, std::string);
    void process(ExecutionEnvironment&) override;

 private:
    std::string type;
    std::string filename;
};


class CommandSetTextureResolution : public InstantaneousCommand
{
 public:
    CommandSetTextureResolution(unsigned int);

    void process(ExecutionEnvironment&) override;

 private:
    unsigned int res;
};


#ifdef USE_GLCONTEXT
class CommandRenderPath : public InstantaneousCommand
{
 public:
    CommandRenderPath(GLContext::GLRenderPath);
    void process(ExecutionEnvironment&) override;

 private:
    GLContext::GLRenderPath path;
};
#endif


class CommandSplitView : public InstantaneousCommand
{
 public:
    CommandSplitView(unsigned int, std::string, double);

    void process(ExecutionEnvironment&) override;

 private:
    unsigned int view;
    std::string splitType;
    double splitPos;
};


class CommandDeleteView : public InstantaneousCommand
{
 public:
    CommandDeleteView(unsigned int);

    void process(ExecutionEnvironment&) override;

 private:
    unsigned int view;
};


class CommandSingleView : public InstantaneousCommand
{
 public:
    void process(ExecutionEnvironment&) override;
};


class CommandSetActiveView : public InstantaneousCommand
{
 public:
    CommandSetActiveView(unsigned int);
    void process(ExecutionEnvironment&) override;

 private:
    unsigned int view;
};


class CommandSetRadius : public InstantaneousCommand
{
 public:
    CommandSetRadius(std::string, double);

    void process(ExecutionEnvironment&) override;

 private:
    std::string object;
    double radius;
};


class CommandSetLineColor : public InstantaneousCommand
{
 public:
    CommandSetLineColor(std::string, const Color&);

    void process(ExecutionEnvironment&) override;

 private:
    std::string item;
    Color color;
};


class CommandSetLabelColor : public InstantaneousCommand
{
 public:
    CommandSetLabelColor(std::string, const Color&);

    void process(ExecutionEnvironment&) override;

 private:
    std::string item;
    Color color;
};


class CommandSetTextColor : public InstantaneousCommand
{
 public:
    CommandSetTextColor(const Color&);

    void process(ExecutionEnvironment&) override;

 private:
    Color color;
};


class Execution;

class RepeatCommand : public Command
{
 public:
    RepeatCommand(CommandSequence* _body, int _repeatCount);
    ~RepeatCommand() override;

    double getDuration() const override;
    void process(ExecutionEnvironment& env, double t, double dt) override;

 private:
    CommandSequence* body;
    double bodyDuration{ 0.0 };
    int repeatCount;
    Execution* execution{ nullptr };
};


class CommandScriptImage : public InstantaneousCommand
{
 public:
    CommandScriptImage(float, float, float, float, const fs::path&, bool, std::array<Color, 4>&);

    void process(ExecutionEnvironment&) override;

 private:
    double duration;
    float fadeafter;
    float xoffset;
    float yoffset;
    fs::path filename;
    int fitscreen;
    std::array<Color, 4> colors;
};

class CommandVerbosity : public InstantaneousCommand
{
 public:
    CommandVerbosity(int _level);

    void process(ExecutionEnvironment&) override;

 private:
    int level;
};


class CommandSetWindowBordersVisible : public InstantaneousCommand
{
 public:
    CommandSetWindowBordersVisible(bool _visible) : visible(_visible) {};

    void process(ExecutionEnvironment&) override;

 private:
    bool visible;
};


class CommandSetRingsTexture : public InstantaneousCommand
{
 public:
    CommandSetRingsTexture(std::string, std::string, std::string);

    void process(ExecutionEnvironment&) override;

 private:
    std::string object, textureName, path;
};


class CommandLoadFragment : public InstantaneousCommand
{
 public:
    CommandLoadFragment(std::string, std::string, std::string);

    void process(ExecutionEnvironment&) override;

 private:
    std::string type, fragment, dir;
};

#endif // _COMMAND_H_
