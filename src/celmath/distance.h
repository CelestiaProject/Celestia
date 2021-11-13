// distance.h
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// Distance calculation for various geometric objects.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>

namespace celmath
{
template<class T> T distance(const Eigen::Matrix<T, 3, 1>& p, const Eigen::ParametrizedLine<T, 3>& r)
{
    T t = ((p - r.origin()).dot(r.direction())) / r.direction().squaredNorm();
    if (t <= static_cast<T>(0))
        return (p - r.origin()).norm();
    else
        return (p - r.pointAt(t)).norm();
}
}
