// execution.h
//
// Copyright (C) 2001 Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _EXECUTION_H_
#define _EXECUTION_H_

#include <celengine/execenv.h>
#include <celengine/command.h>


class Execution
{
 public:
    Execution(CommandSequence&, ExecutionEnvironment&);
    
    bool tick(double);
    void reset(CommandSequence&);

 private:
    CommandSequence::const_iterator currentCommand;
    CommandSequence::const_iterator finalCommand;
    ExecutionEnvironment& env;
    double commandTime;
};

#endif // _EXECUTION_H_

