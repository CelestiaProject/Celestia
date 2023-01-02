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

#include <cassert>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "hash.h"

class Value;
using ValueArray = std::vector<Value*>;

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

    Value() = default;
    ~Value();
    Value(const Value&) = delete;
    Value(Value&&) = default;
    Value& operator=(const Value&) = delete;
    Value& operator=(Value&&) = default;

    explicit Value(double d) : type(NumberType)
    {
        data.d = d;
    }
    explicit Value(const char *s) : type(StringType)
    {
        data.s = new std::string(s);
    }
    explicit Value(const std::string_view sv) : type(StringType)
    {
        data.s = new std::string(sv);
    }
    explicit Value(const std::string &s) : type(StringType)
    {
        data.s = new std::string(s);
    }
    explicit Value(std::unique_ptr<ValueArray>&& a) : type(ArrayType)
    {
        data.a = a.release();
    }
    explicit Value(std::unique_ptr<Hash>&& h) : type(HashType)
    {
        data.h = h.release();
    }

    // C++ likes implicit conversions to bool, so use template magic
    // to restrict this constructor to exactly bool
    template<typename T, std::enable_if_t<std::is_same_v<T, bool>, int> = 0>
    explicit Value(T b) : type(BooleanType)
    {
        data.d = b ? 1.0 : 0.0;
    }

    ValueType getType() const
    {
        return type;
    }
    bool isNull() const
    {
        return type == NullType;
    }
    double getNumber() const
    {
        assert(type == NumberType);
        return data.d;
    }
    std::string getString() const
    {
        assert(type == StringType);
        return *data.s;
    }
    const ValueArray* getArray() const
    {
        assert(type == ArrayType);
        return data.a;
    }
    const Hash* getHash() const
    {
        assert(type == HashType);
        return data.h;
    }
    bool getBoolean() const
    {
        assert(type == BooleanType);
        return (data.d != 0.0);
    }

 private:
    union Data
    {
        std::string *s;
        double       d;
        ValueArray  *a;
        Hash        *h;
    };

    ValueType type { NullType };
    Data data;
};
