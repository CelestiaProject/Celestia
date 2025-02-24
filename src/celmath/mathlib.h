// mathlib.h
//
// Copyright (C) 2001-2008, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <config.h>

#include <cmath>

#include <Eigen/Core>

#include <celcompat/numbers.h>

namespace celestia::math
{
template<typename T> inline void sincos(T angle, T& s, T& c)
{
    using std::cos, std::sin;
    s = sin(angle);
    c = cos(angle);
}

#ifdef HAVE_SINCOS
template<> inline void sincos(float angle, float& s, float& c)
{
    ::sincosf(angle, &s, &c);
}

template<> inline void sincos(double angle, double& s, double& c)
{
    ::sincos(angle, &s, &c);
}

template<> inline void sincos(long double angle, long double& s, long double& c)
{
    ::sincosl(angle, &s, &c);
}
#elif defined(HAVE_APPLE_SINCOS)
template<> inline void sincos(float angle, float& s, float& c)
{
    __sincosf(angle, &s, &c);
}

template<> inline void sincos(double angle, double& s, double& c)
{
    __sincos(angle, &s, &c);
}
// Apple's version does not have __sincosl
#endif

#if __cplusplus < 202002L
template<typename T> constexpr T lerp(T t, T a, T b)
{
    return a + t * (b - a);
}
#else
using ::std::lerp;
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
    return x < 0.0
        ? x + (quotient + static_cast<T>(1)) * y
        : x - quotient * y;
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

// Find the intersection of a circle and the plane with the specified normal and
// containing the origin. The circle is defined parametrically by:
// center + cos(t)*u + sin(t)*u
// u and v are orthogonal vectors with magnitudes equal to the radius of the
// circle.
// Return true if there are two solutions.
template<typename T> bool planeCircleIntersection(const Eigen::Matrix<T, 3, 1>& planeNormal,
                                                  const Eigen::Matrix<T, 3, 1>& center,
                                                  const Eigen::Matrix<T, 3, 1>& u,
                                                  const Eigen::Matrix<T, 3, 1>& v,
                                                  Eigen::Matrix<T, 3, 1>* sol0,
                                                  Eigen::Matrix<T, 3, 1>* sol1)
{
    // Any point p on the plane must satisfy p*N = 0. Thus the intersection points
    // satisfy (center + cos(t)U + sin(t)V)*N = 0
    // This simplifies to an equation of the form:
    // a*cos(t)+b*sin(t)+c = 0, with a=N*U, b=N*V, and c=N*center
    T a = u.dot(planeNormal);
    T b = v.dot(planeNormal);
    T c = center.dot(planeNormal);

    // The solution is +-acos((-ac +- sqrt(a^2+b^2-c^2))/(a^2+b^2))
    T s = a * a + b * b;
    if (s == 0.0)
    {
        // No solution; plane containing circle is parallel to test plane
        return false;
    }

    if (s - c * c <= 0)
    {
        // One or no solutions; no need to distinguish between these
        // cases for our purposes.
        return false;
    }

    // No need to actually call acos to get the solution, since we're just
    // going to plug it into sin and cos anyhow.
    T r = b * std::sqrt(s - c * c);
    T cosTheta0 = (-a * c + r) / s;
    T cosTheta1 = (-a * c - r) / s;
    T sinTheta0 = std::sqrt(1 - cosTheta0 * cosTheta0);
    T sinTheta1 = std::sqrt(1 - cosTheta1 * cosTheta1);

    *sol0 = center + cosTheta0 * u + sinTheta0 * v;
    *sol1 = center + cosTheta1 * u + sinTheta1 * v;

    // Check that we've chosen a solution that produces a point on the
    // plane. If not, we need to use the -acos solution.
    if (std::abs(sol0->dot(planeNormal)) > 1.0e-8)
    {
        *sol0 = center + cosTheta0 * u - sinTheta0 * v;
    }

    if (std::abs(sol1->dot(planeNormal)) > 1.0e-8)
    {
        *sol1 = center + cosTheta1 * u - sinTheta1 * v;
    }

    return true;
}
} // namespace celestia::math

constexpr long double operator"" _deg (long double deg)
{
    return celestia::math::degToRad(deg);
}
