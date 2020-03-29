// mathlib.h
//
// Copyright (C) 2001-2008, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELMATH_MATHLIB_H_
#define _CELMATH_MATHLIB_H_

#include <cmath>
#include <cstdlib>
#include <Eigen/Core>

#define PI 3.14159265358979323846

namespace celmath
{
template<typename T> inline void sincos(T angle, T& s, T& c)
{
    s = sin(angle);
    c = cos(angle);
}

// return a random float in [0, 1]
template<typename T> inline T frand()
{
    return (T) (rand() & 0x7fff) / (T) 32767;
}

// return a random float in [-1, 1]
template<typename T> inline T sfrand()
{
    return (T) (rand() & 0x7fff) / (T) 32767 * 2 - 1;
}

#ifndef HAVE_LERP
template<typename T> constexpr T lerp(T t, T a, T b)
{
    return a + t * (b - a);
}
#endif

// return t clamped to [0, 1]
template<typename T> constexpr T clamp(T t)
{
    return (t < 0) ? 0 : ((t > 1) ? 1 : t);
}

#if __cplusplus < 201703L
// return t clamped to [low, high]
template<typename T> constexpr T clamp(T t, T low, T high)
{
    return (t < low) ? low : ((t > high) ? high : t);
}
#endif

template<typename T> inline constexpr T degToRad(T d)
{
    return d / 180 * static_cast<T>(PI);
}

template<typename T> inline constexpr T radToDeg(T r)
{
    return r * 180 / static_cast<T>(PI);
}

template<typename T> inline constexpr T square(T x)
{
    return x * x;
}

template<typename T> inline constexpr T cube(T x)
{
    return x * x * x;
}

template<typename T> inline constexpr T sign(T x)
{
    return (x < 0) ? -1 : ((x > 0) ? 1 : 0);
}

// This function is like fmod except that it always returns
// a positive value in the range [ 0, y )
template<typename T> T pfmod(T x, T y)
{
    T quotient = floor(abs(x / y));
    if (x < 0.0)
        return x + (quotient + 1) * y;
    else
        return x - quotient * y;
}

template<typename T> inline constexpr T circleArea(T r)
{
    return static_cast<T>(PI) * r * r;
}

template<typename T> inline constexpr T sphereArea(T r)
{
    return 4 * static_cast<T>(PI) * r * r;
}

template <typename T> static Eigen::Matrix<T, 3, 1>
ellipsoidTangent(const Eigen::Matrix<T, 3, 1>& recipSemiAxes,
                 const Eigen::Matrix<T, 3, 1>& w,
                 const Eigen::Matrix<T, 3, 1>& e,
                 const Eigen::Matrix<T, 3, 1>& e_,
                 T ee)
{
    // We want to find t such that -E(1-t) + Wt is the direction of a ray
    // tangent to the ellipsoid.  A tangent ray will intersect the ellipsoid
    // at exactly one point.  Finding the intersection between a ray and an
    // ellipsoid ultimately requires using the quadratic formula, which has
    // one solution when the discriminant (b^2 - 4ac) is zero.  The code below
    // computes the value of t that results in a discriminant of zero.
    Eigen::Matrix<T, 3, 1> w_ = w.cwiseProduct(recipSemiAxes);
    T ww = w_.dot(w_);
    T ew = w_.dot(e_);

    // Before elimination of terms:
    // double a =  4 * square(ee + ew) - 4 * (ee + 2 * ew + ww) * (ee - 1.0f);
    // double b = -8 * ee * (ee + ew)  - 4 * (-2 * (ee + ew) * (ee - 1.0f));
    // double c =  4 * ee * ee         - 4 * (ee * (ee - 1.0f));

    // Simplify the below expression and eliminate the ee^2 terms; this
    // prevents precision errors, as ee tends to be a very large value.
    //T a =  4 * square(ee + ew) - 4 * (ee + 2 * ew + ww) * (ee - 1);
    T a =  4 * (square(ew) - ee * ww + ee + 2 * ew + ww);
    T b = -8 * (ee + ew);
    T c =  4 * ee;

    T t = 0;
    T discriminant = b * b - 4 * a * c;

    if (discriminant < 0)
        discriminant = -discriminant; // Bad!

    t = (-b + (T) sqrt(discriminant)) / (2 * a);

    // V is the direction vector.  We now need the point of intersection,
    // which we obtain by solving the quadratic equation for the ray-ellipse
    // intersection.  Since we already know that the discriminant is zero,
    // the solution is just -b/2a
    Eigen::Matrix<T, 3, 1> v = -e * (1 - t) + w * t;
    Eigen::Matrix<T, 3, 1> v_ = v.cwiseProduct(recipSemiAxes);
    T a1 = v_.dot(v_);
    T b1 = (T) 2 * v_.dot(e_);
    T t1 = -b1 / (2 * a1);

    return e + v * t1;
}
} // namespace celmath

#if __cplusplus < 201703L
namespace std
{
    using celmath::clamp;
}
#endif

constexpr long double operator"" _deg (long double deg)
{
    return celmath::degToRad(deg);
}

#endif // _MATHLIB_H_
