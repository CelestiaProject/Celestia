// r128_test.cpp
//
// Copyright (C) 2026-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <random>
#include <string>
#include <string_view>

#include <doctest.h>
#include <fmt/format.h>

#include <celutil/r128util.h>

using namespace std::string_view_literals;

namespace
{

constexpr auto MaxComponent = std::numeric_limits<R128_U64>::max();
constexpr auto MaxBase64 = "/////////////////////w"sv;

std::mt19937
getRng()
{
    std::random_device rd;
    std::uniform_int_distribution<std::uint_least32_t> dist{0, UINT32_C(0xffffffff)};
    constexpr std::size_t seedSize = std::mt19937::state_size * (std::mt19937::word_size / 32);
    std::array<std::uint_least32_t, seedSize> seedData;
    std::ranges::generate(seedData, [&rd, &dist] { return dist(rd); });
    std::seed_seq rngSeed(seedData.begin(), seedData.end());
    return std::mt19937{rngSeed};
}

} // end unnamed namespace

TEST_SUITE_BEGIN("R128");

TEST_CASE("Serialize zero")
{
    REQUIRE(fmt::format("{}", R128(0, 0)).empty());
}

TEST_CASE("Serialize maximum")
{
    REQUIRE(fmt::format("{}", R128(MaxComponent, MaxComponent)) == MaxBase64);
}

TEST_CASE("Deserialize empty")
{
    auto value = celestia::util::DecodeFromBase64(""sv);
    REQUIRE(value.has_value());
    REQUIRE(value->lo == 0);
    REQUIRE(value->hi == 0);
}

TEST_CASE("Deserialize maximum")
{
    auto value = celestia::util::DecodeFromBase64(MaxBase64);
    REQUIRE(value.has_value());
    REQUIRE(value->lo == MaxComponent);
    REQUIRE(value->hi == MaxComponent);
}

TEST_CASE("Ignore trailing equals")
{
    std::string value = "test/+01";
    auto expected = celestia::util::DecodeFromBase64(value);
    REQUIRE(expected.has_value());
    for (int i = 1; i < 10; ++i)
    {
        value.push_back('=');
        auto actual = celestia::util::DecodeFromBase64(value);
        REQUIRE(actual.has_value());
        REQUIRE(*actual == *expected);
    }
}

TEST_CASE("Invalid character")
{
    auto actual = celestia::util::DecodeFromBase64("foo$bar");
    REQUIRE(!actual.has_value());
}

TEST_CASE("Reject overlong")
{
    auto actual = celestia::util::DecodeFromBase64("antidisestablishmentarianism");
    REQUIRE(!actual.has_value());
}

TEST_CASE("Check final character")
{
    constexpr auto alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"sv;

    for (std::size_t length = 1; length < MaxBase64.size(); ++length)
    {
        std::string encoded(length, '/');
        for (std::size_t i = 0; i < alphabet.size(); ++i)
        {
            encoded.back() = alphabet[i];
            bool expected;
            switch (length % 4)
            {
            case 0: expected = true; break;
            case 1: expected = false; break;
            case 2: expected = (i & 0xf) == 0; break;
            case 3: expected = (i & 0x3) == 0; break;
            default: REQUIRE(false); continue;
            }

            bool actual = celestia::util::DecodeFromBase64(encoded).has_value();
            REQUIRE(actual == expected);
        }
    }
}

TEST_CASE("Round trip to base64")
{
    auto rng = getRng();
    std::uniform_int_distribution<R128_U64> dist;
    R128_U64 loMask = 0xff;
    R128_U64 hiMask = 0;
    std::string encoded;
    for (int nBytes = 2; nBytes <= 16; ++nBytes)
    {
        hiMask = (hiMask << 8) | (loMask >> 56);
        loMask = (loMask << 8) | 0xff;
        for (int i = 0; i < 10000; ++i)
        {
            R128 value(dist(rng) & loMask, dist(rng) & hiMask);

            encoded.clear();
            fmt::format_to(std::back_inserter(encoded), "{}", value);
            auto actual = celestia::util::DecodeFromBase64(encoded);
            REQUIRE(actual.has_value());
            REQUIRE(*actual == value);
        }
    }
}

TEST_SUITE_END();
