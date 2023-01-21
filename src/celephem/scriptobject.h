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

#pragma once

#ifndef LUA_VER
#define LUA_VER 0x050100
#endif

#include <string>

#include <lua.hpp>

#include <celengine/hash.h>


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

void SetLuaVariables(lua_State* state, const Hash& parameters);
