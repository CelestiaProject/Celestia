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
#include <string>
#include <string_view>
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
    AssociativeArray(AssociativeArray&&);
    AssociativeArray(const AssociativeArray&) = delete;
    AssociativeArray& operator=(AssociativeArray&&);
    AssociativeArray& operator=(AssociativeArray&) = delete;

    const Value* getValue(std::string_view) const;
    void addValue(std::string&&, Value&&);

    bool getNumber(std::string_view, double&) const;
    bool getNumber(std::string_view, float&) const;
    bool getNumber(std::string_view, int&) const;
    bool getNumber(std::string_view, std::uint32_t&) const;
    bool getString(std::string_view, std::string&) const;
    bool getPath(std::string_view, fs::path&) const;
    bool getBoolean(std::string_view, bool&) const;
    bool getVector(std::string_view, Eigen::Vector3d&) const;
    bool getVector(std::string_view, Eigen::Vector3f&) const;
    bool getVector(std::string_view, Eigen::Vector4d&) const;
    bool getVector(std::string_view, Eigen::Vector4f&) const;
    bool getRotation(std::string_view, Eigen::Quaternionf&) const;
    bool getColor(std::string_view, Color&) const;
    bool getAngle(std::string_view, double&, double = 1.0, double = 0.0) const;
    bool getAngle(std::string_view, float&, double = 1.0, double = 0.0) const;
    bool getLength(std::string_view, double&, double = 1.0, double = 0.0) const;
    bool getLength(std::string_view, float&, double = 1.0, double = 0.0) const;
    bool getTime(std::string_view, double&, double = 1.0, double = 0.0) const;
    bool getTime(std::string_view, float&, double = 1.0, double = 0.0) const;
    bool getMass(std::string_view, double&, double = 1.0, double = 0.0) const;
    bool getMass(std::string_view, float&, double = 1.0, double = 0.0) const;
    bool getLengthVector(std::string_view, Eigen::Vector3d&, double = 1.0, double = 0.0) const;
    bool getLengthVector(std::string_view, Eigen::Vector3f&, double = 1.0, double = 0.0) const;
    bool getSphericalTuple(std::string_view, Eigen::Vector3d&) const;
    bool getSphericalTuple(std::string_view, Eigen::Vector3f&) const;

    template<typename T>
    void for_all(T action) const
    {
        for (const auto& pair : assoc)
        {
            action(pair.first, values[pair.second]);
        }
    }

 private:
    std::vector<Value> values;
    AssocType assoc;
};

using Hash = AssociativeArray;
