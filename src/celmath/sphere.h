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

template<class T> class Sphere
{
 public:
    Sphere();
    Sphere(T);
    Sphere(const Point3<T>&, T);
 
 public:
    Point3<T> center;
    T radius;
};

typedef Sphere<float>   Spheref;
typedef Sphere<double>  Sphered;


template<class T> Sphere<T>::Sphere() :
    center(0, 0, 0), radius(1)
{
}

template<class T> Sphere<T>::Sphere(T _radius) :
    center(0, 0, 0), radius(_radius)
{
}

template<class T> Sphere<T>::Sphere(const Point3<T>& _center, T _radius) :
    center(_center), radius(_radius)
{
}

#endif // _CELMATH_SPHERE_H_

