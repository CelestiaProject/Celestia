// value.cpp
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cassert>
#include <cstdio>
#include <celscript/value.h>
#include <celscript/function.h>

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
    else if (type == FunctionType)
    {
        delete val.funcVal;
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


Value::Value(Function* f) :
    type(FunctionType)
{
    val.funcVal = new Function(*f);
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
        case FunctionType:
            delete val.funcVal;
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
        case NilType:
            break;
        case FunctionType:
            val.funcVal = new Function(*v.val.funcVal);
            break;
        default:
            assert(0);
            break;
        }
    }

    return *this;
}


bool Value::operator==(const Value& v) const
{
    if (v.type != type)
        return false;

    switch (type)
    {
    case BooleanType:
        return v.val.boolVal == val.boolVal;
    case NumberType:
        return v.val.numVal == val.numVal;
    case StringType:
        return *v.val.strVal == *val.strVal;
    case FunctionType:
        return v.val.funcVal == val.funcVal;
    case NilType:
        return true;
    default:
        return false;
    }
}


bool Value::operator!=(const Value& v) const
{
    return !(*this == v);
}


bool Value::toBoolean() const
{
    switch (type)
    {
    case BooleanType:
        return val.boolVal;
    case NumberType:
        return val.numVal != 0.0;
    case StringType:
        return !val.strVal->empty();
    default:
        return false;
    }
}


double Value::toNumber() const
{
    switch (type)
    {
    case BooleanType:
        return val.boolVal ? 1.0 : 0.0;
    case NumberType:
        return val.numVal;
    case StringType:
        return 0.0; // TODO: attempt conversion to number
    default:
        return 0.0;
    }
}


string Value::toString() const
{
    switch (type)
    {
    case BooleanType:
        if (val.boolVal)
            return string("true");
        else
            return string("false");
    case NumberType:
        {
            char buf[256];
            sprintf(buf, "%f", val.numVal);
            return string(buf);
        }

    case StringType:
        return *val.strVal;

    case NilType:
        return string("null");

    default:
        return string("");
    }
}


void Value::output(ostream& out) const
{
    switch (type)
    {
    case NilType:
        out << "null";
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
    case FunctionType:
        out << "[function]";
        break;
    default:
        out << "#unknown";
        break;
    }
}


ostream& operator<<(ostream& out, const celx::Value& v)
{
    v.output(out);
    return out;
}
