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

#include <cstdint>
#include <map>
#include <string>
#include <celcompat/filesystem.h>
#include <celmath/mathlib.h>
#include <Eigen/Geometry>


class Color;
class Value;

using HashIterator = std::map<std::string, Value*>::const_iterator;

class AssociativeArray
{
 public:
    AssociativeArray() = default;
    ~AssociativeArray();
    AssociativeArray(AssociativeArray&&) = default;
    AssociativeArray(const AssociativeArray&) = delete;
    AssociativeArray& operator=(AssociativeArray&&) = default;
    AssociativeArray& operator=(AssociativeArray&) = delete;

    Value* getValue(const std::string&) const;
    void addValue(const std::string&, Value&);

    bool getNumber(const std::string&, double&) const;
    bool getNumber(const std::string&, float&) const;
    bool getNumber(const std::string&, int&) const;
    bool getNumber(const std::string&, std::uint32_t&) const;
    bool getString(const std::string&, std::string&) const;
    bool getPath(const std::string&, fs::path&) const;
    bool getBoolean(const std::string&, bool&) const;
    bool getVector(const std::string&, Eigen::Vector3d&) const;
    bool getVector(const std::string&, Eigen::Vector3f&) const;
    bool getVector(const std::string&, Eigen::Vector4d&) const;
    bool getVector(const std::string&, Eigen::Vector4f&) const;
    bool getRotation(const std::string&, Eigen::Quaternionf&) const;
    bool getColor(const std::string&, Color&) const;
    bool getAngle(const std::string&, double&, double = 1.0, double = 0.0) const;
    bool getAngle(const std::string&, float&, double = 1.0, double = 0.0) const;
    bool getLength(const std::string&, double&, double = 1.0, double = 0.0) const;
    bool getLength(const std::string&, float&, double = 1.0, double = 0.0) const;
    bool getTime(const std::string&, double&, double = 1.0, double = 0.0) const;
    bool getTime(const std::string&, float&, double = 1.0, double = 0.0) const;
    bool getMass(const std::string&, double&, double = 1.0, double = 0.0) const;
    bool getMass(const std::string&, float&, double = 1.0, double = 0.0) const;
    bool getLengthVector(const std::string&, Eigen::Vector3d&, double = 1.0, double = 0.0) const;
    bool getLengthVector(const std::string&, Eigen::Vector3f&, double = 1.0, double = 0.0) const;
    bool getSphericalTuple(const std::string&, Eigen::Vector3d&) const;
    bool getSphericalTuple(const std::string&, Eigen::Vector3f&) const;
    bool getAngleScale(const std::string&, double&) const;
    bool getAngleScale(const std::string&, float&) const;
    bool getLengthScale(const std::string&, double&) const;
    bool getLengthScale(const std::string&, float&) const;
    bool getTimeScale(const std::string&, double&) const;
    bool getTimeScale(const std::string&, float&) const;
    bool getMassScale(const std::string&, double&) const;
    bool getMassScale(const std::string&, float&) const;

    HashIterator begin() const
    {
        return assoc.begin();
    }
    HashIterator end() const
    {
        return assoc.end();
    }

 private:
    std::map<std::string, Value*> assoc;
};

using Hash = AssociativeArray;
