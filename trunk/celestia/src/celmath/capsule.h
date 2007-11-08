// capsule.h
//
// Implementation of a capsule bounding volume. A capsule is a sphere
// swept along a line, parameterized in this class as a starting point,
// an axis, and a radius. 
//
// Copyright (C) 2007, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELMATH_CAPSULE_H_
#define _CELMATH_CAPSULE_H_

#include "vecmath.h"

template<class T> class Capsule
{
 public:
    Capsule();
    Capsule(const Vector3<T>& _axis, T _radius);
    Capsule(const Point3<T>& _origin, const Vector3<T>& _axis, T _radius);
 
 public:
    Point3<T> origin;
    Vector3<T> axis;
    T radius;
};

typedef Capsule<float>   Capsulef;
typedef Capsule<double>  Capsuled;


template<class T> Capsule<T>::Capsule() :
    origin(0, 0, 0), axis(0, 0, 0), radius(1)
{
}


template<class T> Capsule<T>::Capsule(const Vector3<T>& _axis,
				      T _radius) :
    origin(0, 0, 0), axis(_axis), radius(_radius)
{
}


template<class T> Capsule<T>::Capsule(const Point3<T>& _origin,
				      const Vector3<T>& _axis,
				      T _radius) :
    origin(_origin), axis(_axis), radius(_radius)
{
}

#endif // _CELMATH_CAPSULE_H_

