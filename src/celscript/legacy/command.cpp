// command.cpp
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "command.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <sstream>

#include <celengine/asterism.h>
#include <celengine/body.h>
#include <celengine/dsodb.h>
#include <celengine/galaxy.h>
#include <celengine/overlayimage.h>
#include <celengine/render.h>
#include <celengine/selection.h>
#include <celengine/simulation.h>
#include <celengine/solarsys.h>
#include <celengine/stardb.h>
#include <celengine/universe.h>
#ifdef USE_MINIAUDIO
#include <celestia/audiosession.h>
#endif
#include <celestia/celestiacore.h>
#include <celestia/view.h>
#include <celmath/mathlib.h>
#include <celutil/filetype.h>
#include <celutil/logger.h>
#include <celutil/stringutils.h>
#include "execenv.h"


using celestia::util::GetLogger;

namespace celestia::scripts
{

namespace
{
constexpr std::size_t MAX_CONSTELLATIONS = 100;
}

double InstantaneousCommand::getDuration() const
{
    return 0.0;
}

void InstantaneousCommand::process(ExecutionEnvironment& env, double /*t*/, double /*dt*/)
{
    processInstantaneous(env);
}

TimedCommand::TimedCommand(double duration) :
    duration(duration)
{
}

double TimedCommand::getDuration() const
{
    return duration;
}

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

CommandSelect::CommandSelect(std::string _target) : target(std::move(_target))
{
}

void CommandSelect::processInstantaneous(ExecutionEnvironment& env)
{
    Selection sel = env.getSimulation()->findObjectFromPath(target);
    env.getSimulation()->setSelection(sel);
}



////////////////
// Goto command: go to the selected body

CommandGoto::CommandGoto(double t,
                         double dist,
                         const Eigen::Vector3f &_up,
                         ObserverFrame::CoordinateSystem _upFrame) :
    gotoTime(t),
    distance(dist),
    up(_up),
    upFrame(_upFrame)
{
}

void CommandGoto::processInstantaneous(ExecutionEnvironment& env)
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
                                       const Eigen::Vector3f &_up) :
    gotoTime(t),
    distance(dist),
    longitude(_longitude),
    latitude(_latitude),
    up(_up)
{
}

void CommandGotoLongLat::processInstantaneous(ExecutionEnvironment& env)
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

void CommandGotoLocation::processInstantaneous(ExecutionEnvironment& env)
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

void CommandSetUrl::processInstantaneous(ExecutionEnvironment& env)
{
    env.getCelestiaCore()->goToUrl(url);
}


////////////////
// Center command: go to the selected body

CommandCenter::CommandCenter(double t) : centerTime(t)
{
}

void CommandCenter::processInstantaneous(ExecutionEnvironment& env)
{
    env.getSimulation()->centerSelection(centerTime);
}


////////////////
// Follow command: follow the selected body

void CommandFollow::processInstantaneous(ExecutionEnvironment& env)
{
    env.getSimulation()->follow();
}


////////////////
// Synchronous command: maintain the current position relative to the
// surface of the currently selected object.

void CommandSynchronous::processInstantaneous(ExecutionEnvironment& env)
{
    env.getSimulation()->geosynchronousFollow();
}


////////////////
// Chase command:

void CommandChase::processInstantaneous(ExecutionEnvironment& env)
{
    env.getSimulation()->chase();
}


////////////////
// Track command:

void CommandTrack::processInstantaneous(ExecutionEnvironment& env)
{
    env.getSimulation()->setTrackedObject(env.getSimulation()->getSelection());
}


////////////////
// Lock command:

void CommandLock::processInstantaneous(ExecutionEnvironment& env)
{
    env.getSimulation()->phaseLock();
}



////////////////
// Setframe command

CommandSetFrame::CommandSetFrame(ObserverFrame::CoordinateSystem _coordSys,
                                 std::string refName,
                                 std::string targetName) :
    coordSys(_coordSys), refObjectName(std::move(refName)), targetObjectName(std::move(targetName))
{
}

void CommandSetFrame::processInstantaneous(ExecutionEnvironment& env)
{
    Selection ref = env.getSimulation()->findObjectFromPath(refObjectName);
    Selection target;
    if (coordSys == ObserverFrame::PhaseLock)
        target = env.getSimulation()->findObjectFromPath(targetObjectName);
    env.getSimulation()->setFrame(coordSys, ref, target);
}


////////////////
// SetSurface command: select an alternate surface to show

CommandSetSurface::CommandSetSurface(std::string _surfaceName) :
    surfaceName(std::move(_surfaceName))
{
}

void CommandSetSurface::processInstantaneous(ExecutionEnvironment& env)
{
    env.getSimulation()->getActiveObserver()->setDisplayedSurface(surfaceName);
}


////////////////
// Cancel command: stop all motion, set the coordinate system to absolute,
//                 and cancel any tracking

void CommandCancel::processInstantaneous(ExecutionEnvironment& env)
{
    env.getSimulation()->cancelMotion();
    env.getSimulation()->setFrame(ObserverFrame::Universal, Selection());
    env.getSimulation()->setTrackedObject(Selection());
}


////////////////
// Print command: print text to the console

CommandPrint::CommandPrint(std::string _text,
                           int horig, int vorig, int hoff, int voff,
                           float _duration) :
    text(std::move(_text)),
    hOrigin(horig), vOrigin(vorig),
    hOffset(hoff), vOffset(voff),
    duration(_duration)
{
}

void CommandPrint::processInstantaneous(ExecutionEnvironment& env)
{
    env.showText(text, hOrigin, vOrigin, hOffset, vOffset, duration);
}


////////////////
// Clear screen command: clear the console of all text

void CommandClearScreen::processInstantaneous(ExecutionEnvironment& /*unused*/)
{
}


////////////////
// Exit command: quit the program

void CommandExit::processInstantaneous(ExecutionEnvironment& /*unused*/)
{
    exit(0);
}

////////////////
// Set time command: set the simulation time

CommandSetTime::CommandSetTime(double _jd) : jd(_jd)
{
}

void CommandSetTime::processInstantaneous(ExecutionEnvironment& env)
{
    env.getSimulation()->setTime(jd);
}



////////////////
// Set time rate command: set the simulation time rate

CommandSetTimeRate::CommandSetTimeRate(double _rate) : rate(_rate)
{
}

void CommandSetTimeRate::processInstantaneous(ExecutionEnvironment& env)
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
    env.getSimulation()->changeOrbitDistance(static_cast<float>(rate * dt));
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
       auto q = Eigen::Quaternionf(Eigen::AngleAxisf(static_cast<float>(v * dt), (spin / v).normalized()));
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
       auto q = Eigen::Quaternionf(Eigen::AngleAxisf(static_cast<float>(v * dt), (spin / v).normalized()));
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

void CommandSetPosition::processInstantaneous(ExecutionEnvironment& env)
{
    env.getSimulation()->setObserverPosition(pos);
}


////////////////
// Set orientation command: set the orientation of the camera

CommandSetOrientation::CommandSetOrientation(const Eigen::Quaternionf& _orientation) :
    orientation(_orientation)
{
}

void CommandSetOrientation::processInstantaneous(ExecutionEnvironment& env)
{
    env.getSimulation()->setObserverOrientation(orientation);
}

////////////////
// Look back command: reverse observer orientation

void CommandLookBack::processInstantaneous(ExecutionEnvironment& env)
{
  env.getSimulation()->reverseObserverOrientation();
}

//////////////////
// Set render flags command

CommandRenderFlags::CommandRenderFlags(std::uint64_t _setFlags, std::uint64_t _clearFlags) :
    setFlags(_setFlags), clearFlags(_clearFlags)
{
}

void CommandRenderFlags::processInstantaneous(ExecutionEnvironment& env)
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

void CommandLabels::processInstantaneous(ExecutionEnvironment& env)
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

void CommandOrbitFlags::processInstantaneous(ExecutionEnvironment& env)
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

void CommandSetVisibilityLimit::processInstantaneous(ExecutionEnvironment& env)
{
    env.getSimulation()->setFaintestVisible((float) magnitude);
}
////////////////
// Set FaintestAutoMag45deg command

CommandSetFaintestAutoMag45deg::CommandSetFaintestAutoMag45deg(double mag) :
    magnitude(mag)
{
}

void CommandSetFaintestAutoMag45deg::processInstantaneous(ExecutionEnvironment& env)
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

void CommandSetAmbientLight::processInstantaneous(ExecutionEnvironment& env)
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

void CommandSetGalaxyLightGain::processInstantaneous(ExecutionEnvironment& /* env */)
{
    Galaxy::setLightGain(lightGain);
}


////////////////
// Set command

CommandSet::CommandSet(std::string _name, double _value) :
    name(std::move(_name)), value(_value)
{
}

void CommandSet::processInstantaneous(ExecutionEnvironment& env)
{
    if (compareIgnoringCase(name, "MinOrbitSize") == 0)
    {
        if (env.getRenderer() != nullptr)
            env.getRenderer()->setMinimumOrbitSize(static_cast<float>(value));
    }
    else if (compareIgnoringCase(name, "AmbientLightLevel") == 0)
    {
        if (env.getRenderer() != nullptr)
            env.getRenderer()->setAmbientLightLevel(static_cast<float>(value));
    }
    else if (compareIgnoringCase(name, "FOV") == 0)
    {
        if (env.getRenderer() != nullptr)
            env.getSimulation()->getActiveObserver()->setFOV(celmath::degToRad(static_cast<float>(value)));
    }
    else if (compareIgnoringCase(name, "StarDistanceLimit") == 0)
    {
        if (env.getRenderer() != nullptr)
            env.getRenderer()->setDistanceLimit(static_cast<float>(value));
    }
    else if (compareIgnoringCase(name, "StarStyle") == 0)
    {
        // The cast from double to an enum requires an intermediate cast to int
        // Probably shouldn't be doing this at all, but other alternatives
        // are more trouble than they're worth.
        if (env.getRenderer() != nullptr)
            env.getRenderer()->setStarStyle(static_cast<Renderer::StarStyle>(static_cast<int>(value)));
    }
}


////////////////
// Mark object command

CommandMark::CommandMark(std::string _target,
                         celestia::MarkerRepresentation _rep,
                         bool _occludable) :
    target(std::move(_target)),
    rep(_rep),
    occludable(_occludable)
{
}

void CommandMark::processInstantaneous(ExecutionEnvironment& env)
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

CommandUnmark::CommandUnmark(std::string _target) :
    target(std::move(_target))
{
}

void CommandUnmark::processInstantaneous(ExecutionEnvironment& env)
{
    Selection sel = env.getSimulation()->findObjectFromPath(target);
    if (sel.empty())
        return;

    if (env.getSimulation()->getUniverse() != nullptr)
        env.getSimulation()->getUniverse()->unmarkObject(sel, 1);
}



///////////////
// Unmarkall command - clear all current markers

void CommandUnmarkAll::processInstantaneous(ExecutionEnvironment& env)
{
    if (env.getSimulation()->getUniverse() != nullptr)
        env.getSimulation()->getUniverse()->unmarkAll();
}


////////////////
// Preload textures command

CommandPreloadTextures::CommandPreloadTextures(std::string _name) :
    name(std::move(_name))
{
}

void CommandPreloadTextures::processInstantaneous(ExecutionEnvironment& env)
{
    Selection target = env.getSimulation()->findObjectFromPath(name);
    if (target.body() == nullptr)
        return;

    if (env.getRenderer() != nullptr)
        env.getRenderer()->loadTextures(target.body());
}


////////////////
// Capture command

CommandCapture::CommandCapture(std::string _type, std::string _filename)
    : type(std::move(_type)), filename(std::move(_filename))
{
}

void CommandCapture::processInstantaneous(ExecutionEnvironment& env)
{
    ContentType _type = ContentType::Unknown;
    if (type == "jpeg" || type == "jpg")
        _type = ContentType::JPEG;
    else if (type == "png")
        _type = ContentType::PNG;
#ifdef USE_LIBAVIF
    else if (type == "avif")
        _type = ContentType::AVIF;
#endif
    env.getCelestiaCore()->saveScreenShot(filename, _type);
}


////////////////
// Set texture resolution command

CommandSetTextureResolution::CommandSetTextureResolution(unsigned int _res) :
    res(_res)
{
}

void CommandSetTextureResolution::processInstantaneous(ExecutionEnvironment& env)
{
    if (env.getRenderer() != nullptr)
    {
        env.getRenderer()->setResolution(res);
        env.getCelestiaCore()->notifyWatchers(CelestiaCore::RenderFlagsChanged);
    }
}


////////////////
// SplitView command

CommandSplitView::CommandSplitView(unsigned int _view, std::string _splitType, double _splitPos) :
    view(_view),
    splitType(std::move(_splitType)),
    splitPos(_splitPos)
{
}

void CommandSplitView::processInstantaneous(ExecutionEnvironment& env)
{
    std::vector<Observer*> observer_list = env.getCelestiaCore()->getObservers();
    if (view >= 1 && view <= observer_list.size())
    {
        Observer* obs = observer_list[view - 1];
        View* view = env.getCelestiaCore()->getViewByObserver(obs);
        View::Type type = (compareIgnoringCase(splitType, "h") == 0) ? View::HorizontalSplit : View::VerticalSplit;
        env.getCelestiaCore()->splitView(type, view, static_cast<float>(splitPos));
    }
}


////////////////
// DeleteView command

CommandDeleteView::CommandDeleteView(unsigned int _view) :
    view(_view)
{
}

void CommandDeleteView::processInstantaneous(ExecutionEnvironment& env)
{
    std::vector<Observer*> observer_list = env.getCelestiaCore()->getObservers();

    if (view >= 1 && view <= observer_list.size())
    {
        Observer* obs = observer_list[view - 1];
        View* view = env.getCelestiaCore()->getViewByObserver(obs);
        env.getCelestiaCore()->deleteView(view);
    }
}


////////////////
// SingleView command

void CommandSingleView::processInstantaneous(ExecutionEnvironment& env)
{
    View* view = env.getCelestiaCore()->getViewByObserver(env.getSimulation()->getActiveObserver());
    env.getCelestiaCore()->singleView(view);
}


////////////////
// SetActiveView command

CommandSetActiveView::CommandSetActiveView(unsigned int _view) :
    view(_view)
{
}

void CommandSetActiveView::processInstantaneous(ExecutionEnvironment& env)
{
    std::vector<Observer*> observer_list = env.getCelestiaCore()->getObservers();

    if (view >= 1 && view <= observer_list.size())
    {
        Observer* obs = observer_list[view - 1];
        View* view = env.getCelestiaCore()->getViewByObserver(obs);
        env.getCelestiaCore()->setActiveView(view);
    }
}


////////////////
// SetRadius command

CommandSetRadius::CommandSetRadius(std::string _object, double _radius) :
    object(std::move(_object)),
    radius(_radius)
{
}

void CommandSetRadius::processInstantaneous(ExecutionEnvironment& env)
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

CommandSetLineColor::CommandSetLineColor(std::string _item, const Color& _color) :
    item(std::move(_item)),
    color(_color)
{
}

void CommandSetLineColor::processInstantaneous(ExecutionEnvironment& env)
{
    auto &LineColorMap = env.getCelestiaCore()->scriptMaps()->LineColorMap;
    if (LineColorMap.count(item) == 0)
    {
        GetLogger()->warn("Unknown line style: {}\n", item);
    }
    else
    {
        *LineColorMap[item] = color;
    }
}


////////////////
// SetLabelColor command

CommandSetLabelColor::CommandSetLabelColor(std::string _item, const Color& _color) :
    item(std::move(_item)),
    color(_color)
{
}

void CommandSetLabelColor::processInstantaneous(ExecutionEnvironment& env)
{
    auto &LabelColorMap = env.getCelestiaCore()->scriptMaps()->LabelColorMap;
    if (LabelColorMap.count(item) == 0)
    {
        GetLogger()->error("Unknown label style: {}\n", item);
    }
    else
    {
        *LabelColorMap[item] = color;
    }
}


////////////////
// SetTextColor command

CommandSetTextColor::CommandSetTextColor(const Color& _color) :
    color(_color)
{
}

void CommandSetTextColor::processInstantaneous(ExecutionEnvironment& env)
{
    env.getCelestiaCore()->setTextColor(color);
}


#ifdef USE_MINIAUDIO
CommandPlay::CommandPlay(int channel,
                         std::optional<float> volume,
                         float pan,
                         std::optional<bool> loop,
                         const std::optional<fs::path> &filename,
                         bool nopause) :
    channel(channel),
    volume(volume),
    pan(pan),
    loop(loop),
    filename(filename),
    nopause(nopause)
{
}

void CommandPlay::processInstantaneous(ExecutionEnvironment& env)
{
    auto appCore = env.getCelestiaCore();
    if (!filename.has_value())
    {
        // filename not set, only try to set values
        if (volume.has_value())
            appCore->setAudioVolume(channel, volume.value());
        appCore->setAudioPan(channel, pan);
        if (loop.has_value())
            appCore->setAudioLoop(channel, loop.value());
        appCore->setAudioNoPause(channel, nopause);
    }
    else if (filename.value().empty())
        appCore->stopAudio(channel);
    else
        appCore->playAudio(channel,
                           filename.value(),
                           0.0,
                           volume.value_or(celestia::defaultAudioVolume),
                           pan,
                           loop.value_or(false),
                           nopause);
}
#endif

// ScriptImage command
CommandScriptImage::CommandScriptImage(float _duration, float _fadeafter,
                                       float _xoffset, float _yoffset,
                                       const fs::path &_filename,
                                       bool _fitscreen,
                                       std::array<Color,4> &_colors) :
    duration(_duration),
    fadeafter(_fadeafter),
    xoffset(_xoffset),
    yoffset(_yoffset),
    filename(_filename),
    fitscreen(_fitscreen)
{
    std::copy(_colors.begin(), _colors.end(), colors.begin());
}

void CommandScriptImage::processInstantaneous(ExecutionEnvironment& env)
{
    auto image = std::make_unique<OverlayImage>(filename, env.getRenderer());
    image->setDuration(duration);
    image->setFadeAfter(fadeafter);
    image->setOffset(xoffset, yoffset);
    image->setColor(colors);
    image->fitScreen(fitscreen);
    env.getCelestiaCore()->setScriptImage(std::move(image));
}

// Verbosity command
CommandVerbosity::CommandVerbosity(int _level) :
    level(_level)
{
}

void CommandVerbosity::processInstantaneous(ExecutionEnvironment& env)
{
    env.getCelestiaCore()->setHudDetail(level);
}


///////////////
//

void CommandConstellations::processInstantaneous(ExecutionEnvironment& env)
{
    Universe* u = env.getSimulation()->getUniverse();
    if (u == nullptr)
        return;

    AsterismList& asterisms = *u->getAsterisms();
    for (auto& ast : asterisms)
    {
        if (flags.none)
        {
            ast.setActive(false);
        }
        else if (flags.all)
        {
            ast.setActive(true);
        }
        else
        {
            auto name = ast.getName(false);
            auto it = std::find_if(constellations.begin(), constellations.end(),
                                   [&name](Cons& c){ return compareIgnoringCase(c.name, name) == 0; });

            if (it != constellations.end())
                ast.setActive(it->active);
        }
    }
}

void CommandConstellations::setValues(std::string_view _cons, bool act)
{
    // ignore all above 99 constellations
    if (constellations.size() == MAX_CONSTELLATIONS)
        return;

    std::string cons(_cons);
    std::replace(cons.begin(), cons.end(), '_', ' ');

    auto it = std::find_if(constellations.begin(), constellations.end(),
                           [&cons](Cons& c){ return compareIgnoringCase(c.name, cons) == 0; });

    if (it != constellations.end())
        it->active = act;
    else
        constellations.emplace_back(cons, act); // If not found then add a new constellation
}


void CommandConstellationColor::processInstantaneous(ExecutionEnvironment& env)
{
    Universe* u = env.getSimulation()->getUniverse();
    if (u == nullptr)
        return;

    AsterismList& asterisms = *u->getAsterisms();
    for (auto& ast : asterisms)
    {
        if (flags.none)
        {
            ast.unsetOverrideColor();
        }
        else if (flags.all)
        {
            ast.setOverrideColor(rgb);
        }
        else
        {
            auto name = ast.getName(false);
            auto it = std::find_if(constellations.begin(), constellations.end(),
                                   [&name](const std::string& c){ return compareIgnoringCase(c, name) == 0; });

            if (it != constellations.end())
            {
                if (flags.unset)
                    ast.unsetOverrideColor();
                else
                    ast.setOverrideColor(rgb);
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

void CommandConstellationColor::setConstellations(std::string_view _cons)
{
    // ignore all above 99 constellations
    if (constellations.size() == MAX_CONSTELLATIONS)
       return;

    std::string cons(_cons);
    std::replace(cons.begin(), cons.end(), '_', ' ');

    // If not found then add a new constellation
    if (std::none_of(constellations.begin(), constellations.end(),
                     [&cons](const std::string& c){ return compareIgnoringCase(c, cons) == 0; }))
        constellations.push_back(cons);
}


///////////////
// SetWindowBordersVisible command
void CommandSetWindowBordersVisible::processInstantaneous(ExecutionEnvironment& env)
{
    env.getCelestiaCore()->setFramesVisible(visible);
}


///////////////
// SetRingsTexture command
CommandSetRingsTexture::CommandSetRingsTexture(std::string _object,
                                               std::string _textureName,
                                               std::string _path) :
    object(std::move(_object)),
    textureName(std::move(_textureName)),
    path(std::move(_path))
{
}

void CommandSetRingsTexture::processInstantaneous(ExecutionEnvironment& env)
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
CommandLoadFragment::CommandLoadFragment(std::string _type, std::string _fragment, std::string _dir) :
    type(std::move(_type)),
    fragment(std::move(_fragment)),
    dir(std::move(_dir))
{
}

void CommandLoadFragment::processInstantaneous(ExecutionEnvironment& env)
{
    Universe* u = env.getSimulation()->getUniverse();
    if (u == nullptr)
        return;

    std::istringstream in(fragment);
    if (compareIgnoringCase(type, "ssc") == 0)
    {
        LoadSolarSystemObjects(in, *u, dir);
    }
    else if (compareIgnoringCase(type, "stc") == 0)
    {
        u->getStarCatalog()->load(in, dir);
    }
    else if (compareIgnoringCase(type, "dsc") == 0)
    {
        u->getDSOCatalog()->load(in, dir);
    }
}

} // end namespace celestia::scripts
