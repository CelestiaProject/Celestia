// command.cpp
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "command.h"
#include "astro.h"
#include "asterism.h"
#include "execution.h"
#include "glcontext.h"
#include <celestia/celestiacore.h>
#include <celestia/imagecapture.h>
#include <celestia/celx_internal.h>
#include <celutil/util.h>
#include <iostream>
#include "eigenport.h"

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
                         ObserverFrame::CoordinateSystem _upFrame) :
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
                                       sel.radius() * distance,
                                       toEigen(up), upFrame);
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
                                              sel.radius() * distance,
                                              longitude, latitude,
                                              toEigen(up));
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
    Quatd toOrientation = Quatd(rotation.w, rotation.x, rotation.y, rotation.z);
    UniversalCoord toPosition = UniversalCoord::CreateUly(toEigen(translation));
    env.getSimulation()->gotoLocation(toPosition, toEigen(toOrientation), gotoTime);
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

CommandSetFrame::CommandSetFrame(ObserverFrame::CoordinateSystem _coordSys,
                                 const string& refName,
                                 const string& targetName) :
    coordSys(_coordSys), refObjectName(refName), targetObjectName(targetName)
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
    env.getSimulation()->setFrame(ObserverFrame::Universal, Selection());
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
        env.getSimulation()->orbit(toEigen(q));
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
        env.getSimulation()->rotate(toEigen(q));
    }
}


CommandMove::CommandMove(double _duration, const Vec3d& _velocity) :
    TimedCommand(_duration),
    velocity(_velocity)
{
}

void CommandMove::process(ExecutionEnvironment& env, double, double dt)
{
    Eigen::Vector3d velocityKm = toEigen(velocity) * dt * astro::microLightYearsToKilometers(1.0);
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

CommandSetOrientation::CommandSetOrientation(const Vec3f& _axis, float _angle) :
    axis(_axis), angle(_angle)
{
}

void CommandSetOrientation::process(ExecutionEnvironment& env)
{
    Quatf q(1);
    q.setAxisAngle(axis, angle);
    env.getSimulation()->setObserverOrientation(toEigen(q));
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

CommandMark::CommandMark(const string& _target,
                         MarkerRepresentation _rep,
                         bool _occludable) :
    target(_target),
    rep(_rep),
    occludable(_occludable)
{
}

void CommandMark::process(ExecutionEnvironment& env)
{
    Selection sel = env.getSimulation()->findObjectFromPath(target);
    if (sel.empty())
        return;

    if (env.getSimulation()->getUniverse() != NULL)
    {

        env.getSimulation()->getUniverse()->markObject(sel, rep, 1, occludable);
    }
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
#ifndef TARGET_OS_MAC
    // Get the dimensions of the current viewport
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    if (compareIgnoringCase(type, "jpeg") == 0)
    {
        CaptureGLBufferToJPEG(filename,
                              viewport[0], viewport[1],
                              viewport[2], viewport[3]);
    }
    if (compareIgnoringCase(type, "png") == 0)
    {
        CaptureGLBufferToPNG(filename,
                             viewport[0], viewport[1],
                             viewport[2], viewport[3]);
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
    if (env.getRenderer() != NULL)
    {
        env.getRenderer()->setResolution(res);
        env.getCelestiaCore()->notifyWatchers(CelestiaCore::RenderFlagsChanged);
    }
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


////////////////
// SplitView command

CommandSplitView::CommandSplitView(unsigned int _view, const string& _splitType, double _splitPos) :
    view(_view),
    splitType(_splitType),
    splitPos(_splitPos)
{
}

void CommandSplitView::process(ExecutionEnvironment& env)
{
    vector<Observer*> observer_list;
    getObservers(env.getCelestiaCore(), observer_list);

    if (view >= 1 && view <= observer_list.size())
    {
        Observer* obs = observer_list[view - 1];
        View* view = getViewByObserver(env.getCelestiaCore(), obs);
        View::Type type = (compareIgnoringCase(splitType, "h") == 0) ? View::HorizontalSplit : View::VerticalSplit;
        env.getCelestiaCore()->splitView(type, view, (float)splitPos);
    }
}


////////////////
// DeleteView command

CommandDeleteView::CommandDeleteView(unsigned int _view) :
    view(_view)
{
}

void CommandDeleteView::process(ExecutionEnvironment& env)
{
    vector<Observer*> observer_list;
    getObservers(env.getCelestiaCore(), observer_list);

    if (view >= 1 && view <= observer_list.size())
    {
        Observer* obs = observer_list[view - 1];
        View* view = getViewByObserver(env.getCelestiaCore(), obs);
        env.getCelestiaCore()->deleteView(view);
    }
}


////////////////
// SingleView command

CommandSingleView::CommandSingleView()
{
}

void CommandSingleView::process(ExecutionEnvironment& env)
{
    View* view = getViewByObserver(env.getCelestiaCore(), env.getSimulation()->getActiveObserver());
    env.getCelestiaCore()->singleView(view);
}


////////////////
// SetActiveView command

CommandSetActiveView::CommandSetActiveView(unsigned int _view) :
    view(_view)
{
}

void CommandSetActiveView::process(ExecutionEnvironment& env)
{
    vector<Observer*> observer_list;
    getObservers(env.getCelestiaCore(), observer_list);

    if (view >= 1 && view <= observer_list.size())
    {
        Observer* obs = observer_list[view - 1];
        View* view = getViewByObserver(env.getCelestiaCore(), obs);
        env.getCelestiaCore()->setActiveView(view);
    }
}


////////////////
// SetRadius command

CommandSetRadius::CommandSetRadius(const string& _object, double _radius) :
    object(_object),
    radius(_radius)
{
}

void CommandSetRadius::process(ExecutionEnvironment& env)
{
    Selection sel = env.getSimulation()->findObjectFromPath(object);
    if (sel.body() != NULL)
    {
        Body* body = sel.body();
        float iradius = body->getRadius();
        if ((radius > 0))
        {
            body->setSemiAxes(body->getSemiAxes() * ((float) radius / iradius));
        }

        if (body->getRings() != NULL)
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

CommandSetLineColor::CommandSetLineColor(const string& _item, Color _color) :
    item(_item),
    color(_color)
{
}

void CommandSetLineColor::process(ExecutionEnvironment& /* env */)
{
    if (CelxLua::LineColorMap.count(item) == 0)
    {
        cerr << "Unknown line style: " << item << "\n";
    }
    else
    {
        *CelxLua::LineColorMap[item] = color;
    }
}


////////////////
// SetLabelColor command

CommandSetLabelColor::CommandSetLabelColor(const string& _item, Color _color) :
    item(_item),
    color(_color)
{
}

void CommandSetLabelColor::process(ExecutionEnvironment& /* env */)
{
    if (CelxLua::LabelColorMap.count(item) == 0)
    {
        cerr << "Unknown label style: " << item << "\n";
    }
    else
    {
        *CelxLua::LabelColorMap[item] = color;
    }
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

double RepeatCommand::getDuration() const
{
    return bodyDuration * repeatCount;
}

//Audio support by Victor, modified by Vincent, Leserg & Alexell
CommandPlay::CommandPlay(int _channel, float _volume, bool _loop, 
                          const std::string& _filename, bool _nopause) :
   channel(_channel),
   volume(_volume),
   loop(_loop),
   filename(_filename),
   nopause(_nopause)
{
}

void CommandPlay::process(ExecutionEnvironment& env)
{
   env.getCelestiaCore()->playSoundFile(channel, volume, loop, filename.c_str(), nopause);
}

// SCRIPT IMAGE START: Author Vincent
// ScriptImage command
CommandScriptImage::CommandScriptImage(double _duration, float _xoffset,
                         float _yoffset, float _alpha, const std::string& _filename, int _fitscreen) :
    duration(_duration),
    xoffset(_xoffset),
    yoffset(_yoffset),
    alpha(_alpha),
    filename(_filename),
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
// SCRIPT IMAGE END


//====================================================
// Add on by Javier Nieto from www.astrohenares.org
//====================================================
CommandConstellations::CommandConstellations()
{
    numConstellations = 0;
    all = 0;
    none = 0;
}

void CommandConstellations::process(ExecutionEnvironment& env)
{
    Universe* u = env.getSimulation()->getUniverse();
    if (u != NULL)
    {
        AsterismList* asterisms = u->getAsterisms();
        for (AsterismList::const_iterator iter = asterisms->begin();
             iter != asterisms->end(); iter++)
        {
            Asterism* ast = *iter;
            if (none)
            {
                ast->setActive(0);
            }
            else if (all)
            {
                ast->setActive(1);
            }
            else
            {
                for (int i = 0; i < numConstellations; i++)
                {
                    if (compareIgnoringCase(constellation[i],ast->getName(false)) == 0)
                    {
                        ast->setActive(active[i] != 0);
                        break;
                    }
                }
            }
        }
    }
}

void CommandConstellations::setValues(string cons, int act)
{
    int found = 0;
    for (unsigned int j = 0; j < cons.size(); j++)
    {
        if(cons[j] == '_')
            cons[j] = ' ';
    }

    // ignore all above 99 constellations
    if (numConstellations == MAX_CONSTELLATIONS)
        return;

    for (int i = 0; i < numConstellations; i++)
    {
        if (compareIgnoringCase(constellation[i], cons) == 0 )
        {
            active[i]=act;
            found=1;
            break;
        }
    }

    if (!found)
    {
        constellation[numConstellations]=cons;
        active[numConstellations]=act;
        numConstellations++;
    }
}


CommandConstellationColor::CommandConstellationColor()
{
    numConstellations=0;
    all=0;
    none=0;
    unset=0;
}


void CommandConstellationColor::process(ExecutionEnvironment& env)
{
    Universe* u = env.getSimulation()->getUniverse();
    if (u != NULL)
    {
        AsterismList* asterisms = u->getAsterisms();
        for (AsterismList::const_iterator iter = asterisms->begin();
             iter != asterisms->end(); iter++)
        {
            Asterism* ast = *iter;
            if (none)
            {
                ast->unsetOverrideColor();
            }
            else if (all)
            {
                ast->setOverrideColor(rgb);
            }
            else
            {
                for(int i = 0; i < numConstellations; i++)
                {
                    if (compareIgnoringCase(constellation[i],ast->getName(false)) ==0 )
                    {
                        if(unset)
                            ast->unsetOverrideColor();
                        else
                            ast->setOverrideColor(rgb);
                        break;
                    }
                }
            }
        }
    }
}


void CommandConstellationColor::setColor(float r, float g, float b)
{
    rgb = Color(r, g, b);
    unset = 0;
}


void CommandConstellationColor::unsetColor()
{
    unset = 1;
}


void CommandConstellationColor::setConstellations(string cons)
{
    int found=0;
    for (unsigned int j = 0; j < cons.size(); j++)
    {
        if (cons[j] == '_')
            cons[j] = ' ';
    }

    // ignore all above 99 constellations
    if (numConstellations == MAX_CONSTELLATIONS)
        return;

    for (int i=0; i<numConstellations; i++)
    {
        if (compareIgnoringCase(constellation[i], cons) == 0)
        {
            found=1;
            break;
        }
    }

    if (!found)
    {
        constellation[numConstellations]=cons;
        numConstellations++;
    }
}


///////////////
// SetWindowBordersVisible command
void CommandSetWindowBordersVisible::process(ExecutionEnvironment& env)
{
    env.getCelestiaCore()->setFramesVisible(visible);
}
