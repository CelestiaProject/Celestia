// value.cpp
//
// Copyright (C) 2001-2019, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "value.h"

namespace celestia
{
namespace engine
{

/****** Value method implementations *******/

Value::Value(double d)
{
    type = NumberType;
    data.d = d;
}

Value::Value(const std::string& s)
{
    type = StringType;
    data.s = new std::string(s);
}

Value::Value(ValueArray* a)
{
    type = ArrayType;
    data.a = a;
}

Value::Value(Hash* h)
{
    type = HashType;
    data.h = h;
}

Value::Value(bool b)
{
    type = BooleanType;
    data.d = b ? 1.0 : 0.0;
}

Value::~Value()
{
    if (type == StringType)
    {
        delete data.s;
    }
    else if (type == ArrayType)
    {
        if (data.a != nullptr)
        {
            for (unsigned int i = 0; i < data.a->size(); i++)
                delete (*data.a)[i];
            delete data.a;
        }
    }
    else if (type == HashType)
    {
        if (data.h != nullptr)
        {
#if 0
            Hash::iterator iter = data.h->begin();
            while (iter != data.h->end())
            {
                delete iter->second;
                iter++;
            }
#endif
            delete data.h;
        }
    }
}

Value::ValueType Value::getType() const
{
    return type;
}

double Value::getNumber() const
{
    // ASSERT(type == NumberType);
    return data.d;
}

std::string Value::getString() const
{
    // ASSERT(type == StringType);
    return *data.s;
}

ValueArray* Value::getArray() const
{
    // ASSERT(type == ArrayType);
    return data.a;
}

Hash* Value::getHash() const
{
    // ASSERT(type == HashType);
    return data.h;
}

bool Value::getBoolean() const
{
    // ASSERT(type == BooleanType);
    return (data.d != 0.0);
}

}
}
