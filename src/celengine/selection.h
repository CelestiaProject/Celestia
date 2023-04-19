// selection.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cstddef>
#include <functional>
#include <string>

#include <Eigen/Core>

#include <celengine/univcoord.h>

class AstroObject;
class Star;
class Body;
class Location;
class DeepSkyObject;

enum class SelectionType
{
    Nil,
    Star,
    Body,
    DeepSky,
    Location,
};

class Selection
{
 public:
    Selection() = default;
    Selection(Star* star) : type(SelectionType::Star), obj(star) { checkNull(); };
    Selection(Body* body) : type(SelectionType::Body), obj(body) { checkNull(); };
    Selection(DeepSkyObject* deepsky) : type(SelectionType::DeepSky), obj(deepsky) {checkNull(); };
    Selection(Location* location) : type(SelectionType::Location), obj(location) { checkNull(); };
    ~Selection() = default;

    bool empty() const { return type == SelectionType::Nil; }
    double radius() const;
    UniversalCoord getPosition(double t) const;
    Eigen::Vector3d getVelocity(double t) const;
    std::string getName(bool i18n = false) const;
    Selection parent() const;

    bool isVisible() const;

    Star* star() const
    {
        return type == SelectionType::Star ? static_cast<Star*>(obj) : nullptr;
    }

    Body* body() const
    {
        return type == SelectionType::Body ? static_cast<Body*>(obj) : nullptr;
    }

    DeepSkyObject* deepsky() const
    {
        return type == SelectionType::DeepSky ? static_cast<DeepSkyObject*>(obj) : nullptr;
    }

    Location* location() const
    {
        return type == SelectionType::Location ? static_cast<Location*>(obj) : nullptr;
    }

    AstroObject *object() const
    {
        return type != SelectionType::Nil ? static_cast<AstroObject*>(obj) : nullptr;
    }

    SelectionType getType() const { return type; }

 private:
    SelectionType type { SelectionType::Nil };
    void* obj { nullptr };

    void checkNull() { if (obj == nullptr) type = SelectionType::Nil; }

    friend bool operator==(const Selection& s0, const Selection& s1);
    friend bool operator!=(const Selection& s0, const Selection& s1);
    friend struct std::hash<Selection>;
};


inline bool operator==(const Selection& s0, const Selection& s1)
{
    return s0.type == s1.type && s0.obj == s1.obj;
}

inline bool operator!=(const Selection& s0, const Selection& s1)
{
    return s0.type != s1.type || s0.obj != s1.obj;
}

namespace std
{
template<>
struct hash<Selection>
{
    std::size_t operator()(const Selection& sel) const noexcept
    {
        return std::hash<const void*>()(sel.obj);
    }
};
};
