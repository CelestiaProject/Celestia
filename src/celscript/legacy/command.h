// command.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#ifdef USE_MINIAUDIO
#include <optional>
#endif
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <celcompat/filesystem.h>
#include <celengine/marker.h>
#include <celengine/observer.h>
#include <celutil/color.h>

namespace celestia::scripts
{

class ExecutionEnvironment;

class Command
{
 public:
    Command() = default;
    virtual ~Command() = default;

    virtual void process(ExecutionEnvironment&, double t, double dt) = 0;
    virtual double getDuration() const = 0;
};

using CommandSequence = std::vector<std::unique_ptr<Command>>;


class InstantaneousCommand : public Command
{
 public:
    double getDuration() const override;
    void process(ExecutionEnvironment& env, double /*t*/, double /*dt*/) final;

 protected:
    virtual void processInstantaneous(ExecutionEnvironment&) = 0;
};

class TimedCommand : public Command
{
 public:
    TimedCommand(double _duration);

    double getDuration() const override;

 private:
    double duration;
};


class CommandNoOp : public InstantaneousCommand
{
 protected:
    void processInstantaneous(ExecutionEnvironment&) override { /* no-op */ }
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

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

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

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

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

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

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

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    double gotoTime;
    Eigen::Vector3d translation;
    Eigen::Quaterniond rotation;
};

class CommandSetUrl : public InstantaneousCommand
{
 public:
    CommandSetUrl(std::string);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    std::string url;
};


class CommandCenter : public InstantaneousCommand
{
 public:
    CommandCenter(double t);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    double centerTime;
};


class CommandFollow : public InstantaneousCommand
{
 protected:
    void processInstantaneous(ExecutionEnvironment&) override;
};


class CommandSynchronous : public InstantaneousCommand
{
 protected:
    void processInstantaneous(ExecutionEnvironment&) override;
};


class CommandLock : public InstantaneousCommand
{
 protected:
    void processInstantaneous(ExecutionEnvironment&) override;
};


class CommandChase : public InstantaneousCommand
{
 protected:
    void processInstantaneous(ExecutionEnvironment&) override;
};


class CommandTrack : public InstantaneousCommand
{
 protected:
    void processInstantaneous(ExecutionEnvironment&) override;
};


class CommandSetFrame : public InstantaneousCommand
{
 public:
    CommandSetFrame(ObserverFrame::CoordinateSystem,
                    std::string, std::string);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    ObserverFrame::CoordinateSystem coordSys;
    std::string refObjectName;
    std::string targetObjectName;
};


class CommandSetSurface : public InstantaneousCommand
{
 public:
    CommandSetSurface(std::string);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    std::string surfaceName;
};


class CommandCancel : public InstantaneousCommand
{
 protected:
    void processInstantaneous(ExecutionEnvironment&) override;
};


class CommandExit : public InstantaneousCommand
{
 protected:
    void processInstantaneous(ExecutionEnvironment&) override;
};


class CommandPrint : public InstantaneousCommand
{
 public:
    CommandPrint(std::string, int horig, int vorig, int hoff, int voff,
                 float _duration);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

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
 protected:
    void processInstantaneous(ExecutionEnvironment&) override;
};


class CommandSetTime : public InstantaneousCommand
{
 public:
    CommandSetTime(double _jd);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    double jd;
};


class CommandSetTimeRate : public InstantaneousCommand
{
 public:
    CommandSetTimeRate(double);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

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

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    UniversalCoord pos;
};


class CommandSetOrientation : public InstantaneousCommand
{
 public:
    CommandSetOrientation(const Eigen::Quaternionf&);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    Eigen::Quaternionf orientation;
};

class CommandLookBack : public InstantaneousCommand
{
 protected:
    void processInstantaneous(ExecutionEnvironment&) override;
};


class CommandRenderFlags : public InstantaneousCommand
{
 public:
    CommandRenderFlags(std::uint64_t _setFlags, std::uint64_t _clearFlags);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    std::uint64_t setFlags;
    std::uint64_t clearFlags;
};


class CommandConstellations : public InstantaneousCommand
{
    struct Flags
    {
        bool none:1;
        bool all:1;
    };

 public:
    void setValues(std::string_view cons, bool act);

    Flags flags { false, false };

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

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
    void setConstellations(std::string_view cons);
    void setColor(float r, float g, float b);
    void unsetColor();

    Flags flags { false, false, false};

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    std::vector<std::string> constellations;
    Color rgb;
};


class CommandLabels : public InstantaneousCommand
{
 public:
    CommandLabels(int _setFlags, int _clearFlags);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    int setFlags;
    int clearFlags;
};


class CommandOrbitFlags : public InstantaneousCommand
{
 public:
    CommandOrbitFlags(int _setFlags, int _clearFlags);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    int setFlags;
    int clearFlags;
};


class CommandSetVisibilityLimit : public InstantaneousCommand
{
 public:
    CommandSetVisibilityLimit(double);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    double magnitude;
};

class CommandSetFaintestAutoMag45deg : public InstantaneousCommand
{
 public:
    CommandSetFaintestAutoMag45deg(double);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    double magnitude;
};

class CommandSetAmbientLight : public InstantaneousCommand
{
 public:
    CommandSetAmbientLight(float);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    float lightLevel;
};


class CommandSetGalaxyLightGain : public InstantaneousCommand
{
 public:
    CommandSetGalaxyLightGain(float);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    float lightGain;
};


class CommandSet : public InstantaneousCommand
{
 public:
    CommandSet(std::string, double);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    std::string name;
    double value;
};


class CommandPreloadTextures : public InstantaneousCommand
{
 public:
    CommandPreloadTextures(std::string);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    std::string name;
};


class CommandMark : public InstantaneousCommand
{
 public:
    CommandMark(std::string, celestia::MarkerRepresentation, bool);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    std::string target;
    celestia::MarkerRepresentation rep;
    bool occludable;
};


class CommandUnmark : public InstantaneousCommand
{
 public:
    CommandUnmark(std::string);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    std::string target;
};


class CommandUnmarkAll : public InstantaneousCommand
{
 protected:
    void processInstantaneous(ExecutionEnvironment&) override;
};


class CommandCapture : public InstantaneousCommand
{
 public:
    CommandCapture(std::string, std::string);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    std::string type;
    std::string filename;
};


class CommandSetTextureResolution : public InstantaneousCommand
{
 public:
    CommandSetTextureResolution(unsigned int);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    unsigned int res;
};


class CommandSplitView : public InstantaneousCommand
{
 public:
    CommandSplitView(unsigned int, std::string, double);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    unsigned int view;
    std::string splitType;
    double splitPos;
};


class CommandDeleteView : public InstantaneousCommand
{
 public:
    CommandDeleteView(unsigned int);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    unsigned int view;
};


class CommandSingleView : public InstantaneousCommand
{
 protected:
    void processInstantaneous(ExecutionEnvironment&) override;
};


class CommandSetActiveView : public InstantaneousCommand
{
 public:
    CommandSetActiveView(unsigned int);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    unsigned int view;
};


class CommandSetRadius : public InstantaneousCommand
{
 public:
    CommandSetRadius(std::string, double);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    std::string object;
    double radius;
};


class CommandSetLineColor : public InstantaneousCommand
{
 public:
    CommandSetLineColor(std::string, const Color&);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    std::string item;
    Color color;
};


class CommandSetLabelColor : public InstantaneousCommand
{
 public:
    CommandSetLabelColor(std::string, const Color&);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    std::string item;
    Color color;
};


class CommandSetTextColor : public InstantaneousCommand
{
 public:
    CommandSetTextColor(const Color&);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    Color color;
};


#ifdef USE_MINIAUDIO
class CommandPlay : public InstantaneousCommand
{
 public:
    CommandPlay(int channel, std::optional<float> volume, float pan, std::optional<bool> loop, const std::optional<fs::path> &filename, bool nopause);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    int channel;
    std::optional<float> volume;
    float pan;
    std::optional<bool> loop;
    std::optional<fs::path> filename;
    bool nopause;
};
#endif

class CommandScriptImage : public InstantaneousCommand
{
 public:
    CommandScriptImage(float, float, float, float, const fs::path&, bool, std::array<Color, 4>&);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

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

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    int level;
};


class CommandSetWindowBordersVisible : public InstantaneousCommand
{
 public:
    CommandSetWindowBordersVisible(bool _visible) : visible(_visible) {};

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    bool visible;
};


class CommandSetRingsTexture : public InstantaneousCommand
{
 public:
    CommandSetRingsTexture(std::string, std::string, std::string);

 protected:
    void processInstantaneous(ExecutionEnvironment&) override;

 private:
    std::string object, textureName, path;
};

} // end namespace celestia::scripts
