// celx_object.h
//
// Copyright (C) 2003-2008, the Celestia Development Team
//
// Lua script extensions for Celestia: object
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELX_OBJECT_H_
#define _CELX_OBJECT_H_

struct lua_State;
class Selection;

extern void CreateObjectMetaTable(lua_State* l);
extern void ExtendObjectMetaTable(lua_State* l);
extern Selection* to_object(lua_State* l, int index);
extern int object_new(lua_State* l, const Selection& sel);

#endif // _CELX_OBJECT_H_
