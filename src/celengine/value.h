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
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "astro.h"
#include "hash.h"

enum class ValueType : std::uint8_t
{
    NullType       = 0,
    NumberType     = 1,
    StringType     = 2,
    ArrayType      = 3,
    HashType       = 4,
    BooleanType    = 5
};

class Value;
using ValueArray = std::vector<Value>;

// Value acts as a custom variant type which stores the units data in what
// would otherwise be padding between the discriminant and the union data.
// Single ownership of the contained value is enforced via the constructors,
// which either obtain ownership by releasing a unique-ptr, or create a new
// copied object.
// Lines that trigger Sonar rules forbidding manual memory management are
// marked as NOSONAR, as the use of new/delete is inherent to the functioning
// of the class.

class Value //NOSONAR
{
 public:
    struct Units
    {
        astro::LengthUnit length{ astro::LengthUnit::Default };
        astro::TimeUnit time{ astro::TimeUnit::Default };
        astro::AngleUnit angle{ astro::AngleUnit::Default };
        astro::MassUnit mass{ astro::MassUnit::Default };
    };

    Value() = default;
    ~Value();
    Value(const Value&) = delete;
    Value(Value&&) noexcept;
    Value& operator=(const Value&) = delete;
    Value& operator=(Value&&) noexcept;

    explicit Value(double d) : type(ValueType::NumberType)
    {
        data.d = d;
    }
    explicit Value(const char *s) : type(ValueType::StringType)
    {
        data.s = new std::string(s); //NOSONAR
    }
    explicit Value(const std::string_view sv) : type(ValueType::StringType)
    {
        data.s = new std::string(sv); //NOSONAR
    }
    explicit Value(const std::string &s) : type(ValueType::StringType)
    {
        data.s = new std::string(s); // NOSONAR
    }
    explicit Value(std::string&& s) : type(ValueType::StringType)
    {
        data.s = new std::string(std::move(s)); //NOSONAR
    }
    explicit Value(std::unique_ptr<ValueArray>&& a) : type(ValueType::ArrayType)
    {
        data.a = a.release();
    }
    explicit Value(std::unique_ptr<Hash>&& h) : type(ValueType::HashType)
    {
        data.h = h.release();
    }

    // C++ likes implicit conversions to bool, so use template magic
    // to restrict this constructor to exactly bool
    template<typename T, std::enable_if_t<std::is_same_v<T, bool>, int> = 0>
    explicit Value(T b) : type(ValueType::BooleanType)
    {
        data.d = b ? 1.0 : 0.0;
    }

    void setUnits(Units _units) { units = _units; }

    ValueType getType() const
    {
        return type;
    }
    bool isNull() const
    {
        return type == ValueType::NullType;
    }
    std::optional<double> getNumber() const
    {
        return type == ValueType::NumberType
            ? std::make_optional(data.d)
            : std::nullopt;
    }
    const std::string* getString() const
    {
        return type == ValueType::StringType
            ? data.s
            : nullptr;
    }
    const ValueArray* getArray() const
    {
        return type == ValueType::ArrayType
            ? data.a
            : nullptr;
    }
    const Hash* getHash() const
    {
        return type == ValueType::HashType
            ? data.h
            : nullptr;
    }
    std::optional<bool> getBoolean() const
    {
        return type == ValueType::BooleanType
            ? std::make_optional(data.d != 0.0)
            : std::nullopt;
    }

    astro::LengthUnit getLengthUnit() const { return units.length; }
    astro::TimeUnit getTimeUnit() const { return units.time; }
    astro::AngleUnit getAngleUnit() const { return units.angle; }
    astro::MassUnit getMassUnit() const { return units.mass; }

 private:
    union Data
    {
        const std::string *s;
        double             d;
        const ValueArray  *a;
        const Hash        *h;
    };

    ValueType type { ValueType::NullType };
    Units units{ };
    Data data;
};
