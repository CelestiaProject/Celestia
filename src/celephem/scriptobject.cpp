// scriptobject.cpp
// 
// Copyright (C) 2006, Chris Laurel <claurel@shatters.net>
//
// Helper functions for Celestia's interface to Lua scripts.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstdio>

#include "scriptobject.h"

using namespace std;


// global script context for scripted orbits and rotations
static lua_State* scriptObjectLuaState = NULL;

static const char* ScriptedObjectNamePrefix = "cel_script_object_";
static unsigned int ScriptedObjectNameIndex = 1;


/*! Set the script context for ScriptedOrbits and ScriptRotations
 *  Should be called just once at initialization.
 */
void
SetScriptedObjectContext(lua_State* l)
{
    scriptObjectLuaState = l;
}


/*! Get the global script context for ScriptedOrbits.
 */
lua_State*
GetScriptedObjectContext()
{
    return scriptObjectLuaState;
}


/*! Generate a unique name for this script orbit object so that
 * we can refer to it later.
 */
string
GenerateScriptObjectName()
{
    char buf[64];
    sprintf(buf, "%s%u", ScriptedObjectNamePrefix, ScriptedObjectNameIndex);
    ScriptedObjectNameIndex++;

    return string(buf);
}


/*! Helper function to retrieve an entry from a table and leave
 *  it on the top of the stack.
 */
void
GetLuaTableEntry(lua_State* state,
                 int tableIndex,
                 const string& key)
{
    lua_pushvalue(state, tableIndex);
    lua_pushstring(state, key.c_str());
    lua_gettable(state, -2);
    lua_remove(state, -2);
}


/*! Helper function to retrieve a number value from a table; returns
 *  the specified default value if the key doesn't exist in the table.
 */
double
SafeGetLuaNumber(lua_State* state,
                 int tableIndex,
                 const string& key,
                 double defaultValue)
{
    double v = defaultValue;

    GetLuaTableEntry(state, tableIndex, key);
    if (lua_isnumber(state, -1))
        v = lua_tonumber(state, -1);
    lua_pop(state, 1);

    return v;
}


/*! Convert all the parameters in an AssociativeArray to their equivalent Lua
 *  types and insert them into the table on the top of the stack. Presently,
 *  only number, string, and boolean values are converted.
 */
void
SetLuaVariables(lua_State* state, Hash* parameters)
{
    for (HashIterator iter = parameters->begin(); iter != parameters->end();
         iter++)
    {
        size_t percentPos = iter->first.find('%');
        if (percentPos == string::npos)
        {
            switch (iter->second->getType())
            {
            case Value::NumberType:
                lua_pushstring(state, iter->first.c_str());
                lua_pushnumber(state, iter->second->getNumber());
                lua_settable(state, -3);
                break;
            case Value::StringType:
                lua_pushstring(state, iter->first.c_str());
                lua_pushstring(state, iter->second->getString().c_str());
                lua_settable(state, -3);
                break;
            case Value::BooleanType:
                lua_pushstring(state, iter->first.c_str());
                lua_pushboolean(state, iter->second->getBoolean());
                lua_settable(state, -3);
                break;
            default:
                break;
            }
        }
    }
}
