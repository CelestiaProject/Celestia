// hash.h
//
// Copyright (C) 2001-2019, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <celcompat/filesystem.h>


class Color;
class Value;

class AssociativeArray
{
 public:
    using AssocType = std::map<std::string, std::size_t, std::less<>>;

    AssociativeArray() = default;
    ~AssociativeArray();
    AssociativeArray(AssociativeArray&&) = delete;
    AssociativeArray(const AssociativeArray&) = delete;
    AssociativeArray& operator=(AssociativeArray&&) = delete;
    AssociativeArray& operator=(AssociativeArray&) = delete;

    const Value* getValue(std::string_view) const;
    void addValue(std::string&&, Value&&);

    template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
    std::optional<T> getNumber(std::string_view key) const
    {
        auto number = getNumberImpl(key);
        if constexpr (std::is_same_v<std::remove_cv_t<T>, double>)
            return number;
        else
            return convertNumeric<T>(number);
    }

    const std::string* getString(std::string_view) const;
    std::optional<fs::path> getPath(std::string_view) const;
    std::optional<bool> getBoolean(std::string_view) const;

    template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
    std::optional<Eigen::Matrix<T, 3, 1>> getVector3(std::string_view key) const
    {
        auto vec3 = getVector3Impl(key);
        if constexpr (std::is_same_v<std::remove_cv_t<T>, double>)
            return vec3;
        else
            return convertMatrix<T>(vec3);
    }

    template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
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

    template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
    std::optional<T> getAngle(std::string_view key, double outputScale = 1.0, double defaultScale = 0.0) const
    {
        auto angle = getAngleImpl(key, outputScale, defaultScale);
        if constexpr (std::is_same_v<std::remove_cv_t<T>, double>)
            return angle;
        else
            return convertNumeric<T>(angle);
    }

    template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
    std::optional<T> getLength(std::string_view key, double outputScale = 1.0, double defaultScale = 0.0) const
    {
        auto length = getLengthImpl(key, outputScale, defaultScale);
        if constexpr (std::is_same_v<std::remove_cv_t<T>, double>)
            return length;
        else
            return convertNumeric<T>(length);
    }

    template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
    std::optional<T> getTime(std::string_view key, double outputScale = 1.0, double defaultScale = 0.0) const
    {
        auto time = getTimeImpl(key, outputScale, defaultScale);
        if constexpr (std::is_same_v<std::remove_cv_t<T>, double>)
            return time;
        else
            return convertNumeric<T>(time);
    }

    template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
    std::optional<T> getMass(std::string_view key, double outputScale = 1.0, double defaultScale = 0.0) const
    {
        auto mass = getMassImpl(key, outputScale, defaultScale);
        if constexpr (std::is_same_v<std::remove_cv_t<T>, double>)
            return mass;
        else
            return convertNumeric<T>(mass);
    }

    template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
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

    template<typename T>
    void for_all(T action) const
    {
        for (const auto& [key, value] : assoc)
        {
            action(key, values[value]);
        }
    }

 private:
    // At this point, Value is an incomplete type. C++17 allows us to store
    // this in a vector but not a map, so use the map to store vector indices
    std::vector<Value> values;
    AssocType assoc;

    std::optional<double> getNumberImpl(std::string_view) const;
    std::optional<Eigen::Vector3d> getVector3Impl(std::string_view) const;
    std::optional<Eigen::Vector4d> getVector4Impl(std::string_view) const;

    std::optional<double> getAngleImpl(std::string_view, double, double) const;
    std::optional<double> getLengthImpl(std::string_view, double, double) const;
    std::optional<double> getTimeImpl(std::string_view, double, double) const;
    std::optional<double> getMassImpl(std::string_view, double, double) const;

    std::optional<Eigen::Vector3d> getLengthVectorImpl(std::string_view, double, double) const;

    template<typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
    inline static std::optional<T>
    convertNumeric(const std::optional<double>& maybeDouble)
    {
        return maybeDouble.has_value()
            ? std::optional<T>{ static_cast<T>(*maybeDouble) }
            : std::nullopt;
    }

    template<typename T, int X, int Y,
             std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
    inline static std::optional<Eigen::Matrix<T, X, Y>>
    convertMatrix(const std::optional<Eigen::Matrix<double, X, Y>>& maybeMatrix)
    {
        return maybeMatrix.has_value()
            ? std::make_optional<Eigen::Matrix<T, X, Y>>(maybeMatrix->template cast<T>())
            : std::nullopt;
    }
};

using Hash = AssociativeArray;
