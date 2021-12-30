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

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <string_view>

#include "bigfix.h"
#include "logger.h"

using namespace std::string_view_literals;

using celestia::util::GetLogger;

namespace
{

constexpr double POW2_31 = 0x1p+31; // 2^31
constexpr double WORD0_FACTOR = 0x1p-64; // 2^-64
constexpr double WORD1_FACTOR = 0x1p-32; // 2^-32
constexpr double WORD2_FACTOR = 1.0;
constexpr double WORD3_FACTOR = 0x1p+32; // 2^32

constexpr std::string_view alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"sv;
static_assert(alphabet.size() == 64, "Base64 decoder alphabet must be of length 64");

constexpr std::uint8_t AsciiRange = 128;

using DecoderArray = std::array<std::int8_t, AsciiRange>;

DecoderArray createBase64Decoder()
{
    DecoderArray decoder;
    decoder.fill(-1);
    for (std::size_t i = 0; i < alphabet.size(); ++i)
    {
        std::uint8_t idx = static_cast<std::uint8_t>(alphabet[i]);
        assert(idx < AsciiRange);
        decoder[idx] = static_cast<std::int8_t>(i);
    }

    return decoder;
}

} // end unnamed namespace

/*** Constructors ***/
// Create a BigFix initialized to zero
BigFix::BigFix()
{
    hi = 0;
    lo = 0;
}

BigFix::BigFix(std::uint64_t i)
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
    double e = std::floor(d * (1.0 / WORD3_FACTOR));
    if (e < POW2_31)
    {
        auto w3 = static_cast<std::uint32_t>(e);
        d -= w3 * WORD3_FACTOR;
        auto w2 = static_cast<std::uint32_t>(d * (1.0 / WORD2_FACTOR));
        d -= w2 * WORD2_FACTOR;
        auto w1 = static_cast<std::uint32_t>(d * (1.0 / WORD1_FACTOR));
        d -= w1 * WORD1_FACTOR;
        auto w0 = static_cast<std::uint32_t>(d * (1.0 / WORD0_FACTOR));

        hi = (static_cast<std::uint64_t>(w3) << 32) | w2;
        lo = (static_cast<std::uint64_t>(w1) << 32) | w0;

      if (isNegative)
          negate128(hi, lo);
    }
    else
    {
      // Not a good idea but at least they are initialized
      // if too big (>= 2**63) value is passed
      GetLogger()->error("Too big value {} passed to BigFix::BigFix()\n", d);
      hi = lo = 0;
    }
}

BigFix::operator double() const
{
    // Handle negative values by inverting them before conversion,
    // then inverting the converted value.
    int sign = 1;
    std::uint64_t l = lo;
    std::uint64_t h = hi;

    if (isNegative())
    {
        negate128(h, l);
        sign = -1;
    }

    // Need to break the number into 32-bit chunks because a 64-bit
    // integer has more bits of precision than a double.
    std::uint32_t w0 = static_cast<std::uint32_t>(l);
    std::uint32_t w1 = static_cast<std::uint32_t>(l >> 32);
    std::uint32_t w2 = static_cast<std::uint32_t>(h);
    std::uint32_t w3 = static_cast<std::uint32_t>(h >> 32);
    double d;

    d = (w0 * WORD0_FACTOR +
         w1 * WORD1_FACTOR +
         w2 * WORD2_FACTOR +
         w3 * WORD3_FACTOR) * sign;

    return d;
}

BigFix::operator float() const
{
    return static_cast<float>(static_cast<double>(*this));
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
    std::uint32_t w0 = static_cast<std::uint32_t>(f.lo);
    std::uint32_t w1 = static_cast<std::uint32_t>(f.lo >> 32);
    std::uint32_t w2 = static_cast<std::uint32_t>(f.hi);
    std::uint32_t w3 = static_cast<std::uint32_t>(f.hi >> 32);

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

    std::uint64_t ah = a.hi;
    std::uint64_t al = a.lo;
    if (a.isNegative())
        BigFix::negate128(ah, al);

    std::uint64_t bh = b.hi;
    std::uint64_t bl = b.lo;
    if (b.isNegative())
        BigFix::negate128(bh, bl);

    // Break the values down into 32-bit words so that the partial products
    // will fit into 64-bit words.
    std::uint64_t aw[4];
    aw[0] = al & UINT64_C(0xffffffff);
    aw[1] = al >> 32;
    aw[2] = ah & UINT64_C(0xffffffff);
    aw[3] = ah >> 32;

    std::uint64_t bw[4];
    bw[0] = bl & UINT64_C(0xffffffff);
    bw[1] = bl >> 32;
    bw[2] = bh & UINT64_C(0xffffffff);
    bw[3] = bh >> 32;

    // Set the high and low non-zero words; this will
    // speed up multiplicatoin of integers and values
    // less than one.
    std::uint32_t hiworda = ah == 0 ? 1 : 3;
    std::uint32_t loworda = al == 0 ? 2 : 0;
    std::uint32_t hiwordb = bh == 0 ? 1 : 3;
    std::uint32_t lowordb = bl == 0 ? 2 : 0;

    std::uint32_t result[8] = {0};

    for (std::uint32_t i = lowordb; i <= hiwordb; i++)
    {
        std::uint32_t carry = 0;

        unsigned int j;
        for (j = loworda; j <= hiworda; j++)
        {
            std::uint64_t partialProd = aw[j] * bw[i];

            // This sum will never overflow. Let N = 2^32;
            // the max value of the partial product is (N-1)^2.
            // The max values of result[i + j] and carry are
            // N-1. Thus the max value of the sum is
            // (N-1)^2 + 2(N-1) = (N^2 - 2N + 1) + 2(N-1) = N^2-1
            std::uint64_t q = static_cast<std::uint64_t>(result[i + j])
                            + partialProd
                            + static_cast<std::uint64_t>(carry);
            carry = static_cast<std::uint32_t>(q >> 32);
            result[i + j] = static_cast<std::uint32_t>(q);
        }

        result[i + j] = carry;
    }

    // TODO: check for overflow
    // (as simple as result[0] != 0 || result[1] != 0 || highbit(result[2]))
    BigFix c;
    c.lo = static_cast<std::uint64_t>(result[2]) + (static_cast<std::uint64_t>(result[3]) << 32);
    c.hi = static_cast<std::uint64_t>(result[4]) + (static_cast<std::uint64_t>(result[5]) << 32);

    bool resultNegative = a.isNegative() != b.isNegative();
    return resultNegative ?  -c : c;
}

int BigFix::sign() const
{
    if (hi == 0 && lo == 0)
        return 0;
    return isNegative() ? -1 : 1;
}

BigFix BigFix::fromBase64(std::string_view val)
{
    static DecoderArray decoder = createBase64Decoder();

    std::uint16_t n[8] = {0};

    // Code from original BigFix class to convert base64 string into
    // array of 8 16-bit values.
    int char_count = 0;
    int bits = 0;

    int i = 0;

    for (unsigned char c : val)
    {
        if (c == '=')
            break;
        if (c >= AsciiRange)
            continue;
        std::int8_t decoded = decoder[c];
        if (decoded < 0)
            continue;
        bits += decoded;
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
    BigFix result;
    result.lo = (static_cast<std::uint64_t>(n[0])
                 | (static_cast<std::uint64_t>(n[1]) << 16)
                 | (static_cast<std::uint64_t>(n[2]) << 32)
                 | (static_cast<std::uint64_t>(n[3]) << 48));
    result.hi = (static_cast<std::uint64_t>(n[4])
                 | (static_cast<std::uint64_t>(n[5]) << 16)
                 | (static_cast<std::uint64_t>(n[6]) << 32)
                 | (static_cast<std::uint64_t>(n[7]) << 48));
    return result;
}

std::string BigFix::toBase64() const
{
    // Old BigFix class used 8 16-bit words. The bulk of this function
    // is copied from that class, so first we'll convert from two
    // 64-bit words to 8 16-bit words so that the old code can work
    // as-is.
    std::uint16_t n[8] {
        static_cast<std::uint16_t>(lo),
        static_cast<std::uint16_t>(lo >> 16),
        static_cast<std::uint16_t>(lo >> 32),
        static_cast<std::uint16_t>(lo >> 48),

        static_cast<std::uint16_t>(hi),
        static_cast<std::uint16_t>(hi >> 16),
        static_cast<std::uint16_t>(hi >> 32),
        static_cast<std::uint16_t>(hi >> 48),
    };

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
