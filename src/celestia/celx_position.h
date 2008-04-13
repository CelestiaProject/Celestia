// celx_position.h
//
// Copyright (C) 2003-2008, the Celestia Development Team
//
// Lua script extensions for Celestia: position object
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELX_POSITION_H_
#define _CELX_POSITION_H_

struct lua_State;

extern void CreatePositionMetaTable(lua_State* l);
extern int position_new(lua_State* l, const UniversalCoord& uc);
extern UniversalCoord* to_position(lua_State* l, int index);

#endif // _CELX_POSITION_H_
