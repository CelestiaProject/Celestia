// command.cpp
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include <celutil/util.h>
#include <celestia/celestiacore.h>
#include <celestia/imagecapture.h>
#include "astro.h"
#include "command.h"
#include "execution.h"
#include "glcontext.h"

using namespace std;


////////////////
// Wait command: a no-op with no side effect other than its duration

CommandWait::CommandWait(double _duration) : TimedCommand(_duration)
{
}

CommandWait::~CommandWait()
{
}

void CommandWait::process(ExecutionEnvironment&, double, double)
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


/////////////////////////////
// GotoLocation

CommandGotoLocation::CommandGotoLocation(double t,
                                         const Point3d& _translation,
                                         const Quatf& _rotation) :
    gotoTime(t), translation(_translation), rotation(_rotation)
{
}

CommandGotoLocation::~CommandGotoLocation()
{
}

void CommandGotoLocation::process(ExecutionEnvironment& env)
{
    RigidTransform to;
    to.rotation = Quatd(rotation.w, rotation.x, rotation.y, rotation.z);
    to.translation = translation;
    env.getSimulation()->gotoLocation(to, gotoTime);
}

/////////////////////////////
// SetUrl

CommandSetUrl::CommandSetUrl(const std::string& _url) :
    url(_url)
{
}

void CommandSetUrl::process(ExecutionEnvironment& env)
{
    env.getCelestiaCore()->goToUrl(url);
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
// Chase command:

CommandChase::CommandChase()
{
}

void CommandChase::process(ExecutionEnvironment& env)
{
    env.getSimulation()->chase();
}


////////////////
// Track command:

CommandTrack::CommandTrack()
{
}

void CommandTrack::process(ExecutionEnvironment& env)
{
    env.getSimulation()->setTrackedObject(env.getSimulation()->getSelection());
}


////////////////
// Lock command:

CommandLock::CommandLock()
{
}

void CommandLock::process(ExecutionEnvironment& env)
{
    env.getSimulation()->phaseLock();
}



////////////////
// Setframe command

CommandSetFrame::CommandSetFrame(astro::CoordinateSystem _coordSys,
                                 const string& refName,
                                 const string& targetName) :
    coordSys(_coordSys), refObjectName(refName), targetObjectName(targetName)
{
}

void CommandSetFrame::process(ExecutionEnvironment& env)
{
    Selection ref = env.getSimulation()->findObjectFromPath(refObjectName);
    Selection target;
    if (coordSys == astro::PhaseLock)
        target = env.getSimulation()->findObjectFromPath(targetObjectName);
    env.getSimulation()->setFrame(FrameOfReference(coordSys, ref, target));
}


////////////////
// SetSurface command: select an alternate surface to show

CommandSetSurface::CommandSetSurface(const string& _surfaceName) :
    surfaceName(_surfaceName)
{
}

void CommandSetSurface::process(ExecutionEnvironment& env)
{
    env.getSimulation()->getActiveObserver()->setDisplayedSurface(surfaceName);
}


////////////////
// Cancel command: stop all motion, set the coordinate system to absolute,
//                 and cancel any tracking

CommandCancel::CommandCancel()
{
}

void CommandCancel::process(ExecutionEnvironment& env)
{
    env.getSimulation()->cancelMotion();
    env.getSimulation()->setFrame(FrameOfReference());
    env.getSimulation()->setTrackedObject(Selection());
}


////////////////
// Print command: print text to the console

CommandPrint::CommandPrint(string _text,
                           int horig, int vorig, int hoff, int voff,
                           double _duration
                           ) : text(_text),
                               hOrigin(horig), vOrigin(vorig),
                               hOffset(hoff), vOffset(voff),
                               duration(_duration)
{
}

void CommandPrint::process(ExecutionEnvironment& env)
{
    env.showText(text, hOrigin, vOrigin, hOffset, vOffset, duration);
}


////////////////
// Clear screen command: clear the console of all text

CommandClearScreen::CommandClearScreen()
{
}

void CommandClearScreen::process(ExecutionEnvironment&)
{
}


////////////////
// Exit command: quit the program

CommandExit::CommandExit()
{
}

void CommandExit::process(ExecutionEnvironment&)
{
    exit(0);
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

void CommandChangeDistance::process(ExecutionEnvironment& env, double, double dt)
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

void CommandOrbit::process(ExecutionEnvironment& env, double, double dt)
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

void CommandRotate::process(ExecutionEnvironment& env, double, double dt)
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

void CommandMove::process(ExecutionEnvironment& env, double, double dt)
{
    env.getSimulation()->setObserverPosition(env.getSimulation()->getObserver().getPosition() + (velocity * dt));
}


////////////////
// Set position command: set the position of the camera

CommandSetPosition::CommandSetPosition(const UniversalCoord& uc) : pos(uc)
{
}

void CommandSetPosition::process(ExecutionEnvironment& env)
{
    env.getSimulation()->setObserverPosition(pos);
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
    env.getSimulation()->setObserverOrientation(q);
}

////////////////
// Look back command: reverse observer orientation

CommandLookBack::CommandLookBack()
{
}

void CommandLookBack::process(ExecutionEnvironment& env)
{
  env.getSimulation()->reverseObserverOrientation();
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


//////////////////
// Set orbit flags command

CommandOrbitFlags::CommandOrbitFlags(int _setFlags, int _clearFlags) :
    setFlags(_setFlags), clearFlags(_clearFlags)
{
}

void CommandOrbitFlags::process(ExecutionEnvironment& env)
{
    Renderer* r = env.getRenderer();
    if (r != NULL)
    {
        r->setOrbitMask(r->getOrbitMask() | setFlags);
        r->setOrbitMask(r->getOrbitMask() & ~clearFlags);
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
    env.getSimulation()->setFaintestVisible((float) magnitude);
}
////////////////
// Set FaintestAutoMag45deg command

CommandSetFaintestAutoMag45deg::CommandSetFaintestAutoMag45deg(double mag) :
    magnitude(mag)
{
}

void CommandSetFaintestAutoMag45deg::process(ExecutionEnvironment& env)
{
    Renderer* r = env.getRenderer();
    if (r != NULL)
        r->setFaintestAM45deg((float) magnitude);
}

////////////////
// Set ambient light command

CommandSetAmbientLight::CommandSetAmbientLight(float level) :
    lightLevel(level)
{
}

void CommandSetAmbientLight::process(ExecutionEnvironment& env)
{
    Renderer* r = env.getRenderer();
    if (r != NULL)
        r->setAmbientLightLevel(lightLevel);
}


////////////////
// Set command

CommandSet::CommandSet(const std::string& _name, double _value) :
    name(_name), value(_value)
{
}

void CommandSet::process(ExecutionEnvironment& env)
{
    if (compareIgnoringCase(name, "MinOrbitSize") == 0)
    {
        if (env.getRenderer() != NULL)
            env.getRenderer()->setMinimumOrbitSize((float) value);
    }
    else if (compareIgnoringCase(name, "AmbientLightLevel") == 0)
    {
        if (env.getRenderer() != NULL)
            env.getRenderer()->setAmbientLightLevel((float) value);
    }
    else if (compareIgnoringCase(name, "FOV") == 0)
    {
        if (env.getRenderer() != NULL)
            env.getSimulation()->getActiveObserver()->setFOV(degToRad((float) value));
    }
    else if (compareIgnoringCase(name, "StarDistanceLimit") == 0)
    {
        if (env.getRenderer() != NULL)
            env.getRenderer()->setDistanceLimit((float) value);
    }
    else if (compareIgnoringCase(name, "StarStyle") == 0)
    {
        // The cast from double to an enum requires an intermediate cast to int
        // Probably shouldn't be doing this at all, but other alternatives
        // are more trouble than they're worth.
        if (env.getRenderer() != NULL)
            env.getRenderer()->setStarStyle((Renderer::StarStyle) (int) value);
    }
}


////////////////
// Mark object command

CommandMark::CommandMark(const string& _target, Color _color, float _size,
                         Marker::Symbol _symbol, const string& _name) :
    target(_target),
    color(_color),
    size(_size),
    symbol(_symbol),
    name(_name)
{
}

void CommandMark::process(ExecutionEnvironment& env)
{
    Selection sel = env.getSimulation()->findObjectFromPath(target);
    if (sel.empty())
        return;

    if (env.getSimulation()->getUniverse() != NULL)
        env.getSimulation()->getUniverse()->markObject(sel, size, color, symbol, 1, name);
}



////////////////
// Unmark object command

CommandUnmark::CommandUnmark(const string& _target) :
    target(_target)
{
}

void CommandUnmark::process(ExecutionEnvironment& env)
{
    Selection sel = env.getSimulation()->findObjectFromPath(target);
    if (sel.empty())
        return;

    if (env.getSimulation()->getUniverse() != NULL)
        env.getSimulation()->getUniverse()->unmarkObject(sel, 1);
}



///////////////
// Unmarkall command - clear all current markers

CommandUnmarkAll::CommandUnmarkAll()
{
}

void CommandUnmarkAll::process(ExecutionEnvironment& env)
{
    if (env.getSimulation()->getUniverse() != NULL)
        env.getSimulation()->getUniverse()->unmarkAll();
}


////////////////
// Preload textures command

CommandPreloadTextures::CommandPreloadTextures(const string& _name) :
    name(_name)
{
}

void CommandPreloadTextures::process(ExecutionEnvironment& env)
{
    Selection target = env.getSimulation()->findObjectFromPath(name);
    if (target.body() == NULL)
        return;

    if (env.getRenderer() != NULL)
        env.getRenderer()->loadTextures(target.body());
}


////////////////
// Capture command

CommandCapture::CommandCapture(const std::string& _type,
    const std::string& _filename) : type(_type), filename(_filename)
{
}

void CommandCapture::process(ExecutionEnvironment&)
{
    bool success = false;
#ifndef MACOSX

    // Get the dimensions of the current viewport
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    if (compareIgnoringCase(type, "jpeg") == 0)
    {
        success = CaptureGLBufferToJPEG(filename,
                                        viewport[0], viewport[1],
                                        viewport[2], viewport[3]);
    }
    if (compareIgnoringCase(type, "png") == 0)
    {
        success = CaptureGLBufferToPNG(filename,
                                       viewport[0], viewport[1],
                                       viewport[2], viewport[3]);
    }
#endif
}


////////////////
// Set RenderPath command

CommandRenderPath::CommandRenderPath(GLContext::GLRenderPath _path) :
    path(_path)
{
}

void CommandRenderPath::process(ExecutionEnvironment& env)
{
    GLContext* context = env.getRenderer()->getGLContext();

    if (context != NULL)
    {
        context->setRenderPath(path);
        env.getCelestiaCore()->notifyWatchers(CelestiaCore::RenderFlagsChanged);
    }
}


///////////////
// Repeat command

RepeatCommand::RepeatCommand(CommandSequence* _body, int _repeatCount) :
    body(_body),
    bodyDuration(0.0),
    repeatCount(_repeatCount),
    execution(NULL)
{
    for (CommandSequence::const_iterator iter = body->begin();
         iter != body->end(); iter++)
    {
        bodyDuration += (*iter)->getDuration();
    }
}

RepeatCommand::~RepeatCommand()
{
    if (execution != NULL)
        delete execution;
    // delete body;
}

void RepeatCommand::process(ExecutionEnvironment& env, double t, double dt)
{
    double t0 = t - dt;
    int loop0 = (int) (t0 / bodyDuration);
    int loop1 = (int) (t / bodyDuration);

    // TODO: This is bogus . . . should not be storing a reference to an
    // execution environment.
    if (execution == NULL)
        execution = new Execution(*body, env);

    if (loop0 == loop1)
    {
        execution->tick(dt);
    }
    else
    {
        double timeLeft = (loop0 + 1) * bodyDuration - t0;
        execution->tick(timeLeft);

        for (int i = loop0 + 1; i < loop1; i++)
        {
            execution->reset(*body);
            execution->tick(bodyDuration);
        }

        execution->reset(*body);
        execution->tick(t - loop1 * bodyDuration);
    }
}

double RepeatCommand::getDuration()
{
    return bodyDuration * repeatCount;
}
