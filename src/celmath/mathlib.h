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

// return t clamped to [low, high]
template<typename T> constexpr T clamp(T t, T low, T high)
{
    return (t < low) ? low : ((t > high) ? high : t);
}

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
}; // namespace celmath
#endif // _MATHLIB_H_
