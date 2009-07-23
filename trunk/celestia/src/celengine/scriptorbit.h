// scriptorbit.h
// 
// Copyright (C) 2006, Chris Laurel <claurel@shatters.net>
//
// Interface for a Celestia trajectory implemented via a Lua script.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_SCRIPTORBIT_H_
#define _CELENGINE_SCRIPTORBIT_H_

#include <celengine/parser.h>
#include <celengine/orbit.h>

struct lua_State;

class ScriptedOrbit : public CachingOrbit
{
 public:
    ScriptedOrbit();
    ~ScriptedOrbit();

    bool initialize(const std::string& moduleName,
                    const std::string& funcName,
                    Hash* parameters);

    virtual Eigen::Vector3d computePosition(double tjd) const;
    //virtual Vec3d computeVelocity(double tjd) const;
    virtual bool isPeriodic() const;
    virtual double getPeriod() const;
    virtual double getBoundingRadius() const;
    virtual void getValidRange(double& begin, double& end) const;

 private:
    lua_State* luaState;
    std::string luaOrbitObjectName;
    double boundingRadius;
    double period;
    double validRangeBegin;
    double validRangeEnd;
};

#endif // _CELENGINE_SCRIPTORBIT_H_
