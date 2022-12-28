// celx_observer.cpp
//
// Copyright (C) 2003-2008, the Celestia Development Team
//
// Lua script extensions for Celestia: observer object
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <memory>

#include <Eigen/Geometry>

#include <celestia/celestiacore.h>
#include <celscript/common/scriptmaps.h>
#include <celutil/logger.h>
#include <celutil/stringutils.h>
#include "celx.h"
#include "celx_internal.h"
#include "celx_observer.h"

using namespace std;
using namespace Eigen;
using namespace celmath;
using celestia::util::GetLogger;


// ==================== Observer ====================

int observer_new(lua_State* l, Observer* o)
{
    CelxLua celx(l);

    Observer** ud = static_cast<Observer**>(lua_newuserdata(l, sizeof(Observer*)));
    *ud = o;

    celx.setClass(Celx_Observer);

    return 1;
}

Observer* to_observer(lua_State* l, int index)
{
    CelxLua celx(l);

    Observer** o = static_cast<Observer**>(lua_touserdata(l, index));
    CelestiaCore* appCore = celx.appCore(AllErrors);

    // Check if pointer is still valid, i.e. is used by a view:
    if (o != nullptr && getViewByObserver(appCore, *o) != nullptr)
    {
        return *o;
    }
    return nullptr;
}

static Observer* this_observer(lua_State* l)
{
    CelxLua celx(l);

    Observer* obs = to_observer(l, 1);
    if (obs == nullptr)
    {
        celx.doError("Bad observer object (maybe tried to access a deleted view?)!");
    }

    return obs;
}


static int observer_isvalid(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments expected for observer:isvalid()");
    lua_pushboolean(l, to_observer(l, 1) != nullptr);
    return 1;
}

static int observer_tostring(lua_State* l)
{
    lua_pushstring(l, "[Observer]");

    return 1;
}

static int observer_setposition(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "One argument required for setpos");

    Observer* o = this_observer(l);

    UniversalCoord* uc = celx.toPosition(2);
    if (uc == nullptr)
    {
        celx.doError("Argument to observer:setposition must be a position");
        return 0;
    }
    o->setPosition(*uc);
    return 0;
}

static int observer_setorientation(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "One argument required for setorientation");

    Observer* o = this_observer(l);

    auto q = celx.toRotation(2);
    if (q == nullptr)
    {
        celx.doError("Argument to observer:setorientation must be a rotation");
        return 0;
    }
    o->setOrientation(*q);
    return 0;
}

static int observer_getorientation(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments expected to observer:getorientation()");

    Observer* o = this_observer(l);
    celx.newRotation(o->getOrientation());

    return 1;
}

static int observer_rotate(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "One argument required for rotate");

    Observer* o = this_observer(l);

    auto q = celx.toRotation(2);
    if (q == nullptr)
    {
        celx.doError("Argument to observer:setpos must be a rotation");
        return 0;
    }
    o->rotate(q->cast<float>());

    return 0;
}

static int observer_orbit(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "One argument required for orbit");

    Observer* o = this_observer(l);

    auto q = celx.toRotation(2);
    if (q == nullptr)
    {
        celx.doError("Argument for observer:orbit must be a rotation");
        return 0;
    }
    o->orbit(Selection(), q->cast<float>());

    return 0;
}

static int observer_lookat(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(3, 4, "Two or three arguments required for lookat");
    int argc = lua_gettop(l);

    Observer* o = this_observer(l);

    UniversalCoord* from = nullptr;
    UniversalCoord* to = nullptr;
    Vector3d* upd = nullptr;

    if (argc == 3)
    {
        to = celx.toPosition(2);
        upd = celx.toVector(3);
        if (to == nullptr)
        {
            celx.doError("Argument 1 (of 2) to observer:lookat must be of type position");
            return 0;
        }
    }
    else
    {
        if (argc == 4)
        {
            from = celx.toPosition(2);
            to = celx.toPosition(3);
            upd = celx.toVector(4);

            if (to == nullptr || from == nullptr)
            {
                celx.doError("Argument 1 and 2 (of 3) to observer:lookat must be of type position");
                return 0;
            }
        }
    }

    if (upd == nullptr)
    {
        celx.doError("Last argument to observer:lookat must be of type vector");
        return 0;
    }
    Vector3d nd;
    if (from == nullptr)
    {
        nd = to->offsetFromKm(o->getPosition());
    }
    else
    {
        nd = to->offsetFromKm(*from);
    }
    Vector3f up = upd->cast<float>();
    Vector3f n = nd.cast<float>().normalized();

    Vector3f v = n.cross(up).normalized();
    Vector3f u = v.cross(n);
    Matrix3f m;
    m.row(0) = v; m.row(1) = u; m.row(2) = n * (-1);
    o->setOrientation(Quaternionf(m));

    return 0;
}

static int observer_gototable(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "Expected one table as argument to goto");

    Observer* o = this_observer(l);
    if (!lua_istable(l, 2))
    {
        lua_pushstring(l, "Argument to goto must be a table");
    }

    Observer::JourneyParams jparams;
    jparams.duration = Observer::JourneyDuration;
    jparams.from = o->getPosition();
    jparams.to = o->getPosition();
    jparams.initialOrientation = o->getOrientation();
    jparams.finalOrientation = o->getOrientation();
    jparams.startInterpolation = Observer::StartInterpolation;
    jparams.endInterpolation = Observer::EndInterpolation;
    jparams.accelTime = Observer::AccelerationTime;
    jparams.traj = Observer::Linear;

    lua_pushstring(l, "duration");
    lua_gettable(l, 2);
    jparams.duration = celx.safeGetNumber(3, NoErrors, "", Observer::JourneyDuration);
    lua_settop(l, 2);

    lua_pushstring(l, "from");
    lua_gettable(l, 2);
    UniversalCoord* from = celx.toPosition(3);
    if (from != nullptr)
        jparams.from = *from;
    lua_settop(l, 2);

    lua_pushstring(l, "to");
    lua_gettable(l, 2);
    UniversalCoord* to = celx.toPosition(3);
    if (to != nullptr)
        jparams.to = *to;
    lua_settop(l, 2);

    lua_pushstring(l, "initialOrientation");
    lua_gettable(l, 2);
    auto rot1 = celx.toRotation(3);
    if (rot1 != nullptr)
        jparams.initialOrientation = *rot1;
    lua_settop(l, 2);

    lua_pushstring(l, "finalOrientation");
    lua_gettable(l, 2);
    auto rot2 = celx.toRotation(3);
    if (rot2 != nullptr)
        jparams.finalOrientation = *rot2;
    lua_settop(l, 2);

    lua_pushstring(l, "startInterpolation");
    lua_gettable(l, 2);
    jparams.startInterpolation = celx.safeGetNumber(3, NoErrors, "", Observer::StartInterpolation);
    lua_settop(l, 2);

    lua_pushstring(l, "endInterpolation");
    lua_gettable(l, 2);
    jparams.endInterpolation = celx.safeGetNumber(3, NoErrors, "", Observer::EndInterpolation);
    lua_settop(l, 2);

    lua_pushstring(l, "accelTime");
    lua_gettable(l, 2);
    jparams.accelTime = celx.safeGetNumber(3, NoErrors, "", Observer::AccelerationTime);
    lua_settop(l, 2);

    jparams.duration = max(0.0, jparams.duration);
    jparams.accelTime = min(1.0, max(0.1, jparams.accelTime));
    jparams.startInterpolation = min(1.0, max(0.0, jparams.startInterpolation));
    jparams.endInterpolation = min(1.0, max(0.0, jparams.endInterpolation));

    // args are in universal coords, let setFrame handle conversion:
    auto tmp = o->getFrame();
    o->setFrame(ObserverFrame::Universal, Selection());
    o->gotoJourney(jparams);
    o->setFrame(tmp);

    return 0;
}

// First argument is the target object or position; optional second argument
// is the travel time
static int observer_goto(lua_State* l)
{
    CelxLua celx(l);
    if (lua_gettop(l) == 2 && lua_istable(l, 2))
    {
        // handle this in own function
        return observer_gototable(l);
    }
    celx.checkArgs(1, 6, "One to five arguments expected to observer:goto");

    Observer* o = this_observer(l);

    Selection* sel = celx.toObject(2);
    UniversalCoord* uc = celx.toPosition(2);
    if (sel == nullptr && uc == nullptr)
    {
        celx.doError("First arg to observer:gotoobject must be object or position");
        return 0;
    }

    double travelTime, startInter, endInter, accelTime;
    travelTime = celx.safeGetNumber(3, WrongType,
                                    "Second arg to observer:gotoobject must be a number",
                                    Observer::JourneyDuration);
    startInter = celx.safeGetNumber(4, WrongType,
                                    "Third arg to observer:gotoobject must be a number",
                                    Observer::StartInterpolation);
    endInter = celx.safeGetNumber(5, WrongType,
                                  "Fourth arg to observer:gotoobject must be a number",
                                  Observer::EndInterpolation);
    accelTime = celx.safeGetNumber(6, WrongType,
                                   "Fifth arg to observer:goto must be a number",
                                   Observer::AccelerationTime);
    if (startInter < 0.0 || startInter > 1.0)
        startInter = Observer::StartInterpolation;
    if (endInter < 0.0 || endInter > 1.0)
        endInter = Observer::EndInterpolation;

    // The first argument may be either an object or a position
    if (sel != nullptr)
    {
        o->gotoSelection(*sel,
                         travelTime,
                         startInter,
                         endInter,
                         accelTime,
                         Vector3f::UnitY(),
                         ObserverFrame::ObserverLocal);
    }
    else
    {
        o->gotoLocation(*uc, o->getOrientation(), travelTime);
    }

    return 0;
}

static int observer_gotolonglat(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 7, "One to five arguments expected to observer:gotolonglat");

    Observer* o = this_observer(l);

    Selection* sel = celx.toObject(2);
    if (sel == nullptr)
    {
        celx.doError("First arg to observer:gotolonglat must be an object");
        return 0;
    }
    double defaultDistance = sel->radius() * 5.0;

    double longitude  = celx.safeGetNumber(3, WrongType, "Second arg to observer:gotolonglat must be a number", 0.0);
    double latitude   = celx.safeGetNumber(4, WrongType, "Third arg to observer:gotolonglat must be a number", 0.0);
    double distance   = celx.safeGetNumber(5, WrongType, "Fourth arg to observer:gotolonglat must be a number", defaultDistance);
    double travelTime = celx.safeGetNumber(6, WrongType, "Fifth arg to observer:gotolonglat must be a number", 5.0);

    //distance = distance / KM_PER_LY;

    Vector3f up = Vector3f::UnitY();
    if (lua_gettop(l) >= 7)
    {
        auto uparg = celx.toVector(7);
        if (uparg == nullptr)
        {
            celx.doError("Sixth argument to observer:gotolonglat must be a vector");
            return 0;
        }
        up = uparg->cast<float>();
    }
    o->gotoSelectionLongLat(*sel, travelTime, distance, (float)longitude, (float)latitude, up);

    return 0;
}

// deprecated: wrong name, bad interface.
static int observer_gotolocation(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 3,"Expected one or two arguments to observer:gotolocation");

    Observer* o = this_observer(l);

    double travelTime = celx.safeGetNumber(3, WrongType, "Second arg to observer:gotolocation must be a number", 5.0);
    if (travelTime < 0)
        travelTime = 0.0;

    UniversalCoord* uc = celx.toPosition(2);
    if (uc != nullptr)
    {
        o->gotoLocation(*uc, o->getOrientation(), travelTime);
    }
    else
    {
        celx.doError("First arg to observer:gotolocation must be a position");
    }

    return 0;
}

static int observer_gotodistance(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 5, "One to four arguments expected to observer:gotodistance");

    Observer* o = this_observer(l);
    Selection* sel = celx.toObject(2);
    if (sel == nullptr)
    {
        celx.doError("First arg to observer:gotodistance must be object");
        return 0;
    }

    double distance = celx.safeGetNumber(3, WrongType, "Second arg to observer:gotodistance must be a number", 20000);
    double travelTime = celx.safeGetNumber(4, WrongType, "Third arg to observer:gotodistance must be a number", 5.0);

    Vector3f up = Vector3f::UnitY();
    if (lua_gettop(l) > 4)
    {
        auto up_arg = celx.toVector(5);
        if (up_arg == nullptr)
        {
            celx.doError("Fourth arg to observer:gotodistance must be a vector");
            return 0;
        }

        up = up_arg->cast<float>();
    }

    o->gotoSelection(*sel, travelTime, distance, up, ObserverFrame::Universal);

    return 0;
}

static int observer_gotosurface(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 3, "One to two arguments expected to observer:gotosurface");

    Observer* o = this_observer(l);
    Selection* sel = celx.toObject(2);
    if (sel == nullptr)
    {
        celx.doError("First arg to observer:gotosurface must be object");
        return 0;
    }

    double travelTime = celx.safeGetNumber(3, WrongType, "Second arg to observer:gotosurface must be a number", 5.0);

    // This is needed because gotoSurface expects frame to be geosync:
    o->geosynchronousFollow(*sel);
    o->gotoSurface(*sel, travelTime);

    return 0;
}

static int observer_center(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 3, "Expected one or two arguments for to observer:center");

    Observer* o = this_observer(l);
    Selection* sel = celx.toObject(2);
    if (sel == nullptr)
    {
        celx.doError("First argument to observer:center must be an object");
        return 0;
    }
    double travelTime = celx.safeGetNumber(3, WrongType, "Second arg to observer:center must be a number", 5.0);

    o->centerSelection(*sel, travelTime);

    return 0;
}

static int observer_centerorbit(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 3, "Expected one or two arguments for to observer:center");

    Observer* o = this_observer(l);
    Selection* sel = celx.toObject(2);
    if (sel == nullptr)
    {
        celx.doError("First argument to observer:centerorbit must be an object");
        return 0;
    }
    double travelTime = celx.safeGetNumber(3, WrongType, "Second arg to observer:centerorbit must be a number", 5.0);

    o->centerSelectionCO(*sel, travelTime);

    return 0;
}

static int observer_cancelgoto(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "Expected no arguments to observer:cancelgoto");

    Observer* o = this_observer(l);
    o->cancelMotion();

    return 0;
}

static int observer_follow(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "One argument expected for observer:follow");

    Observer* o = this_observer(l);
    Selection* sel = celx.toObject(2);
    if (sel == nullptr)
    {
        celx.doError("First argument to observer:follow must be an object");
        return 0;
    }
    o->follow(*sel);

    return 0;
}

static int observer_synchronous(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "One argument expected for observer:synchronous");

    Observer* o = this_observer(l);
    Selection* sel = celx.toObject(2);
    if (sel == nullptr)
    {
        celx.doError("First argument to observer:synchronous must be an object");
        return 0;
    }
    o->geosynchronousFollow(*sel);

    return 0;
}

static int observer_lock(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "One argument expected for observer:lock");

    Observer* o = this_observer(l);
    Selection* sel = celx.toObject(2);
    if (sel == nullptr)
    {
        celx.doError("First argument to observer:phaseLock must be an object");
        return 0;
    }
    o->phaseLock(*sel);

    return 0;
}

static int observer_chase(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "One argument expected for observer:chase");

    Observer* o = this_observer(l);
    Selection* sel = celx.toObject(2);
    if (sel == nullptr)
    {
        celx.doError("First argument to observer:chase must be an object");
        return 0;
    }
    o->chase(*sel);

    return 0;
}

static int observer_track(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "One argument expected for observer:track");

    Observer* o = this_observer(l);

    // If the argument is nil, clear the tracked object
    if (lua_isnil(l, 2))
    {
        o->setTrackedObject(Selection());
    }
    else
    {
        // Otherwise, turn on tracking and set the tracked object
        Selection* sel = celx.toObject(2);
        if (sel == nullptr)
        {
            celx.doError("First argument to observer:center must be an object");
            return 0;
        }
        o->setTrackedObject(*sel);
    }

    return 0;
}

static int observer_gettrackedobject(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments expected to observer:gettrackedobject");

    Observer* o = this_observer(l);
    celx.newObject(o->getTrackedObject());

    return 1;
}

// Return true if the observer is still moving as a result of a goto, center,
// or similar command.
static int observer_travelling(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments expected to observer:travelling");

    Observer* o = this_observer(l);
    if (o->getMode() == Observer::Travelling)
        lua_pushboolean(l, 1);
    else
        lua_pushboolean(l, 0);

    return 1;
}

// Return the observer's current time as a Julian day number
static int observer_gettime(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments expected to observer:gettime");

    Observer* o = this_observer(l);
    lua_pushnumber(l, o->getTime());

    return 1;
}

// Return the observer's current position
static int observer_getposition(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments expected to observer:getposition");

    Observer* o = this_observer(l);
    celx.newPosition(o->getPosition());

    return 1;
}

static int observer_getsurface(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "One argument expected to observer:getsurface()");

    Observer* obs = this_observer(l);
    lua_pushstring(l, obs->getDisplayedSurface().c_str());

    return 1;
}

static int observer_setsurface(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "One argument expected to observer:setsurface()");

    Observer* obs = this_observer(l);
    const char* s = lua_tostring(l, 2);

    if (s == nullptr)
        obs->setDisplayedSurface("");
    else
        obs->setDisplayedSurface(s);

    return 0;
}

static int observer_getframe(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments expected for observer:getframe()");

    Observer* obs = this_observer(l);

    auto frame = obs->getFrame();
    celx.newFrame(*frame);
    return 1;
}

static int observer_setframe(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "One argument required for observer:setframe()");

    Observer* obs = this_observer(l);

    ObserverFrame* frame;
    frame = celx.toFrame(2);
    if (frame != nullptr)
    {
        obs->setFrame(std::shared_ptr<const ObserverFrame>(new ObserverFrame(*frame)));
    }
    else
    {
        celx.doError("Argument to observer:setframe must be a frame");
    }
    return 0;
}

static int observer_setspeed(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "One argument required for observer:setspeed()");

    Observer* obs = this_observer(l);

    double speed = celx.safeGetNumber(2, AllErrors, "First argument to observer:setspeed must be a number");
    obs->setTargetSpeed((float) astro::microLightYearsToKilometers(speed));

    return 0;
}

static int observer_getspeed(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No argument expected for observer:getspeed()");

    Observer* obs = this_observer(l);

    lua_pushnumber(l, (lua_Number) astro::kilometersToMicroLightYears(obs->getTargetSpeed()));

    return 1;
}

static int observer_setfov(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "One argument expected to observer:setfov()");

    Observer* obs = this_observer(l);
    double fov = celx.safeGetNumber(2, AllErrors, "Argument to observer:setfov() must be a number");
    if ((fov >= degToRad(0.001f)) && (fov <= degToRad(120.0f)))
    {
        obs->setFOV((float) fov);
        celx.appCore(AllErrors)->setZoomFromFOV();
    }
    return 0;
}

static int observer_getfov(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No argument expected to observer:getfov()");

    Observer* obs = this_observer(l);
    lua_pushnumber(l, obs->getFOV());
    return 1;
}

static int observer_splitview(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 3, "One or two arguments expected for observer:splitview()");

    Observer* obs = this_observer(l);
    CelestiaCore* appCore = celx.appCore(AllErrors);
    const char* splitType = celx.safeGetString(2, AllErrors, "First argument to observer:splitview() must be a string");
    View::Type type = (compareIgnoringCase(splitType, "h") == 0) ? View::HorizontalSplit : View::VerticalSplit;
    double splitPos = celx.safeGetNumber(3, WrongType, "Number expected as argument to observer:splitview()", 0.5);
    if (splitPos < 0.1)
        splitPos = 0.1;
    if (splitPos > 0.9)
        splitPos = 0.9;
    View* view = getViewByObserver(appCore, obs);
    appCore->splitView(type, view, (float)splitPos);
    return 0;
}

static int observer_deleteview(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No argument expected for observer:deleteview()");

    Observer* obs = this_observer(l);
    CelestiaCore* appCore = celx.appCore(AllErrors);
    View* view = getViewByObserver(appCore, obs);
    appCore->deleteView(view);
    return 0;
}

static int observer_singleview(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No argument expected for observer:singleview()");

    Observer* obs = this_observer(l);
    CelestiaCore* appCore = celx.appCore(AllErrors);
    View* view = getViewByObserver(appCore, obs);
    appCore->singleView(view);
    return 0;
}

static int observer_makeactiveview(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No argument expected for observer:makeactiveview()");

    Observer* obs = this_observer(l);
    CelestiaCore* appCore = celx.appCore(AllErrors);
    View* view = getViewByObserver(appCore, obs);
    appCore->setActiveView(view);
    return 0;
}

static int observer_equal(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "Wrong number of arguments for comparison!");

    Observer* o1 = this_observer(l);
    Observer* o2 = to_observer(l, 2);

    lua_pushboolean(l, (o1 == o2));
    return 1;
}

static int observer_setlocationflags(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(2, 2, "One argument expected for observer:setlocationflags()");
    Observer* obs = this_observer(l);
    if (!lua_istable(l, 2))
    {
        celx.doError("Argument to observer:setlocationflags() must be a table");
        return 0;
    }

    lua_pushnil(l);
    auto locationFlags = obs->getLocationFilter();
    while (lua_next(l, -2) != 0)
    {
        string key;
        bool value = false;
        if (lua_isstring(l, -2))
        {
            key = lua_tostring(l, -2);
        }
        else
        {
            celx.doError("Keys in table-argument to observer:setlocationflags() must be strings");
            return 0;
        }
        if (lua_isboolean(l, -1))
        {
            value = lua_toboolean(l, -1) != 0;
        }
        else
        {
            celx.doError("Values in table-argument to observer:setlocationflags() must be boolean");
            return 0;
        }
        auto &LocationFlagMap = celx.appCore(AllErrors)->scriptMaps()->LocationFlagMap;
        if (LocationFlagMap.count(key) == 0)
        {
            GetLogger()->warn("Unknown key: {}\n", key);
        }
        else
        {
            const auto flag = LocationFlagMap[key];
            if (value)
            {
                locationFlags |= flag;
            }
            else
            {
                locationFlags &= ~flag;
            }
        }
        lua_pop(l,1);
    }
    obs->setLocationFilter(locationFlags);
    return 0;
}

static int observer_getlocationflags(lua_State* l)
{
    CelxLua celx(l);
    celx.checkArgs(1, 1, "No arguments expected for observer:getlocationflags()");
    Observer* obs = this_observer(l);
    lua_newtable(l);
    const auto locationFlags = obs->getLocationFilter();
    auto &LocationFlagMap = celx.appCore(AllErrors)->scriptMaps()->LocationFlagMap;
    std::string itString;
    itString.reserve(celestia::scripts::FlagMapNameLength);
    for (const auto& it : LocationFlagMap)
    {
        itString.clear();
        itString.append(it.first);
        lua_pushstring(l, itString.c_str());
        lua_pushboolean(l, (it.second & locationFlags) != 0);
        lua_settable(l, -3);
    }
    return 1;
}

void CreateObserverMetaTable(lua_State* l)
{
    CelxLua celx(l);
    celx.createClassMetatable(Celx_Observer);

    celx.registerMethod("__tostring", observer_tostring);
    celx.registerMethod("isvalid", observer_isvalid);
#if LUA_VERSION_NUM < 502
    celx.registerMethod("goto", observer_goto);
#endif
    celx.registerMethod("gotoobject", observer_goto);
    celx.registerMethod("gotolonglat", observer_gotolonglat);
    celx.registerMethod("gotolocation", observer_gotolocation);
    celx.registerMethod("gotodistance", observer_gotodistance);
    celx.registerMethod("gotosurface", observer_gotosurface);
    celx.registerMethod("cancelgoto", observer_cancelgoto);
    celx.registerMethod("setposition", observer_setposition);
    celx.registerMethod("lookat", observer_lookat);
    celx.registerMethod("setorientation", observer_setorientation);
    celx.registerMethod("getorientation", observer_getorientation);
    celx.registerMethod("getspeed", observer_getspeed);
    celx.registerMethod("setspeed", observer_setspeed);
    celx.registerMethod("getfov", observer_getfov);
    celx.registerMethod("setfov", observer_setfov);
    celx.registerMethod("rotate", observer_rotate);
    celx.registerMethod("orbit", observer_orbit);
    celx.registerMethod("center", observer_center);
    celx.registerMethod("centerorbit", observer_centerorbit);
    celx.registerMethod("follow", observer_follow);
    celx.registerMethod("synchronous", observer_synchronous);
    celx.registerMethod("chase", observer_chase);
    celx.registerMethod("lock", observer_lock);
    celx.registerMethod("track", observer_track);
    celx.registerMethod("gettrackedobject", observer_gettrackedobject);
    celx.registerMethod("travelling", observer_travelling);
    celx.registerMethod("getframe", observer_getframe);
    celx.registerMethod("setframe", observer_setframe);
    celx.registerMethod("gettime", observer_gettime);
    celx.registerMethod("getposition", observer_getposition);
    celx.registerMethod("getsurface", observer_getsurface);
    celx.registerMethod("setsurface", observer_setsurface);
    celx.registerMethod("splitview", observer_splitview);
    celx.registerMethod("deleteview", observer_deleteview);
    celx.registerMethod("singleview", observer_singleview);
    celx.registerMethod("makeactiveview", observer_makeactiveview);
    celx.registerMethod("getlocationflags", observer_getlocationflags);
    celx.registerMethod("setlocationflags", observer_setlocationflags);
    celx.registerMethod("__eq", observer_equal);

    lua_pop(l, 1); // remove metatable from stack
}
