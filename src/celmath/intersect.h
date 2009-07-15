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

#ifndef _CELMATH_INTERSECT_H_
#define _CELMATH_INTERSECT_H_

#include "ray.h"
#include "sphere.h"
#include "ellipsoid.h"
#include <Eigen/Core>
#include <Eigen/Array>


template<class T> bool testIntersection(const Ray3<T>& ray,
                                        const Sphere<T>& sphere,
                                        T& distance)
{
    Eigen::Matrix<T, 3, 1> diff = ray.origin - sphere.center;
    T s = (T) 1.0 / square(sphere.radius);
    T a = ray.direction.squaredNorm() * s;
    T b = ray.direction.dot(diff) * s;
    T c = diff.squaredNorm() * s - (T) 1.0;
    T disc = b * b - a * c;
    if (disc < 0.0)
        return false;

    disc = (T) sqrt(disc);
    T sol0 = (-b + disc) / a;
    T sol1 = (-b - disc) / a;

    if (sol0 > 0)
    {
        if (sol0 < sol1 || sol1 < 0)
            distance = sol0;
        else
            distance = sol1;
        return true;
    }
    else if (sol1 > 0)
    {
        distance = sol1;
        return true;
    }
    else
    {
        return false;
    }
}


template<class T> bool testIntersection(const Ray3<T>& ray,
                                        const Sphere<T>& sphere,
                                        T& distanceToTester,
                                        T& cosAngleToCenter)
{
    if (testIntersection(ray, sphere, distanceToTester))
    {
        cosAngleToCenter = (sphere.center - ray.origin).dot(ray.direction) / (sphere.center - ray.origin).norm();
        return true;
    }
    return false;
}


template<class T> bool testIntersection(const Ray3<T>& ray,
                                        const Ellipsoid<T>& e,
                                        T& distance)
{
    Eigen::Matrix<T, 3, 1> diff = ray.origin - e.center;
    Eigen::Matrix<T, 3, 1> s = e.axes.cwise().inverse().cwise().square();
    Eigen::Matrix<T, 3, 1> sdir = ray.direction.cwise() * s;
    Eigen::Matrix<T, 3, 1> sdiff = diff.cwise() * s;
    T a = ray.direction.dot(sdir);
    T b = ray.direction.dot(sdiff);
    T c = diff.dot(sdiff) - (T) 1.0;
    T disc = b * b - a * c;
    if (disc < 0.0)
        return false;

    disc = (T) sqrt(disc);
    T sol0 = (-b + disc) / a;
    T sol1 = (-b - disc) / a;

    if (sol0 > 0)
    {
        if (sol0 < sol1 || sol1 < 0)
            distance = sol0;
        else
            distance = sol1;
        return true;
    }
    else if (sol1 > 0)
    {
        distance = sol1;
        return true;
    }
    else
    {
        return false;
    }
}


template<class T> bool testIntersection(const Ray3<T>& ray,
                                        const Ellipsoid<T>& ellipsoid,
                                        T& distanceToTester,
                                        T& cosAngleToCenter)
{
    if (testIntersection(ray, ellipsoid, distanceToTester))
    {
        cosAngleToCenter = (ellipsoid.center - ray.origin).dot(ray.direction) / (ellipsoid.center - ray.origin).norm();
        return true;
    }
    return false;
}

#endif // _CELMATH_INTERSECT_H_
