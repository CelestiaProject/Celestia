// execenv.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string_view>

class CelestiaCore;
class Renderer;
class Simulation;

namespace celestia::scripts
{

class ExecutionEnvironment
{
 public:
    ExecutionEnvironment() {};
    virtual ~ExecutionEnvironment() {};

    // These three methods should be defined inline in derived classes,
    // although the compiler may need some extra context to not
    // treat them as ordinary out-of-line virtual calls:
    virtual Simulation* getSimulation() const = 0;
    virtual Renderer* getRenderer() const = 0;
    virtual CelestiaCore* getCelestiaCore() const = 0;

    virtual void showText(std::string_view, int, int, int, int, double) = 0;
};

}
