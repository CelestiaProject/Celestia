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
        d -= n[7] * 65536.0 * 65536.0 * 65536.0;
        n[6] = (unsigned short) (d / (65536.0 * 65536.0));
        d -= n[6] * 65536.0 * 65536.0;
        n[5] = (unsigned short) (d / 65536.0);
        d -= n[5] * 65536.0;
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


BigFix operator*(BigFix, BigFix)
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



static unsigned char alphabet[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

BigFix::BigFix(const std::string& val)
{
    static char inalphabet[256], decoder[256];
    int i, bits, c, char_count;
    /*int errors = 0;   Unused*/;

    for (i = (sizeof alphabet) - 1; i >= 0 ; i--)
    {
        inalphabet[alphabet[i]] = 1;
        decoder[alphabet[i]] = i;
    }

    for (i = 0; i < 8 ; i++)
        n[i] = 0;

    char_count = 0;
    bits = 0;

    i = 0;

    for (int j = 0; j < (int)val.length(); j++)
    {
        c = val[j];
        if (c == '=')
            break;
        if (c > 255 || !inalphabet[c])
            continue;
        bits += decoder[c];
        char_count++;
        if (char_count == 4)
        {
            n[i/2] >>= 8;
            n[i/2] += (bits >> 8) & 0xff00;
            i++;
            n[i/2] >>= 8;
            n[i/2] += bits & 0xff00;
            i++;
            n[i/2] >>= 8;
            n[i/2] += (bits << 8) & 0xff00;
            i++;
            bits = 0;
            char_count = 0;
        }
        else
        {
            bits <<= 6;
        }
    }

    switch (char_count)
    {
    case 2:
        n[i/2] >>= 8;
        n[i/2] += (bits >> 2) & 0xff00;
        i++;
        break;
    case 3:
        n[i/2] >>= 8;
        n[i/2] += (bits >> 8) & 0xff00;
        i++;
        n[i/2] >>= 8;
        n[i/2] += bits & 0xff00;
        i++;
        break;
    }

    if (i & 1)
        n[i/2] >>= 8;
}


std::string BigFix::toString()
{

    std::string encoded("");
    int bits, c, char_count, started, i, j;

    char_count = 0;
    bits = 0;
    started = 0;

    // Find first significant (non null) byte
    i = 16;
    do {
        i--;
        c = n[i/2];
        if (i & 1) c >>= 8;
        c &= 0xff;
    } while ((c == 0) && (i != 0));

    if (i == 0)
        return encoded;

    // Then we encode starting by the LSB (i+1 bytes to encode)
    j = 0;
    while (j <= i)
    {
        c = n[j/2];
        if ( j & 1 ) c >>= 8;
        c &= 0xff;
        j++;
        bits += c;
        char_count++;
        if (char_count == 3)
        {
            encoded += alphabet[bits >> 18];
            encoded += alphabet[(bits >> 12) & 0x3f];
            encoded += alphabet[(bits >> 6) & 0x3f];
            encoded += alphabet[bits & 0x3f];
            bits = 0;
            char_count = 0;
        }
        else
        {
            bits <<= 8;
        }
    }

    if (char_count != 0)
    {
        bits <<= 16 - (8 * char_count);
        encoded += alphabet[bits >> 18];
        encoded += alphabet[(bits >> 12) & 0x3f];
        if (char_count != 1)
            encoded += alphabet[(bits >> 6) & 0x3f];
    }

    return encoded;
}


