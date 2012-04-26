// scriptrotation.cpp
// 
// Copyright (C) 2006, Chris Laurel <claurel@shatters.net>
//
// Interface for a Celestia rotation model implemented via a Lua script.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstdio>
#include <cassert>
#include "scriptobject.h"
#include "scriptrotation.h"

using namespace Eigen;
using namespace std;


ScriptedRotation::ScriptedRotation() :
    luaState(NULL),
    period(0.0),
    validRangeBegin(0.0),
    validRangeEnd(0.0),
    lastTime(-1.0e50),
    lastOrientation(Quaterniond::Identity()),
    cacheable(true) // non-cacheable rotations not yet supported
{
}


ScriptedRotation::~ScriptedRotation()
{
}


/*! Initialize the script rotation
 *  moduleName is the name of a module that contains the rotation factory
 *  function. The module will be loaded with Lua's require function before
 *  creating the Lua rotation object.
 *
 *  funcName is the name of some factory function in the specified luaState
 *  that will produce a Lua rotation object from the parameter list.
 *
 *  The Lua factory function accepts a single table parameter containg
 *  all the rotation properties. It returns a table with the following
 *  properties:
 *
 *      period - A number giving the period of the rotation. If not present,
 *         the rotation is assumed to be aperiodic.
 *      beginDate, endDate - optional values that specify the time span over
 *         which the rotation model is valid. If not given, the rotation model
 *         is assumed to be valid over all time. The rotation model is invalid
 *         if end < begin.
 *      orientation(time) - The orientation function takes a time value as
 *         input (TDB Julian day) and returns three values which are the the
 *         quaternion (w, x, y, z).
 */
bool
ScriptedRotation::initialize(const std::string& moduleName,
                             const std::string& funcName,
                             Hash* parameters)
{
    if (parameters == NULL)
        return false;

    luaState = GetScriptedObjectContext();
    if (luaState == NULL)
    {
        clog << "ScriptedRotations are currently disabled.\n";
        return false;
    }

    if (!moduleName.empty())
    {
        lua_pushstring(luaState, "require");
        lua_gettable(luaState, LUA_GLOBALSINDEX);
        if (!lua_isfunction(luaState, -1))
        {
            clog << "Cannot load ScriptedRotation package: 'require' function is unavailable\n";
            lua_pop(luaState, 1);
            return false;
        }

        lua_pushstring(luaState, moduleName.c_str());
        if (lua_pcall(luaState, 1, 1, 0) != 0)
        {
            clog << "Failed to load module for ScriptedRotation: " << lua_tostring(luaState, -1) << "\n";
            lua_pop(luaState, 1);
            return false;
        }
    }

    // Get the rotation generator function
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

    // Construct the table that we'll pass to the rotation generator function
    lua_newtable(luaState);

    SetLuaVariables(luaState, parameters);

    // Call the generator function
    if (lua_pcall(luaState, 1, 1, 0) != 0)
    {
        // Some sort of error occurred--the error message is atop the stack
        clog << "Error calling ScriptedRotation generator function: " <<
            lua_tostring(luaState, -1) << "\n";
        lua_pop(luaState, 1);
        return false;
    }

    if (lua_istable(luaState, -1) == 0)
    {
        // We have an object, but it's not a table. Pop it off the
        // stack and report failure.
        clog << "ScriptedRotation generator function returned bad value.\n";
        lua_pop(luaState, 1);
        return false;
    }
    luaRotationObjectName = GenerateScriptObjectName();

    // Attach the name to the script rotation
    lua_pushstring(luaState, luaRotationObjectName.c_str());
    lua_pushvalue(luaState, -2); // dup the rotation object on top of stack
    lua_settable(luaState, LUA_GLOBALSINDEX);

    // Get the rest of the rotation parameters; they are all optional.
    period          = SafeGetLuaNumber(luaState, -1, "period", 0.0);
    validRangeBegin = SafeGetLuaNumber(luaState, -1, "beginDate", 0.0);
    validRangeEnd   = SafeGetLuaNumber(luaState, -1, "endDate", 0.0);

    // Pop the rotations object off the stack
    lua_pop(luaState, 1);

    // Perform some sanity checks on the rotation parameters
    if (validRangeEnd < validRangeBegin)
    {
        clog << "Bad script rotation: valid range end < begin\n";
        return false;
    }

    return true;
}


// Call the position method of the ScriptedRotation object
Quaterniond
ScriptedRotation::spin(double tjd) const
{
    if (tjd != lastTime || !cacheable)
    {
        lua_pushstring(luaState, luaRotationObjectName.c_str());
        lua_gettable(luaState, LUA_GLOBALSINDEX);
        if (lua_istable(luaState, -1))
        {
            lua_pushstring(luaState, "orientation");
            lua_gettable(luaState, -2);
            if (lua_isfunction(luaState, -1))
            {
                lua_pushvalue(luaState, -2); // push 'self' on stack
                lua_pushnumber(luaState, tjd);
                if (lua_pcall(luaState, 2, 4, 0) == 0)
                {
                    lastOrientation = Quaterniond(lua_tonumber(luaState, -4),
                                                  lua_tonumber(luaState, -3),
                                                  lua_tonumber(luaState, -2),
                                                  lua_tonumber(luaState, -1));
#ifdef CELVEC
                    lastOrientation.w = lua_tonumber(luaState, -4);
                    lastOrientation.x = lua_tonumber(luaState, -3);
                    lastOrientation.y = lua_tonumber(luaState, -2);
                    lastOrientation.z = lua_tonumber(luaState, -1);
#endif
                    lua_pop(luaState, 4);
                    lastTime = tjd;
                }
                else
                {
                    // Function call failed for some reason
                    //clog << "ScriptedRotation failed: " << lua_tostring(luaState, -1) << "\n";
                    lua_pop(luaState, 1);
                }
            }
            else
            {
                // Bad orientation function
                lua_pop(luaState, 1);
            }
        }
        else
        {
            // The script rotation object disappeared. OOPS.
        }
    
        // Pop the script rotation object
        lua_pop(luaState, 1);
    }

    return lastOrientation;
}


double
ScriptedRotation::getPeriod() const
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
ScriptedRotation::isPeriodic() const
{
    return period != 0.0;
}


void
ScriptedRotation::getValidRange(double& begin, double& end) const
{
    begin = validRangeBegin;
    end = validRangeEnd;
}
