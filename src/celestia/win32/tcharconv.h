// tcharconv.h
//
// Copyright (C) 2023, Celestia Development Team
//
// Numeric parsing for Windows character sets
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <type_traits>

#include <celcompat/charconv.h>

#ifdef _UNICODE
#include <cstddef>
#include <iterator>
#include <fmt/format.h>
#endif

namespace celestia::win32
{

#ifdef _UNICODE

namespace detail
{

inline constexpr std::size_t tcharconv_buffer_size = 128;

template<std::size_t N>
std::size_t
fill_buffer(fmt::basic_memory_buffer<char, N>& buffer, const wchar_t* first, const wchar_t* last)
{
    buffer.reserve(static_cast<std::size_t>(last - first));
    auto it = std::back_inserter(buffer);
    auto ptr = first;
    while (ptr != last && *ptr >= L'0' && *ptr < L'\177')
    {
        *it = static_cast<char>(*ptr);
        ++ptr;
    }

    return static_cast<std::size_t>(ptr - first);
}

}

template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
compat::from_chars_result
from_tchars(const wchar_t* first, const wchar_t* last, T& value, int base = 10)
{
    fmt::basic_memory_buffer<char, detail::tcharconv_buffer_size> buffer;
    auto size = detail::fill_buffer(buffer, first, last);
    return compat::from_chars(buffer.data(), buffer.data() + size, value, base);
}

inline compat::from_chars_result
from_tchars(const wchar_t* first, const wchar_t* last, float& value,
            compat::chars_format fmt = compat::chars_format::general)
{
    fmt::basic_memory_buffer<char, detail::tcharconv_buffer_size> buffer;
    auto size = detail::fill_buffer(buffer, first, last);
    return compat::from_chars(buffer.data(), buffer.data() + size, value, fmt);
}

inline compat::from_chars_result
from_tchars(const wchar_t* first, const wchar_t* last, double& value,
            compat::chars_format fmt = compat::chars_format::general)
{
    fmt::basic_memory_buffer<char, detail::tcharconv_buffer_size> buffer;
    auto size = detail::fill_buffer(buffer, first, last);
    return compat::from_chars(buffer.data(), buffer.data() + size, value, fmt);
}

inline compat::from_chars_result
from_tchars(const wchar_t* first, const wchar_t* last, long double& value,
            compat::chars_format fmt = compat::chars_format::general)
{
    fmt::basic_memory_buffer<char, detail::tcharconv_buffer_size> buffer;
    auto size = detail::fill_buffer(buffer, first, last);
    return compat::from_chars(buffer.data(), buffer.data() + size, value, fmt);
}

#else

template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
compat::from_chars_result
from_tchars(const char* first, const char* last, T& value, int base = 10)
{
    return compat::from_chars(first, last, value, base);
}

inline compat::from_chars_result
from_tchars(const char* first, const char* last, float& value,
            compat::chars_format fmt = compat::chars_format::general)
{
    return compat::from_chars(first, last, value, fmt);
}

inline compat::from_chars_result
from_tchars(const char* first, const char* last, double& value,
            compat::chars_format fmt = compat::chars_format::general)
{
    return compat::from_chars(first, last, value, fmt);
}

inline compat::from_chars_result
from_tchars(const char* first, const char* last, long double& value,
            compat::chars_format fmt = compat::chars_format::general)
{
    return compat::from_chars(first, last, value, fmt);
}

#endif

} // end namespace celestia::win32
