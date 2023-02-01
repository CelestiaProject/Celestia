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

/****** Value method implementations *******/

Value::Value(Value&& other) noexcept
    : type(other.type),
      units(other.units),
      data(other.data)
{
    other.type = ValueType::NullType;
}


Value& Value::operator=(Value&& other) noexcept
{
    if (this != &other)
    {
        switch (type)
        {
        case ValueType::StringType:
            delete data.s; //NOSONAR
            break;
        case ValueType::ArrayType:
            delete data.a; //NOSONAR
            break;
        case ValueType::HashType:
            delete data.h; //NOSONAR
            break;
        default:
            break;
        }
        type = other.type;
        units = other.units;
        data = other.data;
        other.type = ValueType::NullType;
    }

    return *this;
}


Value::~Value()
{
    switch (type)
    {
    case ValueType::StringType:
        delete data.s;
        break;
    case ValueType::ArrayType:
        delete data.a;
        break;
    case ValueType::HashType:
        delete data.h;
        break;
    default:
        break;
    }
}
