// associativearray.h
//
// Copyright (C) 2001-2019, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <concepts>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <celastro/units.h>
#include "classops.h"

class Color;

namespace celestia::util
{

enum class ValueType : std::int32_t
{
    NullType       = 0,
    NumberType     = 1,
    StringType     = 2,
    ArrayType      = 3,
    HashType       = 4,
    BooleanType    = 5,
};

// Value acts as a custom variant type which stores the units data in what
// would otherwise be padding between the discriminant and the union data.
// Single ownership of the contained value is enforced via the constructors,
// which either obtain ownership by releasing a unique-ptr, or create a new
// copied object.

class AssociativeArray;

class Value;
using ValueArray = std::vector<Value>;

class Value : private NoCopy
{
public:
    struct Units
    {
        astro::LengthUnit length{ astro::LengthUnit::Default };
        astro::TimeUnit time{ astro::TimeUnit::Default };
        astro::AngleUnit angle{ astro::AngleUnit::Default };
        astro::MassUnit mass{ astro::MassUnit::Default };
    };

    // Cannot default this constructor due to non-trivial default constructors
    // of the union members.
    Value() {} //NOSONAR
    ~Value();
    Value(Value&&) noexcept;
    Value& operator=(Value&&) noexcept;

    explicit Value(double);
    explicit Value(const char*);
    explicit Value(std::string_view);
    explicit Value(const std::string&);
    explicit Value(std::string&&);
    explicit Value(std::unique_ptr<const ValueArray>&&);
    explicit Value(std::unique_ptr<const AssociativeArray>&&);

    // C++ likes implicit conversions to bool, so use concepts magic
    // to restrict this constructor to exactly bool
    explicit Value(std::same_as<bool> auto b) : type(ValueType::BooleanType)
    {
        std::construct_at(&doubleData, b ? 1.0 : 0.0);
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
            ? std::make_optional(doubleData)
            : std::nullopt;
    }

    const std::string* getString() const
    {
        return type == ValueType::StringType
            ? stringData.get()
            : nullptr;
    }

    const ValueArray* getArray() const
    {
        return type == ValueType::ArrayType
            ? arrayData.get()
            : nullptr;
    }

    const AssociativeArray* getHash() const
    {
        return type == ValueType::HashType
            ? hashData.get()
            : nullptr;
    }

    std::optional<bool> getBoolean() const
    {
        return type == ValueType::BooleanType
            ? std::make_optional(doubleData != 0.0)
            : std::nullopt;
    }

    astro::LengthUnit getLengthUnit() const { return units.length; }
    astro::TimeUnit getTimeUnit() const { return units.time; }
    astro::AngleUnit getAngleUnit() const { return units.angle; }
    astro::MassUnit getMassUnit() const { return units.mass; }

private:
    void constructValueFrom(Value&);
    void destroy();

    ValueType type{ ValueType::NullType };
    Units units;

    union //NOSONAR
    {
        double                                  doubleData;
        std::unique_ptr<const std::string>      stringData;
        std::unique_ptr<const ValueArray>       arrayData;
        std::unique_ptr<const AssociativeArray> hashData;
    };
};

class AssociativeArray : private NoCopy
{
public:
    AssociativeArray() = default;
    ~AssociativeArray() = default;

    const Value* getValue(std::string_view) const;
    void addValue(std::string&&, Value&&);

    template<typename T>
        requires std::integral<T> || std::floating_point<T>
    std::optional<T> getNumber(std::string_view key) const
    {
        auto number = getNumberImpl(key);
        if constexpr (std::is_same_v<std::remove_cv_t<T>, double>)
            return number;
        else
            return convertNumeric<T>(number);
    }

    const std::string* getString(std::string_view) const;
    std::optional<std::filesystem::path> getPath(std::string_view) const;
    std::optional<bool> getBoolean(std::string_view) const;

    template<std::floating_point T>
    std::optional<Eigen::Matrix<T, 3, 1>> getVector3(std::string_view key) const
    {
        auto vec3 = getVector3Impl(key);
        if constexpr (std::is_same_v<std::remove_cv_t<T>, double>)
            return vec3;
        else
            return convertMatrix<T>(vec3);
    }

    template<std::floating_point T>
    std::optional<Eigen::Matrix<T, 4, 1>> getVector4(std::string_view key) const
    {
        auto vec4 = getVector4Impl(key);
        if constexpr (std::is_same_v<std::remove_cv_t<T>, double>)
            return vec4;
        else
            return convertMatrix<T>(vec4);
    }

    std::optional<Eigen::Quaternionf> getRotation(std::string_view) const;

    std::optional<Color> getColor(std::string_view) const;

    template<std::floating_point T>
    std::optional<T> getAngle(std::string_view key, double outputScale = 1.0, double defaultScale = 0.0) const
    {
        auto angle = getAngleImpl(key, outputScale, defaultScale);
        if constexpr (std::is_same_v<std::remove_cv_t<T>, double>)
            return angle;
        else
            return convertNumeric<T>(angle);
    }

    template<std::floating_point T>
    std::optional<T> getLength(std::string_view key, double outputScale = 1.0, double defaultScale = 0.0) const
    {
        auto length = getLengthImpl(key, outputScale, defaultScale);
        if constexpr (std::is_same_v<std::remove_cv_t<T>, double>)
            return length;
        else
            return convertNumeric<T>(length);
    }

    template<std::floating_point T>
    std::optional<T> getTime(std::string_view key, double outputScale = 1.0, double defaultScale = 0.0) const
    {
        auto time = getTimeImpl(key, outputScale, defaultScale);
        if constexpr (std::is_same_v<std::remove_cv_t<T>, double>)
            return time;
        else
            return convertNumeric<T>(time);
    }

    template<std::floating_point T>
    std::optional<T> getMass(std::string_view key, double outputScale = 1.0, double defaultScale = 0.0) const
    {
        auto mass = getMassImpl(key, outputScale, defaultScale);
        if constexpr (std::is_same_v<std::remove_cv_t<T>, double>)
            return mass;
        else
            return convertNumeric<T>(mass);
    }

    template<std::floating_point T>
    std::optional<Eigen::Matrix<T, 3, 1>> getLengthVector(std::string_view key,
                                                          double outputScale = 1.0,
                                                          double defaultScale = 0.0) const
    {
        auto vec = getLengthVectorImpl(key, outputScale, defaultScale);
        if constexpr (std::is_same_v<std::remove_cv_t<T>, double>)
            return vec;
        else
            return convertMatrix<T>(vec);
    }

    std::optional<Eigen::Vector3d> getSphericalTuple(std::string_view) const;

    auto items() const
    {
        return assoc
             | std::views::transform([](const auto& p)
                                     {
                                         return std::make_pair(std::string_view{p.first}, std::cref(p.second));
                                     });
    }

private:
    std::map<std::string, Value, std::less<>> assoc;

    std::optional<double> getNumberImpl(std::string_view) const;
    std::optional<Eigen::Vector3d> getVector3Impl(std::string_view) const;
    std::optional<Eigen::Vector4d> getVector4Impl(std::string_view) const;

    std::optional<double> getAngleImpl(std::string_view, double, double) const;
    std::optional<double> getLengthImpl(std::string_view, double, double) const;
    std::optional<double> getTimeImpl(std::string_view, double, double) const;
    std::optional<double> getMassImpl(std::string_view, double, double) const;

    std::optional<Eigen::Vector3d> getLengthVectorImpl(std::string_view, double, double) const;

    template<typename T>
        requires std::integral<T> || std::floating_point<T>
    inline static std::optional<T>
    convertNumeric(const std::optional<double>& maybeDouble)
    {
        return maybeDouble.has_value()
            ? std::optional<T>{ static_cast<T>(*maybeDouble) }
            : std::nullopt;
    }

    template<std::floating_point T, int X, int Y>
    inline static std::optional<Eigen::Matrix<T, X, Y>>
    convertMatrix(const std::optional<Eigen::Matrix<double, X, Y>>& maybeMatrix)
    {
        return maybeMatrix.has_value()
            ? std::make_optional<Eigen::Matrix<T, X, Y>>(maybeMatrix->template cast<T>())
            : std::nullopt;
    }
};

} // end namespace celestia::util
