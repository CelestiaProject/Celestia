// geomutil.h
//
// Copyright (C) 2009, the Celestia Development Team
// Original version by Chris Laurel <claurel@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <array>
#include <cmath>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <celcompat/numbers.h>
#include "mathlib.h"

namespace celestia::math
{

template<class T>
const inline Eigen::Quaternion<T> XRot90{ celestia::numbers::sqrt2_v<T> * T{0.5},
                                          celestia::numbers::sqrt2_v<T> * T{0.5},
                                          T{0},
                                          T{0} };

template<class T>
const inline Eigen::Quaternion<T> XRot180{ T{0}, T{1}, T{0}, T{0} };

// Conjugate of 90 degree rotation = -90 degree rotation
template<class T>
const inline Eigen::Quaternion<T> XRot90Conjugate{ celestia::numbers::sqrt2_v<T> * T{0.5},
                                                   -(celestia::numbers::sqrt2_v<T> * T{0.5}),
                                                   T{0},
                                                   T{0} };

template<class T>
const inline Eigen::Quaternion<T> YRot180{ T{0}, T{0}, T{1}, T{0} };

template<class T>
const inline Eigen::Quaternion<T> YRot90Conjugate{ celestia::numbers::sqrt2_v<T> * T{0.5},
                                                   T{0},
                                                   -(celestia::numbers::sqrt2_v<T> * T{0.5}),
                                                   T{0} };

template<class T>
const inline Eigen::Matrix<T, 4, 4> XRot270Matrix = (Eigen::Matrix<T, 4, 4>() <<
                                                     T{ 1}, T{ 0}, T{ 0}, T{ 0},
                                                     T{ 0}, T{ 0}, T{ 1}, T{ 0},
                                                     T{ 0}, T{-1}, T{ 0}, T{ 0},
                                                     T{ 0}, T{ 0}, T{ 0}, T{ 1}).finished();

template<class T>
const inline Eigen::Matrix<T, 4, 4> YRot90Matrix = (Eigen::Matrix<T, 4, 4>() <<
                                                    T{ 0}, T{ 0}, T{ 1}, T{ 0},
                                                    T{ 0}, T{ 1}, T{ 0}, T{ 0},
                                                    T{-1}, T{ 0}, T{ 0}, T{ 0},
                                                    T{ 0}, T{ 0}, T{ 0}, T{ 1}).finished();

template<class T>
const inline Eigen::Matrix<T, 4, 4> YRot180Matrix = (Eigen::Matrix<T, 4, 4>() <<
                                                     T{-1}, T{ 0}, T{ 0}, T{ 0},
                                                     T{ 0}, T{ 1}, T{ 0}, T{ 0},
                                                     T{ 0}, T{ 0}, T{-1}, T{ 0},
                                                     T{ 0}, T{ 0}, T{ 0}, T{ 1}).finished();

template<class T> Eigen::Quaternion<T>
XRotation(T radians)
{
    T s;
    T c;
    sincos(radians * static_cast<T>(0.5), s, c);
    return Eigen::Quaternion<T>(c, s, 0, 0);
}


template<class T> Eigen::Quaternion<T>
YRotation(T radians)
{
    T s;
    T c;
    sincos(radians * static_cast<T>(0.5), s, c);
    return Eigen::Quaternion<T>(c, 0, s, 0);
}


template<class T> Eigen::Quaternion<T>
ZRotation(T radians)
{
    T s;
    T c;
    sincos(radians * static_cast<T>(0.5), s, c);
    return Eigen::Quaternion<T>(c, 0, 0, s);
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
ProjectPerspective(const Eigen::Matrix<T, 3, 1>& from,
                   const Eigen::Matrix<T, 4, 4>& modelViewProjectionMatrix,
                   const std::array<int, 4>& viewport,
                   Eigen::Matrix<T, 3, 1>& to)
{
    Eigen::Matrix<T, 4, 1> in(from.x(), from.y(), from.z(), static_cast<T>(1));
    Eigen::Matrix<T, 4, 1> out = modelViewProjectionMatrix * in;
    if (out.w() == static_cast<T>(0))
        return false;

    out = out.array() / out.w();
    // Map x, y and z to range 0-1
    out = static_cast<T>(0.5) + out.array() * static_cast<T>(0.5);
    // Map x,y to viewport
    out.x() = viewport[0] + out.x() * viewport[2];
    out.y() = viewport[1] + out.y() * viewport[3];

    to = { out.x(), out.y(), out.z() };
    return true;
}

template<class T> bool
ProjectFisheye(const Eigen::Matrix<T, 3, 1>& from,
               const Eigen::Matrix<T, 4, 4>& modelViewMatrix,
               const Eigen::Matrix<T, 4, 4>& projMatrix,
               const std::array<int, 4>& viewport,
               Eigen::Matrix<T, 3, 1>& to)
{
    using std::atan2, std::hypot;
    Eigen::Matrix<T, 4, 1> inPos = modelViewMatrix * Eigen::Matrix<T, 4, 1>(from.x(),
                                                                            from.y(),
                                                                            from.z(),
                                                                            static_cast<T>(1));
    T l = hypot(inPos.x(), inPos.y());
    if (l != static_cast<T>(0))
    {
        T phi = atan2(l, -inPos.z());
        T ratio = phi / static_cast<T>(M_PI_2) / l;
        inPos.x() *= ratio;
        inPos.y() *= ratio;
    }

    Eigen::Matrix<T, 4, 1> out = projMatrix * inPos;
    if (out.w() == static_cast<T>(0))
        return false;

    out = out.array() / out.w();
    // Map x, y and z to range 0-1
    out = static_cast<T>(0.5) + out.array() * static_cast<T>(0.5);
    // Map x,y to viewport
    out.x() = viewport[0] + out.x() * viewport[2];
    out.y() = viewport[1] + out.y() * viewport[3];

    to = { out.x(), out.y(), out.z() };
    return true;
}

/*! Return vertical FOV for perspective projection based on screen dpi, number of vertical
 *  pixels, and screen distance
 */
template<class T>
T PerspectiveFOV(T height, int screenDpi, int distanceToScreen)
{
    return static_cast<T>(2.0) * std::atan(height / (static_cast<T>(screenDpi) / static_cast<T>(25.4)) / static_cast<T>(2.0) / static_cast<T>(distanceToScreen));
}

/*! Return an perspective projection matrix
 */
template<class T> Eigen::Matrix<T, 4, 4>
Perspective(T fovy, T aspect, T nearZ, T farZ)
{
    using std::cos, std::sin;

    if (aspect == static_cast<T>(0))
        return Eigen::Matrix<T, 4, 4>::Identity();

    T deltaZ = farZ - nearZ;
    if (deltaZ == static_cast<T>(0))
        return Eigen::Matrix<T, 4, 4>::Identity();

    T angle = degToRad(fovy / static_cast<T>(2));
    T sine = sin(angle);
    if (sine == static_cast<T>(0))
        return Eigen::Matrix<T, 4, 4>::Identity();
    T ctg = cos(angle) / sine;

    Eigen::Matrix<T, 4, 4> m = Eigen::Matrix<T, 4, 4>::Identity();
    m(0, 0) = ctg / aspect;
    m(1, 1) = ctg;
    m(2, 2) = -(farZ + nearZ) / deltaZ;
    m(2, 3) = static_cast<T>(-2) * nearZ * farZ / deltaZ;
    m(3, 2) = static_cast<T>(-1);
    m(3, 3) = static_cast<T>(0);
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
    m << static_cast<T>(2)/rl,    static_cast<T>(0),     static_cast<T>(0), -(right + left)/rl,
            static_cast<T>(0), static_cast<T>(2)/tb,     static_cast<T>(0), -(top + bottom)/tb,
            static_cast<T>(0),    static_cast<T>(0), static_cast<T>(-2)/fn, -(farZ + nearZ)/fn,
            static_cast<T>(0),    static_cast<T>(0),     static_cast<T>(0),  static_cast<T>(1);
    return m;
}

template<class T> Eigen::Matrix<T, 4, 4>
Ortho2D(T left, T right, T bottom, T top)
{
    return Ortho(left, right, bottom, top, static_cast<T>(-1), static_cast<T>(1));
}

} // namespace celestia::math
