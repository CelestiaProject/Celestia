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

#define R128_IMPLEMENTATION
#include "r128util.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ranges>

using namespace std::string_view_literals;

namespace
{

constexpr std::string_view alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"sv;
static_assert(alphabet.size() == 64, "Base64 decoder alphabet must be of length 64");

constexpr char MinChar = std::ranges::min(alphabet);
constexpr char MaxChar = std::ranges::max(alphabet);

using DecoderArray = std::array<std::int8_t, (MaxChar - MinChar) + 1>;

constexpr DecoderArray
createBase64Decoder()
{
    DecoderArray decoder;
    decoder.fill(-1);
    for (std::size_t i = 0; i < alphabet.size(); ++i)
    {
        decoder[alphabet[i] - MinChar] = static_cast<std::int8_t>(i);
    }

    return decoder;
}

constexpr DecoderArray decoder = createBase64Decoder();

constexpr std::size_t
encodedLength(std::size_t byteCount)
{
    return (byteCount * 4 + 2) / 3;
}

} // end unnamed namespace

namespace celestia::util
{

std::optional<R128>
DecodeFromBase64(std::string_view val)
{
    // remove trailing equals signs
    auto it = std::ranges::find_if_not(val | std::views::reverse,
                                       [](char c) { return c == '='; }).base();
    val = std::string_view(val.begin(), it);
    if (val.empty())
        return R128(0, 0);

    if (val.size() > encodedLength(16) || (val.size() & 0x3) == 1)
        return std::nullopt;

    std::array<std::uint8_t, 16> processed;
    processed.fill(0);
    std::uint32_t input = 0;
    int groupIdx = 0;
    std::size_t processedIdx = 0;
    std::int8_t decoded = 0;
    for (char c : val)
    {
        if (c < MinChar || c > MaxChar)
            return std::nullopt;
        decoded = decoder[c - MinChar];
        // invalid mapping
        if (decoded < 0)
            return std::nullopt;

        input |= decoded;
        ++groupIdx;
        if (groupIdx == 4)
        {
            assert((processedIdx + 2) < processed.size());
            processed[processedIdx    ] = static_cast<std::uint8_t>(input >> 16);
            processed[processedIdx + 1] = static_cast<std::uint8_t>((input >> 8) & 0xff);
            processed[processedIdx + 2] = static_cast<std::uint8_t>(input & 0xff);
            processedIdx += 3;
            input = 0;
            groupIdx = 0;
        }
        else
        {
            input <<= 6;
        }
    }

    if (groupIdx != 0)
    {
        assert(groupIdx != 1 && groupIdx < 4 &&
               (processedIdx + groupIdx - 2) < processed.size());

        input <<= 18 - 6 * groupIdx;
        processed[processedIdx] = static_cast<std::uint8_t>(input >> 16);
        if (groupIdx == 3)
        {
            if (decoded & 0x03) //NOSONAR
            {
                // invalid trailing character
                return std::nullopt;
            }
            processed[processedIdx + 1] = static_cast<std::uint8_t>((input >> 8) & 0xff);
        }
        else if (decoded & 0x0f) //NOSONAR
        {
            // invalid trailing character
            return std::nullopt;
        }
    }

    auto result = std::make_optional<R128>(0, 0);
    auto& r128 = *result;

    for (std::size_t i = 8; i-- > 0;)
    {
        r128.lo = (r128.lo << 8) | processed[i];
        r128.hi = (r128.hi << 8) | processed[i + 8];
    }

    return result;
}

bool isOutOfBounds(const R128 &b)
{
    constexpr std::uint64_t hi_threshold = UINT64_C(1) << 62;
    constexpr std::uint64_t lo_threshold = static_cast<std::uint64_t>(-static_cast<std::int64_t>(hi_threshold));
    return (b.hi > hi_threshold && b.hi < lo_threshold);
}

} // end namespace celestia::util

auto
fmt::formatter<R128>::format(const R128& value, format_context& ctx) const -> decltype(ctx.out())
{
    fmt::basic_memory_buffer<char, encodedLength(16)> encoded;
    const int byteCount = value.hi == 0
        ? 8 - (std::countl_zero(value.lo) >> 3)
        : 16 - (std::countl_zero(value.hi) >> 3);

    std::array data{value.lo, value.hi};
    std::uint32_t output = 0;
    int groupIdx = 0;
    for (int i = 0; i < byteCount; ++i)
    {
        output |= static_cast<std::uint32_t>((data[i >> 3] >> ((i & 7) << 3)) & 0xff);
        ++groupIdx;
        if (groupIdx == 3)
        {
            encoded.push_back(alphabet[output >> 18]);
            encoded.push_back(alphabet[(output >> 12) & 0x3f]);
            encoded.push_back(alphabet[(output >> 6) & 0x3f]);
            encoded.push_back(alphabet[output & 0x3f]);
            output = 0;
            groupIdx = 0;
        }
        else
        {
            output <<= 8;
        }
    }

    if (groupIdx > 0)
    {
        output <<= 16 - (groupIdx << 3);
        encoded.push_back(alphabet[output >> 18]);
        encoded.push_back(alphabet[(output >> 12) & 0x3f]);
        if (groupIdx > 1)
            encoded.push_back(alphabet[(output >> 6) & 0x3f]);
    }

    std::string_view view(encoded.data(), encoded.size());
    return formatter<std::string_view>::format(view, ctx);
}
