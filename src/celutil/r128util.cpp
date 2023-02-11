// r128util.cpp
//
// Copyright (C) 2007-present, Celestia Development Team
// Original version by Chris Laurel <claurel@shatters.net>
//
// 128-bit fixed point (64.64) numbers for high-precision celestial
// coordinates. When you need millimeter accurate navigation across a scale
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

#define R128_IMPLEMENTATION
#include "r128util.h"

using namespace std::string_view_literals;

namespace
{

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

namespace celestia::util
{

std::string EncodeAsBase64(const R128 &b)
{
    // Old BigFix class used 8 16-bit words. The bulk of this function
    // is copied from that class, so first we'll convert from two
    // 64-bit words to 8 16-bit words so that the old code can work
    // as-is.
    std::array<std::uint16_t, 8> n =
    {
        static_cast<std::uint16_t>(b.lo),
        static_cast<std::uint16_t>(b.lo >> 16),
        static_cast<std::uint16_t>(b.lo >> 32),
        static_cast<std::uint16_t>(b.lo >> 48),

        static_cast<std::uint16_t>(b.hi),
        static_cast<std::uint16_t>(b.hi >> 16),
        static_cast<std::uint16_t>(b.hi >> 32),
        static_cast<std::uint16_t>(b.hi >> 48),
    };

    // Conversion using code from the original R128 class.
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

R128 DecodeFromBase64(std::string_view val)
{
    static DecoderArray decoder = createBase64Decoder();

    std::array<std::uint16_t, 8> n = {0};

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
    std::uint64_t lo = (static_cast<std::uint64_t>(n[0])
                        | (static_cast<std::uint64_t>(n[1]) << 16)
                        | (static_cast<std::uint64_t>(n[2]) << 32)
                        | (static_cast<std::uint64_t>(n[3]) << 48));
    std::uint64_t hi = (static_cast<std::uint64_t>(n[4])
                        | (static_cast<std::uint64_t>(n[5]) << 16)
                        | (static_cast<std::uint64_t>(n[6]) << 32)
                        | (static_cast<std::uint64_t>(n[7]) << 48));
    return {lo, hi};

}

bool isOutOfBounds(const R128 &b)
{
    constexpr std::uint64_t hi_threshold = UINT64_C(1) << 62;
    constexpr std::uint64_t lo_threshold = static_cast<std::uint64_t>(-static_cast<std::int64_t>(hi_threshold));
    return (b.hi > hi_threshold && b.hi < lo_threshold);
}

} // end namespace celestia::util
