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

namespace celmath
{

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
LookAt(const Eigen::Matrix<T, 3, 1>& from, const Eigen::Matrix<T, 3, 1>& to, const Eigen::Matrix<T, 3, 1>& up)
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

/*! Project to screen space
 */
template<class T> bool
Project(const Eigen::Matrix<T, 3, 1>& from,
        const Eigen::Matrix<T, 4, 4>& modelViewMatrix,
        const Eigen::Matrix<T, 4, 4>& projMatrix,
        const int viewport[4],
        Eigen::Matrix<T, 3, 1>& to)
{
    Eigen::Matrix<T, 4, 1> in(from.x(), from.y(), from.z(), T(1.0));
    Eigen::Matrix<T, 4, 1> out = projMatrix * modelViewMatrix * in;
    if (out.w() == T(0.0))
        return false;

    out = out.array() / out.w();
    // Map x, y and z to range 0-1
    out = T(0.5) + out.array() * T(0.5);
    // Map x,y to viewport
    out.x() = viewport[0] + out.x() * viewport[2];
    out.y() = viewport[1] + out.y() * viewport[3];

    to = { out.x(), out.y(), out.z() };
    return true;
}

/*! Return an perspective projection matrix
 */
template<class T> Eigen::Matrix<T, 4, 4>
Perspective(T fovy, T aspect, T nearZ, T farZ)
{
    if (aspect == T(0.0))
        return Eigen::Matrix<T, 4, 4>::Identity();

    T deltaZ = farZ - nearZ;
    if (deltaZ == T(0.0))
        return Eigen::Matrix<T, 4, 4>::Identity();

    T angle = degToRad(fovy / 2);
    T sine = sin(angle);
    if (sine == T(0.0))
        return Eigen::Matrix<T, 4, 4>::Identity();
    T ctg = cos(angle) / sine;

    Eigen::Matrix<T, 4, 4> m = Eigen::Matrix<T, 4, 4>::Identity();
    m(0, 0) = ctg / aspect;
    m(1, 1) = ctg;
    m(2, 2) = -(farZ + nearZ) / deltaZ;
    m(2, 3) = T(-2.0) * nearZ * farZ / deltaZ;
    m(3, 2) = T(-1.0);
    m(3, 3) = T(0.0);
    return m;
}

/*! Return an orthographic projection matrix
 */
template<class T> Eigen::Matrix<T, 4, 4>
Ortho(T left, T right, T bottom, T top, T nearZ, T farZ)
{
    T rl = right - left;
    T tb = top - bottom;
    T fn = farZ - nearZ;
    Eigen::Matrix<T, 4, 4> m;
    m << 2/rl,    0,     0, - (right + left) / rl,
            0, 2/tb,     0, - (top + bottom) / tb,
            0,    0, -2/fn, - (farZ + nearZ) / fn,
            0,    0,     0,                     1;
    return m;
}

template<class T> Eigen::Matrix<T, 4, 4>
Ortho2D(T left, T right, T bottom, T top)
{
    return Ortho(left, right, bottom, top, T(-1), T(1));
}

}; // namespace celmath
#endif // _CELMATH_GEOMUTIL_H_
