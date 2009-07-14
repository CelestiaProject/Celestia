// celx_observer.h
//
// Copyright (C) 2003-2008, the Celestia Development Team
//
// Lua script extensions for Celestia: observer object
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELX_OBSERVER_H_
#define _CELX_OBSERVER_H_

struct lua_State;
class Observer;

extern void CreateObserverMetaTable(lua_State* l);
extern int observer_new(lua_State* l, Observer* obs);
extern Observer* to_observer(lua_State* l, int index);

#endif // _CELX_OBSERVER_H_
