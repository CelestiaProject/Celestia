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
#include <stdlib.h>

#define PI 3.14159265358979323846

template<class T> class Math
{
public:
    static inline void sincos(T, T&, T&);
    static inline T frand();
    static inline T sfrand();
    static inline T lerp(T t, T a, T b);
    static inline T clamp(T t);

private:
    // This class is static and should not be instantiated
    Math() {};
};


typedef Math<float> Mathf;
typedef Math<double> Mathd;


template<class T> T degToRad(T d)
{
    return d / 180 * static_cast<T>(PI);
}

template<class T> T radToDeg(T r)
{
    return r * 180 / static_cast<T>(PI);
}

template<class T> T abs(T x)
{
    return (x < 0) ? -x : x;
}

template<class T> T square(T x)
{
    return x * x;
}

template<class T> T cube(T x)
{
    return x * x * x;
}

template<class T> T clamp(T x)
{
    if (x < 0)
        return 0;
    else if (x > 1)
        return 1;
    else
        return x;
}

template<class T> T sign(T x)
{
    if (x < 0)
        return -1;
    else if (x > 0)
        return 1;
    else
        return 0;
}

// This function is like fmod except that it always returns
// a positive value in the range [ 0, y )
template<class T> T pfmod(T x, T y)
{
    T quotient = std::floor(std::abs(x / y));
    if (x < 0.0)
        return x + (quotient + 1) * y;
    else
        return x - quotient * y;
}

template<class T> T circleArea(T r)
{
    return (T) PI * r * r;
}

template<class T> T sphereArea(T r)
{
    return 4 * (T) PI * r * r;
}

template<class T> void Math<T>::sincos(T angle, T& s, T& c)
{
    s = (T) sin(angle);
    c = (T) cos(angle);
}


// return a random float in [0, 1]
template<class T> T Math<T>::frand()
{
    return (T) (rand() & 0x7fff) / (T) 32767;
}


// return a random float in [-1, 1]
template<class T> T Math<T>::sfrand()
{
    return (T) (rand() & 0x7fff) / (T) 32767 * 2 - 1;
}


template<class T> T Math<T>::lerp(T t, T a, T b)
{
    return a + t * (b - a);
}


// return t clamped to [0, 1]
template<class T> T Math<T>::clamp(T t)
{
    if (t < 0)
        return 0;
    else if (t > 1)
        return 1;
    else
        return t;
}

#endif // _MATHLIB_H_
