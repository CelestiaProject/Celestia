// value.cpp
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <celscript/value.h>

using namespace std;
using namespace celx;


Value::Value() :
    type(NilType)
{
}

Value::Value(double x) :
    type(NumberType)
{
    val.numVal = x;
}

Value::Value(const string& x) :
    type(StringType)
{
    val.strVal = new string(x);
}
