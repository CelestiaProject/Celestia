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

#include <cstdint>
#include <string>
#include <string_view>

#define UTF8_DEGREE_SIGN         "\302\260"
#define UTF8_MULTIPLICATION_SIGN "\303\227"
#define UTF8_REPLACEMENT_CHAR    "\357\277\275"

bool UTF8Decode(std::string_view str, int pos, wchar_t &ch);
void UTF8Encode(std::uint32_t ch, std::string &dest);
int  UTF8StringCompare(std::string_view s0, std::string_view s1);
int  UTF8StringCompare(std::string_view s0, std::string_view s1, size_t n, bool ignoreCase = false);

class UTF8StringOrderingPredicate
{
 public:
    bool operator()(std::string_view s0, std::string_view s1) const
    {
        return UTF8StringCompare(s0, s1) == -1;
    }
};

int UTF8Length(std::string_view s);

constexpr int
UTF8EncodedSize(wchar_t ch)
{
    if (ch < 0x80)
        return 1;
    if (ch < 0x800)
        return 2;
#if WCHAR_MAX > 0xFFFFu
    if (ch < 0x10000)
#endif
        return 3;
#if WCHAR_MAX > 0xFFFFu
    if (ch < 0x200000)
        return 4;
    if (ch < 0x4000000)
        return 5;
    else
        return 6;
#endif
}

constexpr int
UTF8EncodedSizeChecked(std::uint32_t ch)
{
    if (ch < 0x80)
        return 1;
    if (ch < 0x800)
        return 2;
#if WCHAR_MAX > 0xFFFFu
    if (ch < 0x10000)
#endif
        return 3;
#if WCHAR_MAX > 0xFFFFu
    if (ch < 0x110000)
        return 4;
    // out-of-range: assume U+FFFD REPLACEMENT CHARACTER
    return 3;
#endif
}

enum class UTF8Status
{
    Ok,
    InvalidFirstByte,
    InvalidTrailingByte,
};

class UTF8Validator
{
public:
    UTF8Status check(char c);
    UTF8Status check(unsigned char c);

private:
    enum class State
    {
        Initial,
        Continuation1,
        Continuation2,
        Continuation3,
        E0Continuation,
        EDContinuation,
        F0Continuation,
        F4Continuation,
    };

    State state{ State::Initial };
};

inline UTF8Status
UTF8Validator::check(char c)
{
    return check(static_cast<unsigned char>(c));
}
