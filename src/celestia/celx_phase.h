// celx_phase.h
//
// Copyright (C) 2003-2008, the Celestia Development Team
//
// Lua script extensions for Celestia: phase object
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELX_PHASE_H_
#define _CELX_PHASE_H_

#include <memory>

struct lua_State;
class TimelinePhase;

inline int celxClassId(const TimelinePhase::SharedConstPtr&)
{
    return Celx_Phase;
}

extern void CreatePhaseMetaTable(lua_State* l);
extern int phase_new(lua_State* l, const TimelinePhase::SharedConstPtr& phase);

#endif // _CELX_PHASE_H_
