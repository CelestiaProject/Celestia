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

#include "celx.h"
#include "celx_internal.h"
#include "celx_phase.h"
#include <celengine/timelinephase.h>


// We want to avoid copying TimelinePhase objects, so we can't make them
// userdata. But, they can't be lightuserdata either because they need to
// be reference counted, and Lua doesn't garbage collect lightuserdata. The
// solution is the PhaseReference object, which just wraps a TimelinePhase
// pointer.
class PhaseReference
{
public:
    PhaseReference(const TimelinePhase& _phase) :
    phase(&_phase)
{
        phase->addRef();
}

~PhaseReference()
{
    phase->release();
}

const TimelinePhase* phase;
};



int phase_new(lua_State* l, const TimelinePhase& phase)
{
    CelxLua celx(l);
    
    // Use placement new to put the new phase reference in the userdata block.
    void* block = lua_newuserdata(l, sizeof(PhaseReference));
    new (block) PhaseReference(phase);

    celx.setClass(Celx_Phase);

    return 1;
}


static const TimelinePhase* to_phase(lua_State* l, int index)
{
    CelxLua celx(l);
    
    PhaseReference* ref = static_cast<PhaseReference*>(celx.checkUserData(index, Celx_Phase));
    return ref == NULL ? NULL : ref->phase;
}


static const TimelinePhase* this_phase(lua_State* l)
{
    CelxLua celx(l);
    
    const TimelinePhase* phase = to_phase(l, 1);
    if (phase == NULL)
    {
        celx.doError("Bad phase object!");
    }

    return phase;
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
        
	const TimelinePhase* phase = this_phase(l);
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
        
	const TimelinePhase* phase = this_phase(l);
    const ReferenceFrame* f = phase->orbitFrame();
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
        
	const TimelinePhase* phase = this_phase(l);
    const ReferenceFrame* f = phase->bodyFrame();
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

    const TimelinePhase* phase = this_phase(l);

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

    const TimelinePhase* phase = this_phase(l);

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
    
    PhaseReference* ref = static_cast<PhaseReference*>(celx.checkUserData(1, Celx_Phase));
    if (ref == NULL)
    {
        celx.doError("Bad phase object during garbage collection!");
    }
    else
    {
        // Explicitly call the destructor since the object was created with placement new
        ref->~PhaseReference();
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
