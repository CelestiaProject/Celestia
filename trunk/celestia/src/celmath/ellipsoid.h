// ellipsoid.h
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELMATH_ELLIPSOID_H_
#define _CELMATH_ELLIPSOID_H_

#include "vecmath.h"

template<class T> class Ellipsoid
{
 public:
    Ellipsoid();
    Ellipsoid(const Vector3<T>&);
    Ellipsoid(const Point3<T>&, const Vector3<T>&);
 
 public:
    Point3<T> center;
    Vector3<T> axes;
};

typedef Ellipsoid<float>   Ellipsoidf;
typedef Ellipsoid<double>  Ellipsoidd;


template<class T> Ellipsoid<T>::Ellipsoid() :
    center(0, 0, 0), axes(1, 1, 1)
{
}

template<class T> Ellipsoid<T>::Ellipsoid(const Vector3<T>& _axes) :
    center(0, 0, 0), axes(_axes)
{
}

template<class T> Ellipsoid<T>::Ellipsoid(const Point3<T>& _center,
                                          const Vector3<T>& _axes) :
    center(_center), axes(_axes)
{
}

#endif // _CELMATH_ELLIPSOID_H_

