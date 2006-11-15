// scriptorbit.cpp
// 
// Copyright (C) 2006, Chris Laurel <claurel@shatters.net>
//
// Interface for a Celestia trajectory implemented via a Lua script.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstdio>
#include <cassert>
#include "scriptorbit.h"

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

using namespace std;

static char* ScriptedOrbitNamePrefix = "cel_script_orbit_";
static unsigned int ScriptedOrbitNameIndex = 1;

// global script context for scripted orbits and rotations
static lua_State* scriptOrbitLuaState = NULL;


void
SetScriptedOrbitContext(lua_State* l)
{
    scriptOrbitLuaState = l;
}


lua_State*
GetScriptedOrbitContext()
{
    return scriptOrbitLuaState;
}


ScriptedOrbit::ScriptedOrbit() :
    luaState(NULL),
    boundingRadius(1.0),
    period(0.0),
    validRangeBegin(0.0),
    validRangeEnd(0.0)
{
}


ScriptedOrbit::~ScriptedOrbit()
{
}


// Helper function to retrieve an entry from a table and leave
// it on the top of the stack.
static void
getTableEntry(lua_State* state,
              int tableIndex,
              const string& key)
{
    lua_pushvalue(state, tableIndex);
    lua_pushstring(state, key.c_str());
    lua_gettable(state, -2);
    lua_remove(state, -2);
}


// Helper function to retrieve a number value from a table; returns
// the specified default value if the key doesn't exist in the table.
static double
safeGetLuaNumber(lua_State* state,
                 int tableIndex,
                 const string& key,
                 double defaultValue)
{
    double v = defaultValue;

    getTableEntry(state, tableIndex, key);
    if (lua_isnumber(state, -1))
        v = lua_tonumber(state, -1);
    lua_pop(state, 1);

    return v;
}

/*! Initialize the script orbit. 
 *  moduleName is the name of a module that contains the orbit factory
 *  function. The module will be loaded with Lua's require function before
 *  creating the Lua orbit object.
 *
 *  funcName is the name of some factory function in the specified luaState
 *  that will produce a Lua orbit object from the parameter list.
 *
 *  The Lua factory function accepts a single table parameter containg
 *  all the orbit properties. It returns a table with the following
 *  properties:
 *
 *      boundingRadius - A number giving the maximum distance of the trajectory
 *         from the origin; must be present, and must be a positive value.
 *      period - A number giving the period of the orbit. If not present,
 *         the orbit is assumed to be aperiodic. The orbital period is only
 *         used for drawing the orbit path.
 *      begin, end - optional values that specify the time span over which
 *         the orbit is valid. If not given, the orbit is assumed to be useable
 *         at any time. The orbit is invalid if end < begin.
 *      position(time) - The position function takes a time value as input
 *         (TDB Julian day) and returns three values which are the x, y, and
 *         z coordinates. Units for the position are kilometers.
 */
bool
ScriptedOrbit::initialize(const std::string& moduleName,
                          const std::string& funcName,
                          Hash* parameters)
{
    if (parameters == NULL)
        return false;

    luaState = GetScriptedOrbitContext();
    if (luaState == NULL)
    {
        clog << "ScriptedOrbits are currently disabled.\n";
        return false;
    }

    if (!moduleName.empty())
    {
        lua_pushstring(luaState, "require");
        lua_gettable(luaState, LUA_GLOBALSINDEX);
        if (!lua_isfunction(luaState, -1))
        {
            clog << "Cannot load ScriptedOrbit package: 'require' function is unavailable\n";
            lua_pop(luaState, 1);
            return false;
        }

        lua_pushstring(luaState, moduleName.c_str());
        if (lua_pcall(luaState, 1, 1, 0) != 0)
        {
            clog << "Failed to load module for ScriptedOrbit: " << lua_tostring(luaState, -1) << "\n";
            lua_pop(luaState, 1);
            return false;
        }
    }

    // Get the orbit generator function
    lua_pushstring(luaState, funcName.c_str());
    lua_gettable(luaState, LUA_GLOBALSINDEX);

    if (lua_isfunction(luaState, -1) == 0)
    {
        // No function with the requested name; pop whatever value we 
        // did receive along with the table of arguments.
        lua_pop(luaState, 1);
        clog << "No Lua function named " << funcName << " found.\n";
        return false;
    }

    // Construct the table that we'll pass to the orbit generator function
    lua_newtable(luaState);

    // Convert all the parameters in the list to their equivalent Lua
    // types and insert them into the table. Presently, only number, string,
    // and boolean values are converted.
    for (HashIterator iter = parameters->begin(); iter != parameters->end();
         iter++)
    {
        switch (iter->second->getType())
        {
        case Value::NumberType:
            lua_pushstring(luaState, iter->first.c_str());
            lua_pushnumber(luaState, iter->second->getNumber());
            lua_settable(luaState, -3);
            break;
        case Value::StringType:
            lua_pushstring(luaState, iter->first.c_str());
            lua_pushstring(luaState, iter->second->getString().c_str());
            lua_settable(luaState, -3);
            break;
        case Value::BooleanType:
            lua_pushstring(luaState, iter->first.c_str());
            lua_pushboolean(luaState, iter->second->getBoolean());
            lua_settable(luaState, -3);
            break;
        default:
            break;
        }
    }

    // Call the generator function
    if (lua_pcall(luaState, 1, 1, 0) != 0)
    {
        // Some sort of error occurred--the error message is atop the stack
        clog << "Error calling ScriptedOrbit generator function: " <<
            lua_tostring(luaState, -1) << "\n";
        lua_pop(luaState, 1);
        return NULL;
    }

    if (lua_istable(luaState, -1) == 0)
    {
        // We have an object, but it's not a table. Pop it off the
        // stack and report failure.
        clog << "ScriptedOrbit generator function returned bad value.\n";
        lua_pop(luaState, 1);
        return NULL;
    }

    // Generate a unique name for this script orbit object so that
    // we can refer to it later.
    char buf[64];
    sprintf(buf, "%s%u", ScriptedOrbitNamePrefix, ScriptedOrbitNameIndex);
    ScriptedOrbitNameIndex++;
    luaOrbitObjectName = string(buf);

    // Attach the name to the script orbit
    lua_pushstring(luaState, luaOrbitObjectName.c_str());
    lua_pushvalue(luaState, -2); // dup the orbit object on top of stack
    lua_settable(luaState, LUA_GLOBALSINDEX);

    // Now, call orbit object methods to get the bounding radius
    // and valid time range.
    lua_pushstring(luaState, "boundingRadius");
    lua_gettable(luaState, -2);
    if (lua_isnumber(luaState, -1) == 0)
    {
        clog << "Bad or missing boundingRadius for ScriptedOrbit object\n";
        lua_pop(luaState, 1);
        return false;
    }

    boundingRadius = lua_tonumber(luaState, -1);
    lua_pop(luaState, 1);

    // Get the rest of the orbit parameters; they are all optional.
    period          = safeGetLuaNumber(luaState, -1, "period", 0.0);
    validRangeBegin = safeGetLuaNumber(luaState, -1, "begin", 0.0);
    validRangeEnd   = safeGetLuaNumber(luaState, -1, "end", 0.0);

    // Pop the orbit object off the stack
    lua_pop(luaState, 1);

    // Perform some sanity checks on the orbit parameters
    if (validRangeEnd < validRangeBegin)
    {
        clog << "Bad script orbit: valid range end < begin\n";
        return false;
    }

    if (boundingRadius <= 0.0)
    {
        clog << "Bad script object: bounding radius must be positive\n";
        return false;
    }

    return true;
}


// Call the position method of the ScriptedOrbit object
Point3d
ScriptedOrbit::computePosition(double tjd) const
{
    Point3d pos(0.0, 0.0, 0.0);

    lua_pushstring(luaState, luaOrbitObjectName.c_str());
    lua_gettable(luaState, LUA_GLOBALSINDEX);
    if (lua_istable(luaState, -1))
    {
        lua_pushstring(luaState, "position");
        lua_gettable(luaState, -2);
        if (lua_isfunction(luaState, -1))
        {
            lua_pushvalue(luaState, -2); // push 'self' on stack
            lua_pushnumber(luaState, tjd);
            if (lua_pcall(luaState, 2, 3, 0) == 0)
            {
                pos.x = lua_tonumber(luaState, -3);
                pos.y = lua_tonumber(luaState, -2);
                pos.z = lua_tonumber(luaState, -1);
                lua_pop(luaState, 3);
            }
            else
            {
                // Function call failed for some reason
                //clog << "ScriptedOrbit failed: " << lua_tostring(luaState, -1) << "\n";
                lua_pop(luaState, 1);
            }
        }
        else
        {
            // Bad position function
            lua_pop(luaState, 1);
        }
    }
    else
    {
        // The script orbit object disappeared. OOPS.
    }
    
    // Pop the script orbit object
    lua_pop(luaState, 1);

    // Convert to Celestia's internal coordinate system 
    return Point3d(pos.x, pos.z, -pos.y);
}


double
ScriptedOrbit::getPeriod() const
{
    if (period == 0.0)
    {
        return validRangeEnd - validRangeBegin;
    }
    else
    {
        return period;
    }
}


bool
ScriptedOrbit::isPeriodic() const
{
    return period != 0.0;
}


void
ScriptedOrbit::getValidRange(double& begin, double& end) const
{
    begin = validRangeBegin;
    end = validRangeEnd;
}


double
ScriptedOrbit::getBoundingRadius() const
{
    return boundingRadius;
}
