// value.h
//
// Copyright (C) 2001-2019, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <string>
#include <vector>

namespace celestia
{
namespace engine
{

class Value;
class AssociativeArray;

typedef std::vector<Value*> Array;
typedef std::vector<Value*> ValueArray;
typedef AssociativeArray Hash;

class Value
{
 public:
    enum ValueType
    {
        NullType       = 0,
        NumberType     = 1,
        StringType     = 2,
        ArrayType      = 3,
        HashType       = 4,
        BooleanType    = 5
    };

    Value(double);
    Value(const std::string&);
    Value(ValueArray*);
    Value(Hash*);
    Value(bool);
    Value() = default;
    ~Value();

    ValueType getType() const;

    bool isNull() const;
    double getNumber() const;
    std::string getString() const;
    ValueArray* getArray() const;
    Hash* getHash() const;
    bool getBoolean() const;

 private:
    union Data
    {
        std::string* s;
        double d;
        ValueArray* a;
        Hash* h;
    };

    ValueType type{ NullType };
    Data data;
};

}
}
