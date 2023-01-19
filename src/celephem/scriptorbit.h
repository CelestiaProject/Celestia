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
#include "orbit.h"

struct lua_State;

class ScriptedOrbit : public CachingOrbit
{
 public:
    ScriptedOrbit() = default;
    ~ScriptedOrbit() override = default;

    bool initialize(const std::string* moduleName,
                    const std::string& funcName,
                    const Hash* parameters,
                    const fs::path& path);

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

#endif // _CELENGINE_SCRIPTORBIT_H_
