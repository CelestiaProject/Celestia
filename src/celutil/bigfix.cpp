// bigfix.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <math.h>
#include <stdio.h>
#include "bigfix.h"


/*** Constructors ***/

// Create a BigFix initialized to zero
BigFix::BigFix()
{
    for (int i = 0; i < N_WORDS; i++)
        n[i] = 0;
}


BigFix::BigFix(int i)
{
    n[N_WORDS / 2] = (unsigned short) (i & 0xffff);
    n[N_WORDS / 2 + 1] = (unsigned short) ((i >> 16) & 0xffff);

    // Sign extend if negative
    if (i < 0)
    {
        for (int j = N_WORDS / 2 + 2; j < N_WORDS; j++)
            n[j] = 0xffff;
    }
}


BigFix::BigFix(double d)
{
    int sign = 1;

    if (d < 0)
    {
        sign = -1;
        d = -d;
    }

    double e = floor(d / (65536.0 * 65536.0 * 65536.0));

    // Check for overflow
    if (e < 32767)
    {
        n[7] = (unsigned short) e;
        d -= n[7] * 65536 * 65536 * 65536;
        n[6] = (unsigned short) (d / (65536.0 * 65536.0));
        d -= n[6] * 65536 * 65536;
        n[5] = (unsigned short) (d / 65536.0);
        d -= n[5] * 65536;
        n[4] = (unsigned short) d;
        d -= n[4];
        n[3] = (unsigned short) (d * 65536.0);
        d -= n[3] / 65536.0;
        n[2] = (unsigned short) (d * 65536.0 * 65536.0);
        d -= n[2] / (65536.0 * 65536.0);
        n[1] = (unsigned short) (d * 65536.0 * 65536.0 * 65536.0);
        d -= n[1] / (65536.0 * 65536.0 * 65536.0);
        n[0] = (unsigned short) (d * 65536.0 * 65536.0 * 65536.0 * 65536.0);
    }

    if (sign < 0)
        *this = -*this;
}


BigFix::operator double() const
{
    double d = 0;
    double e = 1.0 / (65536.0 * 65536.0 * 65536.0 * 65536.0);
    BigFix f;

    if ((n[N_WORDS - 1] & 0x8000) == 0)
        f = *this;
    else
        f = -*this;

    for (int i = 0; i < N_WORDS; i++)
    {
        d += e * f.n[i];
        e *= 65536;
    }

    return ((n[N_WORDS - 1] & 0x8000) == 0) ? d : -d;
}


BigFix::operator float() const
{
    return (float) (double) *this;
}


BigFix BigFix::operator-() const
{
    int i;
    BigFix f;

    for (i = 0; i < N_WORDS; i++)
        f.n[i] = ~n[i];

    unsigned int carry = 1;
    i = 0;
    while (carry != 0 && i < N_WORDS)
    {
        unsigned int m = f.n[i] + carry;
        f.n[i] = (unsigned short) (m & 0xffff);
        carry = m >> 16;
        i++;
    }

    return f;
}


BigFix operator+(BigFix a, BigFix b)
{
    unsigned int carry = 0;
    BigFix c;

    for (int i = 0; i < N_WORDS; i++)
    {
        unsigned int m = a.n[i] + b.n[i] + carry;
        c.n[i] = (unsigned short) (m & 0xffff);
        carry = m >> 16;
    }

    return c;
}


BigFix operator-(BigFix a, BigFix b)
{
    return a + -b;
}


BigFix operator*(BigFix a, BigFix b)
{
    return BigFix();
}


bool operator==(BigFix a, BigFix b)
{
    for (int i = 0; i < N_WORDS; i++)
        if (a.n[i] != b.n[i])
            return false;
    return true;
}


bool operator!=(BigFix a, BigFix b)
{
    return !(a == b);
}


int BigFix::sign()
{
    if ((n[N_WORDS - 1] & 0x8000) != 0)
        return -1;

    for (int i = 0; i < N_WORDS - 1; i++)
        if (n[N_WORDS] != 0)
            return 1;

    return 0;
}


// For debugging
void BigFix::dump()
{
    for (int i = 7; i >= 0; i--)
	printf("%04x ", n[i]);
    printf("\n");
}
