// sphere.h
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <Eigen/Core>

namespace celmath
{

template<class T> class Sphere
{
 public:
    Sphere() :
        center(Eigen::Matrix<T, 3, 1>::Zero()),
        radius(static_cast<T>(1))
    {
    }

    Sphere(T _radius) :
        center(Eigen::Matrix<T, 3, 1>::Zero()),
        radius(_radius)
    {
    }

    Sphere(const Eigen::Matrix<T, 3, 1> _center, T _radius) :
        center(_center),
        radius(_radius)
    {
    }

 public:
    Eigen::Matrix<T, 3, 1> center;
    T radius;
};

using Spheref = Sphere<float>;
using Sphered = Sphere<double>;
} // namespace celmath
