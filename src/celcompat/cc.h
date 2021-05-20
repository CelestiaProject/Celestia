// cc.h
//
// Copyright (C) 2021-present, Celestia Development Team.
//
// from_chars() functions family as in C++17.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <limits>
#include <system_error>
#include <type_traits>

namespace std
{
struct from_chars_result
{
    const char* ptr;
    std::errc ec;
};

enum class chars_format
{
    scientific  = 1,
    fixed       = 2,
    hex         = 4,
    general     = fixed | scientific
};

template <typename T, typename std::enable_if<std::numeric_limits<T>::is_integer, T>::type = 0>
auto from_chars(const char* first, const char* last, T& value, int base = 10)
    -> std::from_chars_result;

auto from_chars(const char* first, const char* last, float &value,
                std::chars_format fmt = std::chars_format::general)
    -> std::from_chars_result;

auto from_chars(const char* first, const char* last, double &value,
                std::chars_format fmt = std::chars_format::general)
    -> std::from_chars_result;

auto from_chars(const char* first, const char* last, long double &value,
                std::chars_format fmt = std::chars_format::general)
    -> std::from_chars_result;

namespace
{
template <typename T>
auto int_from_chars(const char* first, const char* last, T& value, int base) noexcept
    -> std::from_chars_result
{
    auto p = first;
    bool has_minus = false;
    if (std::numeric_limits<T>::is_signed && *p == '-')
    {
        has_minus = true;
        p++;
    }

    using U = typename make_unsigned<T>::type;

    U risky_value;
    U max_digit;
    if (has_minus)
    {
        risky_value = (static_cast<U>(std::numeric_limits<T>::max()) + 1) / base;
        max_digit = (static_cast<U>(std::numeric_limits<T>::max()) + 1) % base;
    }
    else
    {
        risky_value = static_cast<U>(std::numeric_limits<T>::max() / base);
        max_digit = static_cast<U>(std::numeric_limits<T>::max() % base);
    }

    char last_n  = '0' + base;
    char last_ll = 'a' + base - 10;
    char last_ul = 'A' + base - 10;

    bool overflow = false;
    U result = 0;
    for (; p < last; p++)
    {
        U digit;
        if (base <= 10)
        {
            if (*p < '0' || *p >= last_n)
                break;
            digit = static_cast<U>(*p - '0');
        }
        else
        {
            if (*p >= '0' && *p <= '9')
                digit = static_cast<U>(*p - '0');
            else if (*p >= 'a' && *p < last_ll)
                digit = static_cast<U>(*p - 'a' + 10);
            else if (*p >= 'A' && *p < last_ul)
                digit = static_cast<U>(*p - 'A' + 10);
            else
                break;
        }

        if (result > risky_value || (result == risky_value && digit > max_digit))
            overflow = true;

        result = result * base + digit;
    }
    if (p == first || (has_minus && p == first + 1))
        return { first, std::errc::invalid_argument };

    if (overflow)
        return { p, std::errc::result_out_of_range };

    if (has_minus)
        value = -1 - static_cast<T>(result - 1);
    else
        value = static_cast<T>(result);
    return { p, std::errc() };
}

} // anon namespace

template <typename T, typename std::enable_if<std::numeric_limits<T>::is_integer, T>::type>
auto from_chars(const char* first, const char* last, T& value, int base)
    -> std::from_chars_result
{
    return int_from_chars(first, last, value, base);
}
}
