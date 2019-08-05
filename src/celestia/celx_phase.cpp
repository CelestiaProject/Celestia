// celx_phase.cpp
//
// Copyright (C) 2003-2008, the Celestia Development Team
//
// Lua script extensions for Celestia: phase object
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <assert.h>
#include "celx.h"
#include "celx_internal.h"
#include "celx_phase.h"
#include <celengine/timelinephase.h>
#include <celephem/orbit.h>
#include <celephem/rotation.h>

using namespace std;

int phase_new(lua_State* l, const TimelinePhase::SharedConstPtr& phase)
{
    CelxLua celx(l);

    // Use placement new to put the new phase reference in the userdata block.
    void* block = lua_newuserdata(l, sizeof(TimelinePhase::SharedConstPtr));
    new (block) TimelinePhase::SharedConstPtr(phase);

    celx.setClass(Celx_Phase);

    return 1;
}


static TimelinePhase::SharedConstPtr* to_phase(lua_State* l, int index)
{
    CelxLua celx(l);

    return celx.safeGetClass<TimelinePhase::SharedConstPtr>(index);
}


static const TimelinePhase::SharedConstPtr& this_phase(lua_State* l)
{
    CelxLua celx(l);

    auto phase = to_phase(l, 1);
    assert(phase != nullptr);
    if (phase == nullptr)
    {
        celx.doError("Bad phase object!");
    }

    return *phase;
}

/*! phase:timespan()
 *
 * Return the start and end times for this timeline phase.
 *
 * \verbatim
 * -- Example: retrieve the start and end times of the first phase
 * -- of Cassini's timeline:
 * --
 * cassini = celestia:find("Sol/Cassini")
 * phases = cassini:timeline()
 * begintime, endtime = phases[1]:timespan()
 *
 * \endverbatim
 */
static int phase_timespan(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments allowed for to phase:timespan");

    auto phase = this_phase(l);
    celx.push(phase->startTime(), phase->endTime());
    //lua_pushnumber(l, phase->startTime());
    //lua_pushnumber(l, phase->endTime());

    return 2;
}


/*! frame phase:orbitframe()
 *
 * Return the orbit frame for this timeline phase.
 */
static int phase_orbitframe(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments allowed for to phase:orbitframe");

    auto phase = this_phase(l);
    auto f = phase->orbitFrame();
    celx.newFrame(ObserverFrame(*f));

    return 1;
}


/*! frame phase:bodyframe()
 *
 * Return the body frame for this timeline phase.
 */
static int phase_bodyframe(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(1, 1, "No arguments allowed for to phase:bodyframe");

    auto phase = this_phase(l);
    auto f = phase->bodyFrame();
    celx.newFrame(ObserverFrame(*f));

    return 1;
}


/*! position phase:getposition(time: t)
 *
 * Return the position in frame coordinates at the specified time.
 * Times outside the span covered by the phase are automatically clamped
 * to either the beginning or ending of the span.
 */
static int phase_getposition(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "One argument required for phase:getposition");

    auto phase = this_phase(l);

    double tdb = celx.safeGetNumber(2, WrongType, "Argument to phase:getposition() must be number", 0.0);
    if (tdb < phase->startTime())
        tdb = phase->startTime();
    else if (tdb > phase->endTime())
        tdb = phase->endTime();
    celx.newPosition(UniversalCoord(phase->orbit()->positionAtTime(tdb) * astro::kilometersToMicroLightYears(1.0)));

    return 1;
}


/*! rotation phase:getorientation(time: t)
 *
 * Return the orientation in frame coordinates at the specified time.
 * Times outside the span covered by the phase are automatically clamped
 * to either the beginning or ending of the span.
 */
static int phase_getorientation(lua_State* l)
{
    CelxLua celx(l);

    celx.checkArgs(2, 2, "One argument required for phase:getorientation");

    auto phase = this_phase(l);

    double tdb = celx.safeGetNumber(2, WrongType, "Argument to phase:getorientation() must be number", 0.0);
    if (tdb < phase->startTime())
        tdb = phase->startTime();
    else if (tdb > phase->endTime())
        tdb = phase->endTime();
    celx.newRotation(phase->rotationModel()->orientationAtTime(tdb));

    return 1;
}


/*! __tostring metamethod
 * Convert a phase to a string (currently just "[Phase]")
 */
static int phase_tostring(lua_State* l)
{
    lua_pushstring(l, "[Phase]");

    return 1;
}


/*! __gc metamethod
 * Garbage collection for phases.
 */
static int phase_gc(lua_State* l)
{
    CelxLua celx(l);

    auto ref = this_phase(l);
    if (ref)
    {
        ref.reset();
    }
    else
    {
        celx.doError("Bad phase object during garbage collection!");
    }

    return 0;
}


void CreatePhaseMetaTable(lua_State* l)
{
    CelxLua celx(l);

    celx.createClassMetatable(Celx_Phase);

    celx.registerMethod("__tostring", phase_tostring);
    celx.registerMethod("__gc", phase_gc);
    celx.registerMethod("timespan", phase_timespan);
    celx.registerMethod("orbitframe", phase_orbitframe);
    celx.registerMethod("bodyframe", phase_bodyframe);
    celx.registerMethod("getposition", phase_getposition);
    celx.registerMethod("getorientation", phase_getorientation);

    lua_pop(l, 1); // remove metatable from stack
}
