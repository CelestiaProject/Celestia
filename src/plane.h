// plane.h
// 
// Copyright (C) 2000, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _PLANE_H_
#define _PLANE_H_

#include "celestia.h"
#include "mathlib.h"
#include "vecmath.h"


template<class T> class Plane
{
 public:
    inline Plane();
    inline Plane(const Plane<T>&);
    inline Plane(const Vector3<T>&, T);
    inline Plane(const Vector3<T>&, const Point3<T>&);

    T distanceTo(const Point3<T>&);

 public:
    Vector3<T> normal;
    T d;
};


typedef Plane<float> Planef;
typedef Plane<double> Planed;


template<class T> Plane<T>::Plane() : normal(0, 0, 1), d(0)
{
}

template<class T> Plane<T>::Plane(const Plane<T>& p) :
    normal(p.normal), d(p.d)
{
}

template<class T> Plane<T>::Plane(const Vector3<T>& _normal, T _d) :
    normal(_normal), d(_d)
{
}

template<class T> Plane<T>::Plane(const Vector3<T>& _normal, const Point3<T>& _point) :
    normal(_normal)
{
    d = _normal.x * _point.x + _normal.y * _point.y + _normal.z * _point.z;
}

template<class T> T Plane<T>::distanceTo(const Point3<T>& p)
{
    return normal.x * p.x + normal.y * p.y + normal.z * p.z - d;
}

#endif // _PLANE_H_
