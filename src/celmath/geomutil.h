// geomutil.h
//
// Copyright (C) 2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELMATH_GEOMUTIL_H_
#define _CELMATH_GEOMUTIL_H_

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <cmath>


template<class T> Eigen::Quaternion<T>
XRotation(T radians)
{
    T halfAngle = radians * T(0.5);
    return Eigen::Quaternion<T>(std::cos(halfAngle), std::sin(halfAngle), 0, 0);
}


template<class T> Eigen::Quaternion<T>
YRotation(T radians)
{
    T halfAngle = radians * T(0.5);
    return Eigen::Quaternion<T>(std::cos(halfAngle), 0, std::sin(halfAngle), 0);
}


template<class T> Eigen::Quaternion<T>
ZRotation(T radians)
{
    T halfAngle = radians * T(0.5);
    return Eigen::Quaternion<T>(std::cos(halfAngle), 0, 0, std::sin(halfAngle));
}


/*! Determine an orientation that will make the negative z-axis point from
 *  from the observer to the target, with the y-axis pointing in direction
 *  of the component of 'up' that is orthogonal to the z-axis.
 */
template<class T> Eigen::Quaternion<T> 
LookAt(Eigen::Matrix<T, 3, 1> from, Eigen::Matrix<T, 3, 1> to, Eigen::Matrix<T, 3, 1> up)
{
    Eigen::Matrix<T, 3, 1> n = to - from;
    n.normalize();
    Eigen::Matrix<T, 3, 1> v = n.cross(up).normalized();
    Eigen::Matrix<T, 3, 1> u = v.cross(n);

    Eigen::Matrix<T, 3, 3> m;
    m.col(0) = v;
    m.col(1) = u;
    m.col(2) = -n;

    return Eigen::Quaternion<T>(m).conjugate();
}


/*! Get a vector orthogonal to the specified one. The input vector
 *  is assumed to be normalized.
 */
template <typename SCALAR> Eigen::Matrix<SCALAR, 3, 1>
OrthogonalUnitVector(const Eigen::Matrix<SCALAR, 3, 1>& v)
{
    Eigen::Matrix<SCALAR, 3, 1> w;

    if (abs(v.x()) < abs(v.y()) && abs(v.x()) < abs(v.z()))
        w = Eigen::Matrix<SCALAR, 3, 1>::UnitX().cross(v);
    else if (abs(v.y()) < abs(v.z()))
        w = Eigen::Matrix<SCALAR, 3, 1>::UnitY().cross(v);
    else
        w = Eigen::Matrix<SCALAR, 3, 1>::UnitZ().cross(v);

    return w.normalized();
}


#endif // _CELMATH_GEOMUTIL_H_

