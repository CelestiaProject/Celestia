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

#ifndef _CELMATH_DISTANCE_H_
#define _CELMATH_DISTANCE_H_

#include "mathlib.h"
#include "ray.h"
#include "sphere.h"
#include "ellipsoid.h"
#include <Eigen/Core>

namespace celmath
{
template<class T> T distance(const Eigen::Matrix<T, 3, 1>& p, const Ray3<T>& r)
{
    T t = ((p - r.origin).dot(r.direction)) / r.direction.squaredNorm();
    if (t <= 0)
        return (p - r.origin).norm();
    else
        return (p - r.point(t)).norm();
}
}
#endif // _CELMATH_DISTANCE_H_
