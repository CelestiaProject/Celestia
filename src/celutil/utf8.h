// utf8.h
//
// Copyright (C) 2004, Chris Laurel <claurel@shatters.net>
//               2018-present, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#define UTF8_DEGREE_SIGN         "\302\260"
#define UTF8_MULTIPLICATION_SIGN "\303\227"
#define UTF8_REPLACEMENT_CHAR    "\357\277\275"

bool UTF8Decode(std::string_view str, std::int32_t &ch);
bool UTF8Decode(std::string_view str, std::int32_t &pos, std::int32_t &ch);
void UTF8Encode(std::uint32_t ch, std::string &dest);
int  UTF8StringCompare(std::string_view s0, std::string_view s1);
bool UTF8StartsWith(std::string_view str, std::string_view prefix, bool ignoreCase = false);

class UTF8StringOrderingPredicate
{
 public:
    // enable use for lookup of strings by string_view
    using is_transparent = void;

    bool operator()(std::string_view s0, std::string_view s1) const
    {
        return UTF8StringCompare(s0, s1) == -1;
    }
};


class UTF8Validator
{
public:
    std::int32_t check(unsigned char c);
    inline std::int32_t check(char c) { return check(static_cast<unsigned char>(c)); }
    inline bool isInitial() const { return state == State::Initial; }

    static constexpr std::int32_t PartialSequence = -1;
    static constexpr std::int32_t InvalidStarter = -2;
    static constexpr std::int32_t InvalidTrailing = -3;

private:
    enum class State
    {
        Initial,
        Continuation1,
        Continuation2,
        Continuation3,
    };

    State state{ State::Initial };
    std::array<unsigned char, 3> buffer{ };
};
