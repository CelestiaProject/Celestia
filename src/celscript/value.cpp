// value.cpp
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include <celscript/value.h>

using namespace std;
using namespace celx;


Value::Value() :
    type(NilType)
{
}


Value::~Value()
{
    if (type == StringType)
    {
        delete val.strVal;
    }
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


Value::Value(bool x) :
    type(BooleanType)
{
    val.boolVal = x;
}


// Copy constructor
Value::Value(const Value& v) :
    type(NilType)
{
    *this = v;
}


Value& Value::operator=(const Value& v)
{
    if (this != &v)
    {
        // Clean up the current contents first
        switch (type)
        {
        case StringType:
            delete val.strVal;
            break;
        default:
            break;
        }

        type = v.type;
        switch (type)
        {
        case NumberType:
            val.numVal = v.val.numVal;
            break;
        case StringType:
            val.strVal = new string(*v.val.strVal);
            break;
        case BooleanType:
            val.boolVal = v.val.boolVal;
            break;
        default:
            assert(0);
            break;
        }
    }

    return *this;
}


void Value::output(ostream& out) const
{
    switch (type)
    {
    case NilType:
        out << "nil";
        break;
    case BooleanType:
        if (val.boolVal)
            out << "true";
        else
            out << "false";
        break;
    case NumberType:
        out << val.numVal;
        break;
    case StringType:
        out << '"' << *val.strVal << '"';
        break;
    default:
        out << "#unknown";
        break;
    }
}


ostream& operator<<(ostream& out, const Value& v)
{
    v.output(out);
    return out;
}
