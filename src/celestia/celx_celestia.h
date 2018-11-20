// celx_celestia.h
//
// Copyright (C) 2003-2008, the Celestia Development Team
//
// Lua script extensions for Celestia: Celestia object
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELX_CELESTIA_H_
#define _CELX_CELESTIA_H_

struct lua_State;

int celestia_new(lua_State*, CelestiaCore*);
CelestiaCore* to_celestia(lua_State*, int);
CelestiaCore* this_celestia(lua_State*);
void CreateCelestiaMetaTable(lua_State*);
void ExtendCelestiaMetaTable(lua_State*);

#endif // _CELX_CELESTIA_H_
