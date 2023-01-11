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
        return getNumberImpl(key);
    }

    const std::string* getString(std::string_view) const;
    std::optional<fs::path> getPath(std::string_view) const;
    std::optional<bool> getBoolean(std::string_view) const;

    template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
    std::optional<Eigen::Matrix<T, 3, 1>> getVector3(std::string_view key) const
    {
        if constexpr (std::is_same_v<T, double>)
        {
            return getVector3Impl(key);
        }
        else if (auto vec3 = getVector3Impl(key); vec3.has_value())
        {
            return std::make_optional<Eigen::Matrix<T, 3, 1>>(vec3->cast<T>());
        }
        else
        {
            return std::nullopt;
        }
    }

    template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
    std::optional<Eigen::Matrix<T, 4, 1>> getVector4(std::string_view key) const
    {
        if constexpr (std::is_same_v<T, double>)
        {
            return getVector4Impl(key);
        }
        else if (auto vec4 = getVector4Impl(key); vec4.has_value())
        {
            return std::make_optional<Eigen::Matrix<T, 4, 1>>(vec4->cast<T>());
        }
        else
        {
            return std::nullopt;
        }
    }

    std::optional<Eigen::Quaternionf> getRotation(std::string_view) const;

    std::optional<Color> getColor(std::string_view) const;

    template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
    std::optional<T> getAngle(std::string_view key, double outputScale = 1.0, double defaultScale = 0.0) const
    {
        return getAngleImpl(key, outputScale, defaultScale);
    }

    template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
    std::optional<T> getLength(std::string_view key, double outputScale = 1.0, double defaultScale = 0.0) const
    {
        return getLengthImpl(key, outputScale, defaultScale);
    }

    template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
    std::optional<T> getTime(std::string_view key, double outputScale = 1.0, double defaultScale = 0.0) const
    {
        return getTimeImpl(key, outputScale, defaultScale);
    }

    template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
    std::optional<T> getMass(std::string_view key, double outputScale = 1.0, double defaultScale = 0.0) const
    {
        return getMassImpl(key, outputScale, defaultScale);
    }

    template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
    std::optional<Eigen::Matrix<T, 3, 1>> getLengthVector(std::string_view key,
                                                          double outputScale = 1.0,
                                                          double defaultScale = 0.0) const
    {
        if constexpr (std::is_same_v<T, double>)
        {
            return getLengthVectorImpl(key, outputScale, defaultScale);
        }
        else if (auto vec = getLengthVectorImpl(key, outputScale, defaultScale); vec.has_value())
        {
            return std::make_optional<Eigen::Matrix<T, 3, 1>>(vec->cast<T>());
        }
        else
        {
            return std::nullopt;
        }
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
};

using Hash = AssociativeArray;
