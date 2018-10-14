// bigfix.cpp
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

#include <cmath>
#include <fmt/printf.h>
#include <iostream>
#include "bigfix.h"


static const double POW2_31 = 2147483648.0;
static const double POW2_32 = 4294967296.0;
static const double POW2_64 = POW2_32 * POW2_32;

static const double WORD0_FACTOR = 1.0 / POW2_64;
static const double WORD1_FACTOR = 1.0 / POW2_32;
static const double WORD2_FACTOR = 1.0;
static const double WORD3_FACTOR = POW2_32;


/*** Constructors ***/
// Create a BigFix initialized to zero
BigFix::BigFix()
{
    hi = 0;
    lo = 0;
}


BigFix::BigFix(uint64_t i)
{
    hi = i;
    lo = 0;
}


BigFix::BigFix(double d)
{
    bool isNegative = false;

    // Handle negative values by inverting them before conversion,
    // then inverting the converted value.
    if (d < 0)
    {
        isNegative = true;
        d = -d;
    }

    // Need to break the number into 32-bit chunks because a 64-bit
    // integer has more bits of precision than a double.
    double e = floor(d * (1.0 / WORD3_FACTOR));
    if (e < POW2_31)
    {
        auto w3 = (uint32_t) e;
        d -= w3 * WORD3_FACTOR;
        auto w2 = (uint32_t) (d * (1.0 / WORD2_FACTOR));
        d -= w2 * WORD2_FACTOR;
        auto w1 = (uint32_t) (d * (1.0 / WORD1_FACTOR));
        d -= w1 * WORD1_FACTOR;
        auto w0 = (uint32_t) (d * (1.0 / WORD0_FACTOR));

        hi = ((uint64_t) w3 << 32) | w2;
        lo = ((uint64_t) w1 << 32) | w0;

      if (isNegative)
          negate128(hi, lo);
    }
    else
    {
      // Not a good idea but at least they are initialized
      // if too big (>= 2**63) value is passed
      std::cerr << "Too big value " << d <<" passed to BigFix::BigFix()\n";
      hi = lo = 0;
    }
}


BigFix::operator double() const
{
    // Handle negative values by inverting them before conversion,
    // then inverting the converted value.
    int sign = 1;
    uint64_t l = lo;
    uint64_t h = hi;

    if (isNegative())
    {
        negate128(h, l);
        sign = -1;
    }

    // Need to break the number into 32-bit chunks because a 64-bit
    // integer has more bits of precision than a double.
    uint32_t w0 = l & 0xffffffff;
    uint32_t w1 = l >> 32;
    uint32_t w2 = h & 0xffffffff;
    uint32_t w3 = h >> 32;
    double d;

    d = (w0 * WORD0_FACTOR +
         w1 * WORD1_FACTOR +
         w2 * WORD2_FACTOR +
         w3 * WORD3_FACTOR) * sign;

    return d;
}


BigFix::operator float() const
{
    return (float) (double) *this;
}


bool operator==(const BigFix& a, const BigFix& b)
{
    return a.hi == b.hi && a.lo == b.lo;
}


bool operator!=(const BigFix& a, const BigFix& b)
{
    return a.hi != b.hi || a.lo != b.lo;
}


bool operator<(const BigFix& a, const BigFix& b)
{
    if (a.isNegative() == b.isNegative())
    {
        return a.hi == b.hi ? a.lo < b.lo : a.hi < b.hi;
    }
    return a.isNegative();
}


bool operator>(const BigFix& a, const BigFix& b)
{
    return b < a;
}


// TODO: probably faster to do this by converting the double to fixed
// point and using the fix*fix multiplication.
BigFix operator*(BigFix f, double d)
{
    // Need to break the number into 32-bit chunks because a 64-bit
    // integer has more bits of precision than a double.
    uint32_t w0 = f.lo & 0xffffffff;
    uint32_t w1 = f.lo >> 32;
    uint32_t w2 = f.hi & 0xffffffff;
    uint32_t w3 = f.hi >> 32;

    return BigFix(w0 * d * WORD0_FACTOR) +
           BigFix(w1 * d * WORD1_FACTOR) +
           BigFix(w2 * d * WORD2_FACTOR) +
           BigFix(w3 * d * WORD3_FACTOR);
}


/*! Multiply two BigFix values together. This function does not check for
 *  overflow. This is not a problem in Celestia, where it is used exclusively
 *  in multiplications where one multiplicand has absolute value <= 1.0.
 */
BigFix operator*(const BigFix& a, const BigFix& b)
{
    // Multiply two fixed point values together using partial products.

    uint64_t ah = a.hi;
    uint64_t al = a.lo;
    if (a.isNegative())
        BigFix::negate128(ah, al);

    uint64_t bh = b.hi;
    uint64_t bl = b.lo;
    if (b.isNegative())
        BigFix::negate128(bh, bl);

    // Break the values down into 32-bit words so that the partial products
    // will fit into 64-bit words.
    uint64_t aw[4];
    aw[0] = al & 0xffffffff;
    aw[1] = al >> 32;
    aw[2] = ah & 0xffffffff;
    aw[3] = ah >> 32;

    uint64_t bw[4];
    bw[0] = bl & 0xffffffff;
    bw[1] = bl >> 32;
    bw[2] = bh & 0xffffffff;
    bw[3] = bh >> 32;

    // Set the high and low non-zero words; this will
    // speed up multiplicatoin of integers and values
    // less than one.
    uint32_t hiworda = ah == 0 ? 1 : 3;
    uint32_t loworda = al == 0 ? 2 : 0;
    uint32_t hiwordb = bh == 0 ? 1 : 3;
    uint32_t lowordb = bl == 0 ? 2 : 0;

    uint32_t result[8] = {0};

    for (uint32_t i = lowordb; i <= hiwordb; i++)
    {
        uint32_t carry = 0;

        unsigned int j;
        for (j = loworda; j <= hiworda; j++)
        {
            uint64_t partialProd = aw[j] * bw[i];

            // This sum will never overflow. Let N = 2^32;
            // the max value of the partial product is (N-1)^2.
            // The max values of result[i + j] and carry are
            // N-1. Thus the max value of the sum is
            // (N-1)^2 + 2(N-1) = (N^2 - 2N + 1) + 2(N-1) = N^2-1
            uint64_t q = (uint64_t) result[i + j] + partialProd + (uint64_t) carry;
            carry = (uint32_t) (q >> 32);
            result[i + j] = (uint32_t) (q & 0xffffffff);
        }

        result[i + j] = carry;
    }

    // TODO: check for overflow
    // (as simple as result[0] != 0 || result[1] != 0 || highbit(result[2]))
    BigFix c;
    c.lo = (uint64_t) result[2] + ((uint64_t) result[3] << 32);
    c.hi = (uint64_t) result[4] + ((uint64_t) result[5] << 32);

    bool resultNegative = a.isNegative() != b.isNegative();
    return resultNegative ?  -c : c;
}


int BigFix::sign() const
{

    if (hi == 0 && lo == 0)
        return 0;
    return isNegative() ? -1 : 1;
}


// For debugging
void BigFix::dump()
{
    fmt::printf("%08x %08x %08x %08x",
                (uint32_t) (hi >> 32),
                (uint32_t) (hi & 0xffffffff),
                (uint32_t) (lo >> 32),
                (uint32_t) (lo & 0xffffffff));
}


#if 0
int main(int argc, char* argv[])
{
    if (argc != 3)
        return 1;

    double a = 0.0;
    if (sscanf(argv[1], "%lf", &a) != 1)
        return 1;

    double b = 0.0;
    if (sscanf(argv[2], "%lf", &b) != 1)
        return 1;

    fmt::printf("    sum:\n%f\n%f\n", a + b, (double) (BigFix(a) + BigFix(b)));
    fmt::printf("   diff:\n%f\n%f\n", a - b, (double) (BigFix(a) - BigFix(b)));
    fmt::printf("product:\n%f\n%f\n", a * b, (double) (BigFix(a) * BigFix(b)));
    fmt::printf("     lt: %u %u\n", a < b, BigFix(a) < BigFix(b));

    return 0;
}
#endif

static unsigned char alphabet[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

BigFix::BigFix(const std::string& val)
{
    static char inalphabet[256] = {0}, decoder[256] = {0};
    int i, bits, char_count;

    for (i = (sizeof alphabet) - 1; i >= 0 ; i--)
    {
        inalphabet[alphabet[i]] = 1;
        decoder[alphabet[i]] = i;
    }

    uint16_t n[8] = {0};

    // Code from original BigFix class to convert base64 string into
    // array of 8 16-bit values.
    char_count = 0;
    bits = 0;

    i = 0;

    for (unsigned char c : val)
    {
        if (c == '=')
            break;
        if (!inalphabet[c])
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

    if ((i & 1) != 0)
        n[i/2] >>= 8;

    // Now, convert the 8 16-bit values to a 2 64-bit values
    lo = ((uint64_t) n[0] |
          ((uint64_t) n[1] << 16) |
          ((uint64_t) n[2] << 32) |
          ((uint64_t) n[3] << 48));
    hi = ((uint64_t) n[4] |
          ((uint64_t) n[5] << 16) |
          ((uint64_t) n[6] << 32) |
          ((uint64_t) n[7] << 48));
}


std::string BigFix::toString()
{
    // Old BigFix class used 8 16-bit words. The bulk of this function
    // is copied from that class, so first we'll convert from two
    // 64-bit words to 8 16-bit words so that the old code can work
    // as-is.
    uint16_t n[8];

    n[0] = lo & 0xffff;
    n[1] = (lo >> 16) & 0xffff;
    n[2] = (lo >> 32) & 0xffff;
    n[3] = (lo >> 48) & 0xffff;

    n[4] = hi & 0xffff;
    n[5] = (hi >> 16) & 0xffff;
    n[6] = (hi >> 32) & 0xffff;
    n[7] = (hi >> 48) & 0xffff;

    // Conversion using code from the original BigFix class.
    std::string encoded;
    int bits, c, char_count, i;

    char_count = 0;
    bits = 0;

    // Find first significant (non null) byte
    i = 16;
    do {
        i--;
        c = n[i/2];
        if ((i & 1) != 0) c >>= 8;
        c &= 0xff;
    } while ((c == 0) && (i != 0));

    if (i == 0)
        return encoded;

    // Then we encode starting by the LSB (i+1 bytes to encode)
    for (auto j = 0; j <= i; j++)
    {
        c = n[j/2];
        if ( (j & 1) != 0 ) c >>= 8;
        c &= 0xff;
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
