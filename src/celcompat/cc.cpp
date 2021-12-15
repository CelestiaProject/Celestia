// cc.cpp
//
// Copyright (C) 2021-present, Celestia Development Team.
//
// from_chars() functions family as in C++17.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <clocale>
#include <cstdlib>
#include <cstring>

#include <config.h>
#include "cc.h"

namespace celestia
{
namespace compat
{
namespace
{

constexpr std::size_t buffer_size = 512;
constexpr std::size_t inf_length = 3;
constexpr std::size_t infinity_length = 8;
constexpr std::size_t nan_length = 3;
constexpr std::size_t hex_prefix_length = 2;

enum class State
{
    Start,
    Fraction,
    ExponentStart,
    Exponent,
};

from_chars_result
write_buffer(const char* first, const char* last, chars_format fmt, char* buffer, bool& hex_prefix)
{
    hex_prefix = false;
    const char* ptr = first;

    char* buffer_end = buffer + buffer_size - 1; // allow space for zero byte terminator
    if (*ptr == '-')
    {
        *(buffer++) = *(ptr++);
        if (ptr == last) { return { first, std::errc::invalid_argument }; }
    }

    if (*ptr == 'i' || *ptr == 'I')
    {
        if (last - ptr < inf_length) { return { first, std::errc::invalid_argument }; }
        std::size_t length = std::min(static_cast<std::size_t>(infinity_length), static_cast<std::size_t>(last - ptr));
        std::memcpy(buffer, ptr, length);
        buffer += length;
    }
    else if (*ptr == 'n' || *ptr == 'N')
    {
        if (last - ptr < nan_length) { return { first, std::errc::invalid_argument }; }
        std::memcpy(buffer, ptr, nan_length);
        ptr += nan_length;
        buffer += nan_length;
        if (ptr < last && *ptr == '(')
        {
            *(buffer++) = *(ptr++);
            while (ptr < last)
            {
                if (buffer == buffer_end)
                {
                    // non-standard return code: no space to store entire bracket contents
                    return { first, std::errc::not_enough_memory };
                }
                if (*ptr == ')')
                {
                    *(buffer++) = *(ptr++);
                    break;
                }
                else if (std::isalnum(static_cast<unsigned char>(*ptr))
                    || *ptr == '_')
                {
                    *(buffer++) = *(ptr++);
                }
                else
                {
                    break;
                }
            }
        }
    }
    else
    {
        if (fmt == chars_format::hex)
        {
            hex_prefix = true;
            *(buffer++) = '0';
            *(buffer++) = 'x';
        }

        State state = State::Start;
        while (ptr < last)
        {
            if (*ptr == '.')
            {
                if (state != State::Start) { break; }
                state = State::Fraction;
            }
            else if (*ptr == '+' || *ptr == '-')
            {
                if (state != State::ExponentStart) { break; }
                state = State::Exponent;
            }
            else if (fmt == chars_format::hex)
            {
                if (*ptr == 'p' || *ptr == 'P')
                {
                    if (state != State::Start && state != State::Fraction) { break; }
                    state = State::ExponentStart;
                }
                else if (std::isxdigit(static_cast<unsigned char>(*ptr)))
                {
                    if (state == State::ExponentStart) { state = State::Exponent; }
                }
                else
                {
                    break;
                }
            }
            else if (*ptr == 'e' || *ptr == 'E')
            {
                if ((fmt & chars_format::scientific) != chars_format::scientific) { break; }
                state = State::ExponentStart;
            }
            else if (std::isdigit(static_cast<unsigned char>(*ptr)))
            {
                if (state == State::ExponentStart) { state = State::Exponent; }
            }
            else
            {
                break;
            }

            if (buffer == buffer_end) {
                // non-standard return code: no space to keep entire literal
                return { first, std::errc::not_enough_memory };
            }
            *(buffer++) = *(ptr++);
        }

        if (state == State::ExponentStart) { --buffer; state = State::Fraction; }

        if (fmt == chars_format::scientific && state != State::Exponent)
        {
            return { first, std::errc::invalid_argument };
        }
    }

    *buffer = '\0';
    return { ptr, std::errc{} };
}

inline void
parse_value(const char* start, char** end, float& value)
{
    value = std::strtof(start, end);
}

inline void
parse_value(const char* start, char** end, double& value)
{
    value = std::strtod(start, end);
}

inline void
parse_value(const char* start, char** end, long double& value)
{
    value = std::strtold(start, end);
}

template<typename T>
from_chars_result
from_chars_impl(const char* first, const char* last, T& value, chars_format fmt)
{
    char buffer[buffer_size];
    bool hex_prefix;
    from_chars_result result = write_buffer(first, last, fmt, buffer, hex_prefix);
    if (result.ec != std::errc{}) { return result; }

    const char* savedLocale = std::setlocale(LC_NUMERIC, nullptr);
    std::setlocale(LC_NUMERIC, "C");
    char* end;
    T parsed;
    parse_value(buffer, &end, parsed);
    if (errno == ERANGE)
    {
        errno = 0;
        result = { first + (end - buffer), std::errc::result_out_of_range };
    }
    else if (end == buffer)
    {
        result = { first, std::errc::invalid_argument };
    }
    else
    {
        value = parsed;
        if (hex_prefix) { end -= hex_prefix_length; }
        result = { first + (end - buffer), std::errc{} };
    }

    std::setlocale(LC_NUMERIC, savedLocale);
    return result;
}

} // end unnamed namespace

from_chars_result
from_chars(const char* first, const char* last, float &value, chars_format fmt)
{
    return from_chars_impl(first, last, value, fmt);
}

from_chars_result
from_chars(const char* first, const char* last, double &value, chars_format fmt)
{
    return from_chars_impl(first, last, value, fmt);
}

from_chars_result
from_chars(const char* first, const char* last, long double &value, chars_format fmt)
{
    return from_chars_impl(first, last, value, fmt);
}

} // end namespace compat
} // end namespace celestia
