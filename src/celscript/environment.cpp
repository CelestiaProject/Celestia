// environment.cpp
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "environment.h"

using namespace std;
using namespace celx;


Environment::Environment()
{
}

Environment::~Environment()
{
}



GlobalEnvironment::GlobalEnvironment()
{
}

GlobalEnvironment::~GlobalEnvironment()
{
}


void GlobalEnvironment::bind(const string& name, const Value& value)
{
    bindings.insert(map<string, Value*>::value_type(name, new Value(value)));
}

Value* GlobalEnvironment::lookup(const string& name) const
{
    map<string, Value*>::const_iterator iter = bindings.find(name);
    if (iter == bindings.end())
        return NULL;
    else
        return iter->second;
}


Environment* GlobalEnvironment::getParent() const
{
    return NULL;
}



