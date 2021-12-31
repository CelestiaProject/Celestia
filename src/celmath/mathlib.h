// mathlib.h
//
// Copyright (C) 2001-2008, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <cmath>

#include <Eigen/Core>

#include <celcompat/numbers.h>

namespace celmath
{
template<typename T> inline void sincos(T angle, T& s, T& c)
{
    using std::cos, std::sin;
    s = sin(angle);
    c = cos(angle);
}

#ifndef HAVE_LERP
template<typename T> constexpr T lerp(T t, T a, T b)
{
    return a + t * (b - a);
}
#endif

template<typename T> inline constexpr T degToRad(T d)
{
    using celestia::numbers::pi_v;
    return d / static_cast<T>(180) * pi_v<T>;
}

template<typename T> inline constexpr T radToDeg(T r)
{
    using celestia::numbers::inv_pi_v;
    return r * static_cast<T>(180) * inv_pi_v<T>;
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
    return x < static_cast<T>(0) ? static_cast<T>(-1)
        : x > static_cast<T>(0) ? static_cast<T>(1)
        : static_cast<T>(0);
}

// This function is like fmod except that it always returns
// a positive value in the range [ 0, y )
template<typename T> T pfmod(T x, T y)
{
    using std::abs, std::floor;

    T quotient = floor(abs(x / y));
    if (x < 0.0)
        return x + (quotient + 1) * y;
    else
        return x - quotient * y;
}

template<typename T> inline constexpr T circleArea(T r)
{
    using celestia::numbers::pi_v;
    return pi_v<T> * r * r;
}

template<typename T> inline constexpr T sphereArea(T r)
{
    using celestia::numbers::pi_v;
    return static_cast<T>(4) * pi_v<T> * r * r;
}

template <typename T> static Eigen::Matrix<T, 3, 1>
ellipsoidTangent(const Eigen::Matrix<T, 3, 1>& recipSemiAxes,
                 const Eigen::Matrix<T, 3, 1>& w,
                 const Eigen::Matrix<T, 3, 1>& e,
                 const Eigen::Matrix<T, 3, 1>& e_,
                 T ee)
{
    using std::sqrt;

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
    T a = static_cast<T>(4) * (square(ew) - ee * ww + ee + static_cast<T>(2) * ew + ww);
    T b = static_cast<T>(-8) * (ee + ew);
    T c = static_cast<T>(4) * ee;

    T t = 0;
    T discriminant = b * b - 4 * a * c;

    if (discriminant < 0)
        discriminant = -discriminant; // Bad!

    t = (-b + sqrt(discriminant)) / (static_cast<T>(2) * a);

    // V is the direction vector.  We now need the point of intersection,
    // which we obtain by solving the quadratic equation for the ray-ellipse
    // intersection.  Since we already know that the discriminant is zero,
    // the solution is just -b/2a
    Eigen::Matrix<T, 3, 1> v = -e * (static_cast<T>(1) - t) + w * t;
    Eigen::Matrix<T, 3, 1> v_ = v.cwiseProduct(recipSemiAxes);
    T a1 = v_.dot(v_);
    T b1 = static_cast<T>(2) * v_.dot(e_);
    T t1 = -b1 / (static_cast<T>(2) * a1);

    return e + v * t1;
}
} // namespace celmath

constexpr long double operator"" _deg (long double deg)
{
    return celmath::degToRad(deg);
}
