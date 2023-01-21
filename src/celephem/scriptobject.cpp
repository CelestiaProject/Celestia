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

#include <cstddef>
#include <string_view>

#include <fmt/format.h>

#include <celengine/value.h>
#include "scriptobject.h"

using namespace std::string_view_literals;

namespace celestia::ephem
{

namespace
{

class ScriptObjectState
{
private:
    lua_State* context{ nullptr };
    unsigned int nameIndex{ 1 };

public:
    lua_State* getContext() { return context; }
    void setContext(lua_State* luaState) { context = luaState; }
    unsigned int getNameIndex() { return nameIndex++; }
};

// global script context for scripted orbits and rotations

constexpr std::string_view ScriptedObjectNamePrefix = "cel_script_object_"sv;

ScriptObjectState* getCurrentObjectState()
{
    static ScriptObjectState* const state = std::make_unique<ScriptObjectState>().release();
    return state;
}

class HashVisitor
{
private:
    lua_State* state;

public:
    explicit HashVisitor(lua_State* pState) : state{pState} {}

    void operator()(const std::string& key, const Value& value)
    {
        std::size_t percentPos = key.find('%');
        if (percentPos == std::string::npos)
        {
            switch (value.getType())
            {
            case ValueType::NumberType:
                lua_pushstring(state, key.c_str());
                lua_pushnumber(state, *value.getNumber());
                lua_settable(state, -3);
                break;
            case ValueType::StringType:
                lua_pushstring(state, key.c_str());
                lua_pushstring(state, value.getString()->c_str());
                lua_settable(state, -3);
                break;
            case ValueType::BooleanType:
                lua_pushstring(state, key.c_str());
                lua_pushboolean(state, *value.getBoolean());
                lua_settable(state, -3);
                break;
            default:
                break;
            }
        }
    }
};

} // end unnamed namespace

/*! Set the script context for ScriptedOrbits and ScriptRotations
 *  Should be called just once at initialization.
 */
void
SetScriptedObjectContext(lua_State* l)
{
    getCurrentObjectState()->setContext(l);
}


/*! Get the global script context for ScriptedOrbits.
 */
lua_State*
GetScriptedObjectContext()
{
    return getCurrentObjectState()->getContext();
}


/*! Generate a unique name for this script orbit object so that
 * we can refer to it later.
 */
std::string
GenerateScriptObjectName()
{
    std::string buf;
    buf = fmt::format("{}{}", ScriptedObjectNamePrefix, getCurrentObjectState()->getNameIndex());
    return buf;
}


/*! Helper function to retrieve an entry from a table and leave
 *  it on the top of the stack.
 */
void
GetLuaTableEntry(lua_State* state,
                 int tableIndex,
                 const std::string& key)
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
                 const std::string& key,
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
SetLuaVariables(lua_State* state, const AssociativeArray& parameters)
{
    HashVisitor visitor(state);
    parameters.for_all(visitor);
}

} // end namespace celestia::ephem
