// execution.h
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celscript/execution.h>

using namespace celx;


ExecutionContext::ExecutionContext(Environment* env) :
    globalEnv(env)
{
}


ExecutionContext::~ExecutionContext()
{
}


Environment* ExecutionContext::getEnvironment()
{
    return globalEnv;
}
