// execution.h
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef CELSCRIPT_EXECUTION_H_
#define CELSCRIPT_EXECUTION_H_

#include <celscript/celx.h>
#include <celscript/environment.h>


namespace celx
{

class ExecutionContext
{
 public:
    ExecutionContext(Environment*);
    ~ExecutionContext();

    Environment* getEnvironment();
    void runtimeError() {};

 private:
    Environment* globalEnv;
};

} // namespace celx

#endif // CELSCRIPT_EXECUTION_H_
