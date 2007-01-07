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


template<class T> bool testIntersection(const Ray3<T>& ray,
                                        const Sphere<T>& sphere,
                                        T& distance)
{
    Vector3<T> diff = ray.origin - sphere.center;
    T s = (T) 1.0 / square(sphere.radius);
    T a = ray.direction * ray.direction * s;
    T b = ray.direction * diff * s;
    T c = diff * diff * s - (T) 1.0;
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
        cosAngleToCenter    = (sphere.center - ray.origin)*ray.direction/(sphere.center - ray.origin).length();
        return true;
    }
    return false;
}


template<class T> bool testIntersection(const Ray3<T>& ray,
                                        const Ellipsoid<T>& e,
                                        T& distance)
{
    Vector3<T> diff = ray.origin - e.center;
    Vector3<T> s((T) 1.0 / square(e.axes.x),
                 (T) 1.0 / square(e.axes.y),
                 (T) 1.0 / square(e.axes.z));
    Vector3<T> sdir(ray.direction.x * s.x,
                    ray.direction.y * s.y,
                    ray.direction.z * s.z);
    Vector3<T> sdiff(diff.x * s.x, diff.y * s.y, diff.z * s.z);
    T a = ray.direction * sdir;
    T b = ray.direction * sdiff;
    T c = diff * sdiff - (T) 1.0;
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
        cosAngleToCenter  = (ellipsoid.center - ray.origin)*ray.direction/(ellipsoid.center - ray.origin).length();
        return true;
    }
    return false;
}

#endif // _CELMATH_INTERSECT_H_
