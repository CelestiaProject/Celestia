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

#include "scriptorbit.h"

#include <Eigen/Core>
#include <lua.hpp>

#include <celutil/logger.h>
#include "orbit.h"
#include "scriptobject.h"

using celestia::util::GetLogger;

namespace celestia::ephem
{

namespace
{

class ScriptedOrbit : public CachingOrbit
{
 public:
    ScriptedOrbit(lua_State*,
                  std::string&&,
                  double boundingRadius,
                  double period,
                  double validRangeBegin,
                  double validRangeEnd);
    ~ScriptedOrbit() override = default;

    Eigen::Vector3d computePosition(double tjd) const override;
    // Eigen::Vector3d computeVelocity(double tjd) const override;
    bool isPeriodic() const override;
    double getPeriod() const override;
    double getBoundingRadius() const override;
    void getValidRange(double& begin, double& end) const override;

 private:
    lua_State* luaState{ nullptr };
    std::string luaOrbitObjectName;
    double boundingRadius{ 1.0 };
    double period{ 0.0 };
    double validRangeBegin{ 0.0 };
    double validRangeEnd{ 0.0 };
};


ScriptedOrbit::ScriptedOrbit(lua_State* pLuaState,
                             std::string&& pLuaOrbitObjectName,
                             double pBoundingRadius,
                             double pPeriod,
                             double pValidRangeBegin,
                             double pValidRangeEnd)
    : luaState(pLuaState),
      luaOrbitObjectName(std::move(pLuaOrbitObjectName)),
      boundingRadius(pBoundingRadius),
      period(pPeriod),
      validRangeBegin(pValidRangeBegin),
      validRangeEnd(pValidRangeEnd)
{}


// Call the position method of the ScriptedOrbit object
Eigen::Vector3d
ScriptedOrbit::computePosition(double tjd) const
{
    Eigen::Vector3d pos(Eigen::Vector3d::Zero());
    lua_getglobal(luaState, luaOrbitObjectName.c_str());
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
                pos = Eigen::Vector3d(lua_tonumber(luaState, -3),
                                      lua_tonumber(luaState, -2),
                                      lua_tonumber(luaState, -1));
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
    return Eigen::Vector3d(pos.x(), pos.z(), -pos.y());
}


double
ScriptedOrbit::getPeriod() const
{
    if (period == 0.0)
        return validRangeEnd - validRangeBegin;

    return period;
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

} // end unnamed namespace

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
 *      beginDate, endDate - optional values that specify the time span over
 *         which the orbit is valid. If not given, the orbit is assumed to be
 *          useable at any time. The orbit is invalid if end < begin.
 *      position(time) - The position function takes a time value as input
 *         (TDB Julian day) and returns three values which are the x, y, and
 *         z coordinates. Units for the position are kilometers.
 */
std::unique_ptr<Orbit> CreateScriptedOrbit(const std::string* moduleName,
                                           const std::string& funcName,
                                           const AssociativeArray& parameters,
                                           const fs::path& path)
{
    lua_State* luaState = GetScriptedObjectContext();
    if (luaState == nullptr)
    {
        GetLogger()->warn("ScriptedOrbits are currently disabled.\n");
        return nullptr;
    }

    if (moduleName != nullptr && !moduleName->empty())
    {
        lua_getglobal(luaState, "require");
        if (!lua_isfunction(luaState, -1))
        {
            GetLogger()->error("Cannot load ScriptedOrbit package: 'require' function is unavailable\n");
            lua_pop(luaState, 1);
            return nullptr;
        }

        lua_pushstring(luaState, moduleName->c_str());
        if (lua_pcall(luaState, 1, 1, 0) != 0)
        {
            GetLogger()->error("Failed to load module for ScriptedOrbit: {}\n", lua_tostring(luaState, -1));
            lua_pop(luaState, 1);
            return nullptr;
        }
    }

    // Get the orbit generator function
    lua_getglobal(luaState, funcName.c_str());

    if (lua_isfunction(luaState, -1) == 0)
    {
        // No function with the requested name; pop whatever value we
        // did receive along with the table of arguments.
        lua_pop(luaState, 1);
        GetLogger()->error("No Lua function named {} found.\n", funcName);
        return nullptr;
    }

    // Construct the table that we'll pass to the orbit generator function
    lua_newtable(luaState);

    SetLuaVariables(luaState, parameters);
    // set the addon path
    {
        std::string pathStr = path.string();
        lua_pushstring(luaState, "AddonPath");
        lua_pushstring(luaState, pathStr.c_str());
        lua_settable(luaState, -3);
    }

    // Call the generator function
    if (lua_pcall(luaState, 1, 1, 0) != 0)
    {
        // Some sort of error occurred--the error message is atop the stack
        GetLogger()->error("Error calling ScriptedOrbit generator function: {}\n",
                           lua_tostring(luaState, -1));
        lua_pop(luaState, 1);
        return nullptr;
    }

    if (lua_istable(luaState, -1) == 0)
    {
        // We have an object, but it's not a table. Pop it off the
        // stack and report failure.
        GetLogger()->error("ScriptedOrbit generator function returned bad value.\n");
        lua_pop(luaState, 1);
        return nullptr;
    }

    std::string luaOrbitObjectName = GenerateScriptObjectName();

    // Attach the name to the script orbit
    lua_pushvalue(luaState, -1); // dup the orbit object on top of stack
    lua_setglobal(luaState, luaOrbitObjectName.c_str());

    // Now, call orbit object methods to get the bounding radius
    // and valid time range.
    lua_pushstring(luaState, "boundingRadius");
    lua_gettable(luaState, -2);
    if (lua_isnumber(luaState, -1) == 0)
    {
        GetLogger()->error("Bad or missing boundingRadius for ScriptedOrbit object\n");
        lua_pop(luaState, 1);
        return nullptr;
    }

    double boundingRadius = lua_tonumber(luaState, -1);
    lua_pop(luaState, 1);

    // Get the rest of the orbit parameters; they are all optional.
    double period          = SafeGetLuaNumber(luaState, -1, "period", 0.0);
    double validRangeBegin = SafeGetLuaNumber(luaState, -1, "beginDate", 0.0);
    double validRangeEnd   = SafeGetLuaNumber(luaState, -1, "endDate", 0.0);

    // Pop the orbit object off the stack
    lua_pop(luaState, 1);

    // Perform some sanity checks on the orbit parameters
    if (validRangeEnd < validRangeBegin)
    {
        GetLogger()->error("Bad script orbit: valid range end < begin\n");
        return nullptr;
    }

    if (boundingRadius <= 0.0)
    {
        GetLogger()->error("Bad script object: bounding radius must be positive\n");
        return nullptr;
    }

    return std::make_unique<ScriptedOrbit>(luaState,
                                           std::move(luaOrbitObjectName),
                                           boundingRadius,
                                           period,
                                           validRangeBegin,
                                           validRangeEnd);
}

}
