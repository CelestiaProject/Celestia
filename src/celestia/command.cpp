// command.cpp
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include <utility>
#include <algorithm>
#include <Eigen/Geometry>
#include <celutil/util.h>
#include <celmath/mathlib.h>
#include <celengine/astro.h>
#include <celengine/asterism.h>
#include "execution.h"
#ifdef USE_GLCONTEXT
#include <celengine/glcontext.h>
#endif
#include "celestiacore.h"
#include "imagecapture.h"
#ifdef CELX
#include "celx_internal.h"
#endif
#include <celengine/stardataloader.h>
#include <celengine/dsodataloader.h>
#include <celengine/planetdataloader.h>
#include <celengine/multitexture.h>
#ifdef USE_GLCONTEXT
#include <celengine/glcontext.h>
#endif
#include "celestiacore.h"
#include "imagecapture.h"
#include "celx_internal.h"
#include "execution.h"
#include "command.h"

using namespace std;
using namespace Eigen;
using namespace celmath;


////////////////
// Wait command: a no-op with no side effect other than its duration

CommandWait::CommandWait(double _duration) : TimedCommand(_duration)
{
}

void CommandWait::process(ExecutionEnvironment& /*unused*/, double /*unused*/, double /*unused*/)
{
}


////////////////
// Select command: select a body

CommandSelect::CommandSelect(string _target) : target(std::move(_target))
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
                         Eigen::Vector3f _up,
                         ObserverFrame::CoordinateSystem _upFrame) :
    gotoTime(t), distance(dist), up(_up), upFrame(_upFrame)
{
}

void CommandGoto::process(ExecutionEnvironment& env)
{
    Selection sel = env.getSimulation()->getSelection();
    env.getSimulation()->gotoSelection(gotoTime,
                                       sel.radius() * distance,
                                       up, upFrame);
}


////////////////
// GotoLongLat command: go to the selected body and hover over

CommandGotoLongLat::CommandGotoLongLat(double t,
                                       double dist,
                                       float _longitude,
                                       float _latitude,
                                       Eigen::Vector3f _up) :
    gotoTime(t),
    distance(dist),
    longitude(_longitude),
    latitude(_latitude),
    up(_up)
{
}

void CommandGotoLongLat::process(ExecutionEnvironment& env)
{
    Selection sel = env.getSimulation()->getSelection();
    env.getSimulation()->gotoSelectionLongLat(gotoTime,
                                              sel.radius() * distance,
                                              longitude, latitude,
                                              up);
}


/////////////////////////////
// GotoLocation

CommandGotoLocation::CommandGotoLocation(double t,
                                         const Eigen::Vector3d& _translation,
                                         const Eigen::Quaterniond& _rotation) :
    gotoTime(t), translation(_translation), rotation(_rotation)
{
}

void CommandGotoLocation::process(ExecutionEnvironment& env)
{
    UniversalCoord toPosition = UniversalCoord::CreateUly(translation);
    env.getSimulation()->gotoLocation(toPosition, rotation, gotoTime);
}

/////////////////////////////
// SetUrl

CommandSetUrl::CommandSetUrl(std::string _url) :
    url(std::move(_url))
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

void CommandCenter::process(ExecutionEnvironment& env)
{
    env.getSimulation()->centerSelection(centerTime);
}


////////////////
// Follow command: follow the selected body

void CommandFollow::process(ExecutionEnvironment& env)
{
    env.getSimulation()->follow();
}


////////////////
// Synchronous command: maintain the current position relative to the
// surface of the currently selected object.

void CommandSynchronous::process(ExecutionEnvironment& env)
{
    env.getSimulation()->geosynchronousFollow();
}


////////////////
// Chase command:

void CommandChase::process(ExecutionEnvironment& env)
{
    env.getSimulation()->chase();
}


////////////////
// Track command:

void CommandTrack::process(ExecutionEnvironment& env)
{
    env.getSimulation()->setTrackedObject(env.getSimulation()->getSelection());
}


////////////////
// Lock command:

void CommandLock::process(ExecutionEnvironment& env)
{
    env.getSimulation()->phaseLock();
}



////////////////
// Setframe command

CommandSetFrame::CommandSetFrame(ObserverFrame::CoordinateSystem _coordSys,
                                 string refName,
                                 string targetName) :
    coordSys(_coordSys), refObjectName(std::move(refName)), targetObjectName(std::move(targetName))
{
}

void CommandSetFrame::process(ExecutionEnvironment& env)
{
    Selection ref = env.getSimulation()->findObjectFromPath(refObjectName);
    Selection target;
    if (coordSys == ObserverFrame::PhaseLock)
        target = env.getSimulation()->findObjectFromPath(targetObjectName);
    env.getSimulation()->setFrame(coordSys, ref, target);
}


////////////////
// SetSurface command: select an alternate surface to show

CommandSetSurface::CommandSetSurface(string _surfaceName) :
    surfaceName(std::move(_surfaceName))
{
}

void CommandSetSurface::process(ExecutionEnvironment& env)
{
    env.getSimulation()->getActiveObserver()->setDisplayedSurface(surfaceName);
}


////////////////
// Cancel command: stop all motion, set the coordinate system to absolute,
//                 and cancel any tracking

void CommandCancel::process(ExecutionEnvironment& env)
{
    env.getSimulation()->cancelMotion();
    env.getSimulation()->setFrame(ObserverFrame::Universal, Selection());
    env.getSimulation()->setTrackedObject(Selection());
}


////////////////
// Print command: print text to the console

CommandPrint::CommandPrint(string _text,
                           int horig, int vorig, int hoff, int voff,
                           double _duration
                           ) : text(std::move(_text)),
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

void CommandClearScreen::process(ExecutionEnvironment& /*unused*/)
{
}


////////////////
// Exit command: quit the program

void CommandExit::process(ExecutionEnvironment& /*unused*/)
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

void CommandChangeDistance::process(ExecutionEnvironment& env, double /*unused*/, double dt)
{
    env.getSimulation()->changeOrbitDistance((float) (rate * dt));
}


////////////////
// Orbit command: rotate about the selected object

CommandOrbit::CommandOrbit(double _duration, const Eigen::Vector3f& axis, float rate) :
    TimedCommand(_duration),
    spin(axis * rate)
{
}

void CommandOrbit::process(ExecutionEnvironment& env, double /*unused*/, double dt)
{
    float v = spin.norm();
    if (v != 0.0f)
    {
       auto q = Quaternionf(AngleAxisf((float) (v * dt), (spin / v).normalized()));
       env.getSimulation()->orbit(q);
    }
}

CommandRotate::CommandRotate(double _duration, const Eigen::Vector3f& axis, float rate) :
    TimedCommand(_duration),
    spin(axis * rate)
{
}

void CommandRotate::process(ExecutionEnvironment& env, double /*unused*/, double dt)
{
    float v = spin.norm();
    if (v != 0.0f)
    {
       auto q = Quaternionf(AngleAxisf((float) (v * dt), (spin / v).normalized()));
       env.getSimulation()->rotate(q);
    }
}


CommandMove::CommandMove(double _duration, const Eigen::Vector3d& _velocity) :
    TimedCommand(_duration),
    velocity(_velocity)
{
}

void CommandMove::process(ExecutionEnvironment& env, double /*unused*/, double dt)
{
   Eigen::Vector3d velocityKm = velocity * dt * astro::microLightYearsToKilometers(1.0);
   env.getSimulation()->setObserverPosition(env.getSimulation()->getObserver().getPosition().offsetKm(velocityKm));
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

CommandSetOrientation::CommandSetOrientation(const Quaternionf& _orientation) :
    orientation(_orientation)
{
}

void CommandSetOrientation::process(ExecutionEnvironment& env)
{
    env.getSimulation()->setObserverOrientation(orientation);
}

////////////////
// Look back command: reverse observer orientation

void CommandLookBack::process(ExecutionEnvironment& env)
{
  env.getSimulation()->reverseObserverOrientation();
}

//////////////////
// Set render flags command

CommandRenderFlags::CommandRenderFlags(uint64_t _setFlags, uint64_t _clearFlags) :
    setFlags(_setFlags), clearFlags(_clearFlags)
{
}

void CommandRenderFlags::process(ExecutionEnvironment& env)
{
    Renderer* r = env.getRenderer();
    if (r != nullptr)
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
    if (r != nullptr)
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
    if (r != nullptr)
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
    if (r != nullptr)
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
    if (r != nullptr)
        r->setAmbientLightLevel(lightLevel);
}

////////////////
// Set galaxy light gain command

CommandSetGalaxyLightGain::CommandSetGalaxyLightGain(float gain) :
    lightGain(gain)
{
}

void CommandSetGalaxyLightGain::process(ExecutionEnvironment& /* env */)
{
    Galaxy::setLightGain(lightGain);
}


////////////////
// Set command

CommandSet::CommandSet(std::string _name, double _value) :
    name(std::move(_name)), value(_value)
{
}

void CommandSet::process(ExecutionEnvironment& env)
{
    if (compareIgnoringCase(name, "MinOrbitSize") == 0)
    {
        if (env.getRenderer() != nullptr)
            env.getRenderer()->setMinimumOrbitSize((float) value);
    }
    else if (compareIgnoringCase(name, "AmbientLightLevel") == 0)
    {
        if (env.getRenderer() != nullptr)
            env.getRenderer()->setAmbientLightLevel((float) value);
    }
    else if (compareIgnoringCase(name, "FOV") == 0)
    {
        if (env.getRenderer() != nullptr)
            env.getSimulation()->getActiveObserver()->setFOV(degToRad((float) value));
    }
    else if (compareIgnoringCase(name, "StarDistanceLimit") == 0)
    {
        if (env.getRenderer() != nullptr)
            env.getRenderer()->setDistanceLimit((float) value);
    }
    else if (compareIgnoringCase(name, "StarStyle") == 0)
    {
        // The cast from double to an enum requires an intermediate cast to int
        // Probably shouldn't be doing this at all, but other alternatives
        // are more trouble than they're worth.
        if (env.getRenderer() != nullptr)
            env.getRenderer()->setStarStyle((Renderer::StarStyle) (int) value);
    }
}


////////////////
// Mark object command

CommandMark::CommandMark(string _target,
                         MarkerRepresentation _rep,
                         bool _occludable) :
    target(std::move(_target)),
    rep(_rep),
    occludable(_occludable)
{
}

void CommandMark::process(ExecutionEnvironment& env)
{
    Selection sel = env.getSimulation()->findObjectFromPath(target);
    if (sel.empty())
        return;

    if (env.getSimulation()->getUniverse() != nullptr)
    {

        env.getSimulation()->getUniverse()->markObject(sel, rep, 1, occludable);
    }
}



////////////////
// Unmark object command

CommandUnmark::CommandUnmark(string _target) :
    target(std::move(_target))
{
}

void CommandUnmark::process(ExecutionEnvironment& env)
{
    Selection sel = env.getSimulation()->findObjectFromPath(target);
    if (sel.empty())
        return;

    if (env.getSimulation()->getUniverse() != nullptr)
        env.getSimulation()->getUniverse()->unmarkObject(sel, 1);
}



///////////////
// Unmarkall command - clear all current markers

void CommandUnmarkAll::process(ExecutionEnvironment& env)
{
    if (env.getSimulation()->getUniverse() != nullptr)
        env.getSimulation()->getUniverse()->unmarkAll();
}


////////////////
// Preload textures command

CommandPreloadTextures::CommandPreloadTextures(string _name) :
    name(std::move(_name))
{
}

void CommandPreloadTextures::process(ExecutionEnvironment& env)
{
    Selection target = env.getSimulation()->findObjectFromPath(name);
    if (target.body() == nullptr)
        return;

    if (env.getRenderer() != nullptr)
        env.getRenderer()->loadTextures(target.body());
}


////////////////
// Capture command

CommandCapture::CommandCapture(std::string _type,
    std::string _filename) : type(std::move(_type)), filename(std::move(_filename))
{
}

void CommandCapture::process(ExecutionEnvironment& env)
{
#ifndef TARGET_OS_MAC
    const Renderer* r = env.getRenderer();
    if (r == nullptr)
        return;

    // Get the dimensions of the current viewport
    array<int, 4> viewport;
    r->getScreenSize(viewport);

    if (compareIgnoringCase(type, "jpeg") == 0)
    {
        CaptureGLBufferToJPEG(filename,
                              viewport[0], viewport[1],
                              viewport[2], viewport[3],
                              r);
    }
    else if (compareIgnoringCase(type, "png") == 0)
    {
        CaptureGLBufferToPNG(filename,
                             viewport[0], viewport[1],
                             viewport[2], viewport[3],
                             r);
    }
#endif
}


////////////////
// Set texture resolution command

CommandSetTextureResolution::CommandSetTextureResolution(unsigned int _res) :
    res(_res)
{
}

void CommandSetTextureResolution::process(ExecutionEnvironment& env)
{
    if (env.getRenderer() != nullptr)
    {
        env.getRenderer()->setResolution(res);
        env.getCelestiaCore()->notifyWatchers(CelestiaCore::RenderFlagsChanged);
    }
}


////////////////
// Set RenderPath command. Left for compatibility.

#ifdef USE_GLCONTEXT
CommandRenderPath::CommandRenderPath(GLContext::GLRenderPath _path) :
    path(_path)
{
}

void CommandRenderPath::process(ExecutionEnvironment& /*env*/)
{
#if 0
    GLContext* context = env.getRenderer()->getGLContext();

    if (context != nullptr)
    {
        context->setRenderPath(path);
        env.getCelestiaCore()->notifyWatchers(CelestiaCore::RenderFlagsChanged);
    }
#endif
}
#endif


////////////////
// SplitView command

CommandSplitView::CommandSplitView(unsigned int _view, string _splitType, double _splitPos) :
    view(_view),
    splitType(std::move(_splitType)),
    splitPos(_splitPos)
{
}

void CommandSplitView::process(ExecutionEnvironment& env)
{
#ifdef CELX // because of getObservers
    vector<Observer*> observer_list;
    getObservers(env.getCelestiaCore(), observer_list);

    if (view >= 1 && view <= observer_list.size())
    {
        Observer* obs = observer_list[view - 1];
        View* view = getViewByObserver(env.getCelestiaCore(), obs);
        View::Type type = (compareIgnoringCase(splitType, "h") == 0) ? View::HorizontalSplit : View::VerticalSplit;
        env.getCelestiaCore()->splitView(type, view, (float)splitPos);
    }
#endif
}


////////////////
// DeleteView command

CommandDeleteView::CommandDeleteView(unsigned int _view) :
    view(_view)
{
}

void CommandDeleteView::process(ExecutionEnvironment& env)
{
#ifdef CELX
    vector<Observer*> observer_list;
    getObservers(env.getCelestiaCore(), observer_list);

    if (view >= 1 && view <= observer_list.size())
    {
        Observer* obs = observer_list[view - 1];
        View* view = getViewByObserver(env.getCelestiaCore(), obs);
        env.getCelestiaCore()->deleteView(view);
    }
#endif
}


////////////////
// SingleView command

void CommandSingleView::process(ExecutionEnvironment& env)
{
#ifdef CELX
    View* view = getViewByObserver(env.getCelestiaCore(), env.getSimulation()->getActiveObserver());
    env.getCelestiaCore()->singleView(view);
#endif
}


////////////////
// SetActiveView command

CommandSetActiveView::CommandSetActiveView(unsigned int _view) :
    view(_view)
{
}

void CommandSetActiveView::process(ExecutionEnvironment& env)
{
#ifdef CELX
    vector<Observer*> observer_list;
    getObservers(env.getCelestiaCore(), observer_list);

    if (view >= 1 && view <= observer_list.size())
    {
        Observer* obs = observer_list[view - 1];
        View* view = getViewByObserver(env.getCelestiaCore(), obs);
        env.getCelestiaCore()->setActiveView(view);
    }
#endif
}


////////////////
// SetRadius command

CommandSetRadius::CommandSetRadius(string _object, double _radius) :
    object(std::move(_object)),
    radius(_radius)
{
}

void CommandSetRadius::process(ExecutionEnvironment& env)
{
    Selection sel = env.getSimulation()->findObjectFromPath(object);
    if (sel.body() != nullptr)
    {
        Body* body = sel.body();
        float iradius = body->getRadius();
        if ((radius > 0))
        {
            body->setSemiAxes(body->getSemiAxes() * ((float) radius / iradius));
        }

        if (body->getRings() != nullptr)
        {
            RingSystem rings(0.0f, 0.0f);
            rings = *body->getRings();
            float inner = rings.innerRadius;
            float outer = rings.outerRadius;
            rings.innerRadius = inner * (float) radius / iradius;
            rings.outerRadius = outer * (float) radius / iradius;
            body->setRings(rings);
        }
    }
}


////////////////
// SetLineColor command

CommandSetLineColor::CommandSetLineColor(string _item, Color _color) :
    item(std::move(_item)),
    color(_color)
{
}

void CommandSetLineColor::process(ExecutionEnvironment& /* env */)
{
#ifdef CELX
    if (CelxLua::LineColorMap.count(item) == 0)
    {
        cerr << "Unknown line style: " << item << "\n";
    }
    else
    {
        *CelxLua::LineColorMap[item] = color;
    }
#endif
}


////////////////
// SetLabelColor command

CommandSetLabelColor::CommandSetLabelColor(string _item, Color _color) :
    item(std::move(_item)),
    color(_color)
{
}

void CommandSetLabelColor::process(ExecutionEnvironment& /* env */)
{
#ifdef CELX
    if (CelxLua::LabelColorMap.count(item) == 0)
    {
        cerr << "Unknown label style: " << item << "\n";
    }
    else
    {
        *CelxLua::LabelColorMap[item] = color;
    }
#endif
}


////////////////
// SetTextColor command

CommandSetTextColor::CommandSetTextColor(Color _color) :
    color(_color)
{
}

void CommandSetTextColor::process(ExecutionEnvironment& env)
{
    env.getCelestiaCore()->setTextColor(color);
}


///////////////
// Repeat command

RepeatCommand::RepeatCommand(CommandSequence* _body, int _repeatCount) :
    body(_body),
    repeatCount(_repeatCount)
{
    for (const auto b : *body)
    {
        bodyDuration += b->getDuration();
    }
}

RepeatCommand::~RepeatCommand()
{

    delete execution;
    // delete body;
}

void RepeatCommand::process(ExecutionEnvironment& env, double t, double dt)
{
    double t0 = t - dt;
    auto loop0 = (int) (t0 / bodyDuration);
    auto loop1 = (int) (t / bodyDuration);

    // TODO: This is bogus . . . should not be storing a reference to an
    // execution environment.
    if (execution == nullptr)
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

double RepeatCommand::getDuration() const
{
    return bodyDuration * repeatCount;
}

// ScriptImage command
CommandScriptImage::CommandScriptImage(float _duration, float _xoffset,
                                       float _yoffset, float _alpha,
                                       std::string _filename, bool _fitscreen) :
    duration(_duration),
    xoffset(_xoffset),
    yoffset(_yoffset),
    alpha(_alpha),
    filename(std::move(_filename)),
    fitscreen(_fitscreen)
{
}

void CommandScriptImage::process(ExecutionEnvironment& env)
{
    env.getCelestiaCore()->setScriptImage(duration, xoffset, yoffset, alpha, filename, fitscreen);
}

// Verbosity command
CommandVerbosity::CommandVerbosity(int _level) :
    level(_level)
{
}

void CommandVerbosity::process(ExecutionEnvironment& env)
{
    env.getCelestiaCore()->setHudDetail(level);
}


///////////////
//

void CommandConstellations::process(ExecutionEnvironment& env)
{
    Universe* u = env.getSimulation()->getUniverse();
    if (!u)
        return;

    AsterismList& asterisms = *u->getAsterisms();
    for (const auto ast : asterisms)
    {
        if (flags.none)
        {
            ast->setActive(0);
        }
        else if (flags.all)
        {
            ast->setActive(1);
        }
        else
        {
            auto name = ast->getName(false);
            auto it = std::find_if(constellations.begin(), constellations.end(),
                                   [&name](Cons& c){ return compareIgnoringCase(c.name, name) == 0; });

            if (it != constellations.end())
                ast->setActive(it->active != 0);
        }
    }
}

void CommandConstellations::setValues(string cons, int act)
{
    // ignore all above 99 constellations
    if (constellations.size() == MAX_CONSTELLATIONS)
        return;

    std::replace(cons.begin(), cons.end(), '_', ' ');

    auto it = std::find_if(constellations.begin(), constellations.end(),
                                   [&cons](Cons& c){ return compareIgnoringCase(c.name, cons) == 0; });

    if (it != constellations.end())
        it->active = act;
    else
        constellations.push_back({cons, act}); // If not found then add a new constellation
}


void CommandConstellationColor::process(ExecutionEnvironment& env)
{
    Universe* u = env.getSimulation()->getUniverse();
    if (!u)
        return;

    AsterismList& asterisms = *u->getAsterisms();
    for (const auto ast : asterisms)
    {
        if (flags.none)
        {
            ast->unsetOverrideColor();
        }
        else if (flags.all)
        {
            ast->setOverrideColor(rgb);
        }
        else
        {
            auto name = ast->getName(false);
            //std::vector<std::string>::iterator it;
            auto it = std::find_if(constellations.begin(), constellations.end(),
                                   [&name](string& c){ return compareIgnoringCase(c, name) == 0; });

            if (it != constellations.end())
            {
                if (flags.unset)
                    ast->unsetOverrideColor();
                else
                    ast->setOverrideColor(rgb);
            }
        }
    }
}


void CommandConstellationColor::setColor(float r, float g, float b)
{
    rgb = Color(r, g, b);
    flags.unset = false;
}


void CommandConstellationColor::unsetColor()
{
    flags.unset = true;
}

void CommandConstellationColor::setConstellations(string cons)
{
    // ignore all above 99 constellations
    if (constellations.size() == MAX_CONSTELLATIONS)
       return;

    std::replace(cons.begin(), cons.end(), '_', ' ');

    // If not found then add a new constellation
    if (std::none_of(constellations.begin(), constellations.end(),
                     [&cons](string& c){ return compareIgnoringCase(c, cons) == 0; }))
        constellations.push_back(cons);
}


///////////////
// SetWindowBordersVisible command
void CommandSetWindowBordersVisible::process(ExecutionEnvironment& env)
{
    env.getCelestiaCore()->setFramesVisible(visible);
}


///////////////
// SetRingsTexture command
CommandSetRingsTexture::CommandSetRingsTexture(string _object,
                                               string _textureName,
                                               string _path) :
    object(std::move(_object)),
    textureName(std::move(_textureName)),
    path(std::move(_path))
{
}

void CommandSetRingsTexture::process(ExecutionEnvironment& env)
{
    Selection sel = env.getSimulation()->findObjectFromPath(object);
    if (sel.body() != nullptr &&
        sel.body()->getRings() != nullptr &&
        !textureName.empty())
    {
        sel.body()->getRings()->texture = MultiResTexture(textureName, path);
    }
}


///////////////
// LoadFragment command
CommandLoadFragment::CommandLoadFragment(string _type, string _fragment, string _dir) :
    type(std::move(_type)),
    fragment(std::move(_fragment)),
    dir(std::move(_dir))
{
}

void CommandLoadFragment::process(ExecutionEnvironment& env)
{
    Universe* u = env.getSimulation()->getUniverse();
    if (u == nullptr)
        return;

    istringstream in(fragment);
    if (compareIgnoringCase(type, "ssc") == 0)
    {
		SSCDataLoader loader(u, dir);
		loader.load(in);
    }
    else if (compareIgnoringCase(type, "stc") == 0)
    {
        StcDataLoader loader(&(u->getDatabase()));
        loader.resourcePath = dir;
        loader.load(in);
    }
    else if (compareIgnoringCase(type, "dsc") == 0)
    {
        DscDataLoader loader(&(u->getDatabase()));
        loader.resourcePath = dir;
        loader.load(in);
    }
}
