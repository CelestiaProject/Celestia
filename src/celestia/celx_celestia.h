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

#include <lua.hpp>
#include <string>
#include <iostream>

#include "lua_registerable.h"

#include <celengine/observer.h>
#include <celestia/CelestiaCoreApplication.h>

int celestia_new(lua_State*, CelestiaCoreApplication*);
void CreateCelestiaMetaTable(lua_State* );
void ExtendCelestiaMetaTable(lua_State*);

class LuaCelestia : public CelestiaCoreApplication, public LuaRegisterable
{
public:
    static void registerInLua(sol::state&);
};

#endif // _CELX_CELESTIA_H_
