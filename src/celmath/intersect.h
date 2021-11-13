// intersect.h
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// Intersection calculation for various geometric objects.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cmath>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "mathlib.h"
#include "sphere.h"
#include "ellipsoid.h"

namespace celmath
{

template<class T> bool testIntersection(const Eigen::ParametrizedLine<T, 3>& ray,
                                        const Sphere<T>& sphere,
                                        T& distance)
{
    using std::sqrt;

    Eigen::Matrix<T, 3, 1> diff = ray.origin() - sphere.center;
    T s = static_cast<T>(1) / square(sphere.radius);
    T a = ray.direction().squaredNorm() * s;
    T b = ray.direction().dot(diff) * s;
    T c = diff.squaredNorm() * s - (T) 1.0;
    T disc = b * b - a * c;
    if (disc < static_cast<T>(0))
        return false;

    disc = sqrt(disc);
    T sol0 = (-b + disc) / a;
    T sol1 = (-b - disc) / a;

    if (sol0 > static_cast<T>(0))
    {
        if (sol0 < sol1 || sol1 < static_cast<T>(0))
            distance = sol0;
        else
            distance = sol1;
        return true;
    }
    else if (sol1 > static_cast<T>(0))
    {
        distance = sol1;
        return true;
    }
    else
    {
        return false;
    }
}


template<class T> bool testIntersection(const Eigen::ParametrizedLine<T, 3>& ray,
                                        const Sphere<T>& sphere,
                                        T& distanceToTester,
                                        T& cosAngleToCenter)
{
    if (testIntersection(ray, sphere, distanceToTester))
    {
        cosAngleToCenter = (sphere.center - ray.origin()).dot(ray.direction())
            / (sphere.center - ray.origin()).norm();
        return true;
    }
    return false;
}


template<class T> bool testIntersection(const Eigen::ParametrizedLine<T, 3>& ray,
                                        const Ellipsoid<T>& e,
                                        T& distance)
{
    using std::sqrt;

    Eigen::Matrix<T, 3, 1> diff = ray.origin() - e.center;
    Eigen::Matrix<T, 3, 1> s = e.axes.cwiseInverse().array().square();
    Eigen::Matrix<T, 3, 1> sdir = ray.direction().cwiseProduct(s);
    Eigen::Matrix<T, 3, 1> sdiff = diff.cwiseProduct(s);

    T a = ray.direction().dot(sdir);
    T b = ray.direction().dot(sdiff);
    T c = diff.dot(sdiff) - static_cast<T>(1);
    T disc = b * b - a * c;
    if (disc < static_cast<T>(0))
        return false;

    disc = sqrt(disc);
    T sol0 = (-b + disc) / a;
    T sol1 = (-b - disc) / a;

    if (sol0 > static_cast<T>(0))
    {
        if (sol0 < sol1 || sol1 < static_cast<T>(0))
            distance = sol0;
        else
            distance = sol1;
        return true;
    }
    else if (sol1 > static_cast<T>(0))
    {
        distance = sol1;
        return true;
    }
    else
    {
        return false;
    }
}


template<class T> bool testIntersection(const Eigen::ParametrizedLine<T, 3>& ray,
                                        const Ellipsoid<T>& ellipsoid,
                                        T& distanceToTester,
                                        T& cosAngleToCenter)
{
    if (testIntersection(ray, ellipsoid, distanceToTester))
    {
        cosAngleToCenter = (ellipsoid.center - ray.origin()).dot(ray.direction())
            / (ellipsoid.center - ray.origin()).norm();
        return true;
    }
    return false;
}
} // namespace celmath
