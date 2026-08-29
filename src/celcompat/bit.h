// bit.h
//
// Copyright (C) 2023-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#if __cplusplus >= 202002L && __has_include(<bit>)
#include <bit>
#endif

#include <cstdint>
#include <type_traits>

/**
 * std::byteswap implementation
 */
namespace celestia::compat
{

#if __cpp_lib_byteswap

using std::byteswap;

#else

namespace impl
{

constexpr std::uint16_t
bswap_16 (std::uint16_t val) noexcept
{
#ifdef  __GNUC__
    return __builtin_bswap16(val);
#else
    return ((((val) >> 8) & 0xff) | (((val) & 0xff) << 8));
#endif
}

constexpr std::uint32_t
bswap_32(std::uint32_t val) noexcept
{
#ifdef  __GNUC__
    return __builtin_bswap32(val);
#else
    return (((val) & 0xff000000) >> 24) | (((val) & 0x00ff0000) >>  8) |
            (((val) & 0x0000ff00) <<  8) | (((val) & 0x000000ff) << 24);
#endif
}

constexpr std::uint64_t
bswap_64(std::uint64_t val) noexcept
{
#ifdef  __GNUC__
    return __builtin_bswap64(val);
#else
    return (((val & 0xff00000000000000ull) >> 56) |
            ((val & 0x00ff000000000000ull) >> 40) |
            ((val & 0x0000ff0000000000ull) >> 24) |
            ((val & 0x000000ff00000000ull) >> 8)  |
            ((val & 0x00000000ff000000ull) << 8)  |
            ((val & 0x0000000000ff0000ull) << 24) |
            ((val & 0x000000000000ff00ull) << 40) |
            ((val & 0x00000000000000ffull) << 56));
#endif
}

} // namespace

template<std::integral T>
[[nodiscard]] constexpr T byteswap(T n) noexcept
{
    if constexpr (sizeof(T) == 1)
        return n;
    else if constexpr (sizeof(T) == 2)
        return static_cast<T>(impl::bswap_16(static_cast<std::uint16_t>(n)));
    else if constexpr (sizeof(T) == 4)
        return static_cast<T>(impl::bswap_32(static_cast<std::uint32_t>(n)));
    else if constexpr (sizeof(T) == 8)
        return static_cast<T>(impl::bswap_64(static_cast<std::uint64_t>(n)));
    else
        static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8,
                      "Unexpected integer size");
}

#endif // __cpp_lib_byteswap

} // namespace celestia::compat
