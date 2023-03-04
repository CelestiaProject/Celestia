// execution.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstddef>

#include "command.h"

namespace celestia::scripts
{

class ExecutionEnvironment;

class Execution
{
 public:
    Execution(CommandSequence&&, ExecutionEnvironment&);

    bool tick(double);

 private:
    CommandSequence commandSequence;
    std::size_t currentCommand{ 0 };
    ExecutionEnvironment& env;
    double commandTime{ -1.0 };
};

} // end namespace celestia::scripts
