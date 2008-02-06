// bigfix.h
//
// Copyright (C) 2007-2008, Chris Laurel <claurel@shatters.net>
//
// 128-bit fixed point (64.64) numbers for high-precision celestial
// coordinates.  When you need millimeter accurate navigation across a scale
// of thousands of light years, double precision floating point numbers
// are inadequate.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _CELUTIL_BIGFIX64_H_
#define _CELUTIL_BIGFIX64_H_

#include <string>
#include "basictypes.h"

/*! 64.64 signed fixed point numbers.
 */

class BigFix
{
 public:
    BigFix();
    BigFix(uint64);
    BigFix(double);
    BigFix(const std::string&);

    operator double() const;
    operator float() const;

    BigFix operator-() const;
    BigFix operator+=(const BigFix&);
    BigFix operator-=(const BigFix&);

    friend BigFix operator+(const BigFix&, const BigFix&);
    friend BigFix operator-(const BigFix&, const BigFix&);
    friend BigFix operator*(const BigFix&, const BigFix&);
    friend BigFix operator*(BigFix, double);
    friend bool operator==(const BigFix&, const BigFix&);
    friend bool operator!=(const BigFix&, const BigFix&);
    friend bool operator<(const BigFix&, const BigFix&);
    friend bool operator>(const BigFix&, const BigFix&);

    int sign() const;

    // for debugging
    void dump();
    std::string toString();

 private:
    bool isNegative() const
    {
        return hi > INT64_MAX;
    }

    static void negate128(uint64& hi, uint64& lo);

 private:
    uint64 hi;
    uint64 lo;
};


// Compute the additive inverse of a 128-bit twos complement value
// represented by two 64-bit values.
inline void BigFix::negate128(uint64& hi, uint64& lo)
{
    // For a twos-complement number, -n = ~n + 1
    hi = ~hi;
    lo = ~lo;
    lo++;
    if (lo == 0)
        hi++;
}

inline BigFix BigFix::operator-() const
{
    BigFix result = *this;

    negate128(result.hi, result.lo);

    return result;
}


inline BigFix BigFix::operator+=(const BigFix& a)
{
    lo += a.lo;
    hi += a.hi;

    // carry
    if (lo < a.lo)
        hi++;

    return *this;
}


inline BigFix BigFix::operator-=(const BigFix& a)
{
    lo -= a.lo;
    hi -= a.hi;

    // borrow
    if (lo > a.lo)
        hi--;

    return *this;
}


inline BigFix operator+(const BigFix& a, const BigFix& b)
{
    BigFix c;

    c.lo = a.lo + b.lo;
    c.hi = a.hi + b.hi;

    // carry
    if (c.lo < a.lo)
        c.hi++;

    return c;
}


inline BigFix operator-(const BigFix& a, const BigFix& b)
{
    BigFix c;

    c.lo = a.lo - b.lo;
    c.hi = a.hi - b.hi;

    // borrow
    if (c.lo > a.lo)
        c.hi--;

    return c;
}

#endif // _CELUTIL_BIGFIX64_H_
