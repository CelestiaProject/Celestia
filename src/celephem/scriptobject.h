// scriptobject.h
// 
// Copyright (C) 2006, Chris Laurel <claurel@shatters.net>
//
// Helper functions for Celestia's interface to Lua scripts.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_SCRIPTOBJECT_H_
#define _CELENGINE_SCRIPTOBJECT_H_

#ifndef LUA_VER
#define LUA_VER 0x050000
#endif

#if LUA_VER >= 0x050100
#include "lua.hpp"
#else
extern "C" {
#include "lua.h"
#include "lualib.h"
}
#endif

#include <string>
#include <celengine/parser.h>


void SetScriptedObjectContext(lua_State* l);

lua_State* GetScriptedObjectContext();


std::string GenerateScriptObjectName();

void GetLuaTableEntry(lua_State* state,
                      int tableIndex,
                      const std::string& key);

double SafeGetLuaNumber(lua_State* state,
                        int tableIndex,
                        const std::string& key,
                        double defaultValue);

void SetLuaVariables(lua_State* state, Hash* parameters);

#endif // _CELENGINE_SCRIPTOBJECT_H_
