// ellipsoid.h
//
// Copyright (C) 2002-2008, Chris Laurel <claurel@shatters.net>
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
    
    bool contains(const Point3<T>& p) const;
 
 public:
    Point3<T> center;
    Vector3<T> axes;
};

typedef Ellipsoid<float>   Ellipsoidf;
typedef Ellipsoid<double>  Ellipsoidd;


/*! Default Ellipsoid constructor. Create a unit sphere centered
 *  at the origin.
 */
template<class T> Ellipsoid<T>::Ellipsoid() :
    center(0, 0, 0), axes(1, 1, 1)
{
}

/*! Created an ellipsoid with the specified semiaxes, centered
 *  at the origin.
 */
template<class T> Ellipsoid<T>::Ellipsoid(const Vector3<T>& _axes) :
    center(0, 0, 0), axes(_axes)
{
}

/*! Create an ellipsoid with the specified center and semiaxes.
 */
template<class T> Ellipsoid<T>::Ellipsoid(const Point3<T>& _center,
                                          const Vector3<T>& _axes) :
    center(_center), axes(_axes)
{
}

/*! Test whether the point p lies inside the ellipsoid.
 */
template<class T> bool Ellipsoid<T>::contains(const Point3<T>& p) const
{
    Vector3<T> v = p - center;
    v = Vector3<T>(v.x / axes.x, v.y / axes.y, v.z / axes.z);
    return v * v <= (T) 1.0;
}

#endif // _CELMATH_ELLIPSOID_H_

