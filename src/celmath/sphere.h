// sphere.h
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELMATH_SPHERE_H_
#define _CELMATH_SPHERE_H_

#include "vecmath.h"
#include <Eigen/Core>

template<class T> class Sphere
{
 public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    Sphere() :
        center(0, 0, 0),
        radius(1)
    {
    }

    Sphere(T _radius) :
        center(0, 0, 0),
        radius(_radius)
    {
    }

    Sphere(const Eigen::Matrix<T, 3, 1> _center, T _radius) :
        center(_center),
        radius(_radius)
    {
    }

    // Compatibility
    Sphere(const Point3<T>& _center, T _radius) :
        center(_center.x, _center.y, _center.z),
        radius(_radius)
    {
    }
 
 public:
    Eigen::Matrix<T, 3, 1> center;
    T radius;
};

typedef Sphere<float>   Spheref;
typedef Sphere<double>  Sphered;

#endif // _CELMATH_SPHERE_H_

