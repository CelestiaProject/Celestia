// ray.h
//
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELMATH_RAY_H_
#define _CELMATH_RAY_H_

#include "vecmath.h"

template<class T> class Ray3
{
 public:
    Ray3();
    Ray3(const Point3<T>&, const Vector3<T>&);
 
    Point3<T> point(T) const;
   
 public:
    Point3<T> origin;
    Vector3<T> direction;
};

typedef Ray3<float>   Ray3f;
typedef Ray3<double>  Ray3d;


template<class T> Ray3<T>::Ray3() :
    origin(0, 0, 0), direction(0, 0, -1)
{
}

template<class T> Ray3<T>::Ray3(const Point3<T>& _origin,
                                const Vector3<T>& _direction) :
    origin(_origin), direction(_direction)
{

}

template<class T> Point3<T> Ray3<T>::point(T t) const
{
    return origin + direction * t;
}

template<class T> Ray3<T> operator*(const Ray3<T>& r, const Matrix3<T>& m)
{
    return Ray3<T>(r.origin * m, r.direction * m);
}

template<class T> Ray3<T> operator*(const Ray3<T>& r, const Matrix4<T>& m)
{
    return Ray3<T>(r.origin * m, r.direction * m);
}

#endif // _CELMATH_RAY_H_

