// scriptrotation.h
// 
// Copyright (C) 2006, Chris Laurel <claurel@shatters.net>
//
// Interface for a Celestia rotation model implemented via a Lua script.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELENGINE_SCRIPTROTATION_H_
#define _CELENGINE_SCRIPTROTATION_H_

#include <celengine/parser.h>
#include <celengine/rotation.h>

struct lua_State;

class ScriptedRotation : public RotationModel
{
 public:
    ScriptedRotation();
    ~ScriptedRotation();

    bool initialize(const std::string& moduleName,
                    const std::string& funcName,
                    Hash* parameters);

    virtual Quatd spin(double tjd) const;

    virtual bool isPeriodic() const;
    virtual double getPeriod() const;
    virtual void getValidRange(double& begin, double& end) const;

 private:
    lua_State* luaState;
    std::string luaRotationObjectName;
    double period;
    double validRangeBegin;
    double validRangeEnd;

    // Cached values
    mutable double lastTime;
    mutable Quatd lastOrientation;

    bool cacheable;
};

#endif // _CELENGINE_SCRIPTROTATION_H_
