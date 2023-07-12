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

#if defined(__has_builtin)
#  if __has_builtin(__builtin_bit_cast)
#    define HAVE_BUILTIN_BIT_CAST 1
#  endif
#elif defined(__clang__)
#  if __clang_major__ >= 9
#    define HAVE_BUILTIN_BIT_CAST 1
#  endif
#  define HAVE_BUILTIN_MEMCPY 1
#elif defined(__GNUC__)
#  if __GNUC__ >= 11
#    define HAVE_BUILTIN_BIT_CAST 1
#  endif
#  define HAVE_BUILTIN_MEMCPY 1
#endif

#if !defined(HAVE_BUILTIN_BIT_CAST) && !defined(HAVE_BUILTIN_MEMCPY)
#include <cstring> // memcpy
#endif

/**
 * std::endian implementation
 */
namespace celestia::compat
{

#if __cpp_lib_endian

using std::endian;

#else

enum class endian
{
#if defined(_MSC_VER) && !defined(__clang__)
    little = 1234,
    big    = 4321,
    native = little
#else
    little = __ORDER_LITTLE_ENDIAN__,
    big    = __ORDER_BIG_ENDIAN__,
    native = __BYTE_ORDER__
#endif
};

#endif // __cpp_lib_endian

} // namespace celestia::compat

/**
 *  std::bit_cast implementation
 */
namespace celestia::compat
{

#if __cpp_lib_bit_cast

using std::bit_cast;

#else

template<typename To, typename From,
         std::enable_if_t<std::conjunction_v<std::bool_constant<sizeof(To) == sizeof(From)>,
                                             std::is_trivially_copyable<To>,
                                             std::is_trivially_copyable<From>>,
                          bool> = false>
[[nodiscard]]
#if defined(HAVE_BUILTIN_BIT_CAST) || defined(HAVE_BUILTIN_MEMCPY)
constexpr
#else
inline
#endif
To bit_cast(const From src) noexcept
{
#ifdef HAVE_BUILTIN_BIT_CAST
    return __builtin_bit_cast(To, src);
#else
    static_assert(std::is_trivially_constructible_v<To>,
        "This implementation additionally requires "
        "destination type to be trivially constructible");

    To dst;
#ifdef HAVE_BUILTIN_MEMCPY
    __builtin_memcpy(&dst, &src, sizeof(To));
#else
    std::memcpy(&dst, &src, sizeof(To));
#endif
    return dst;
#endif
}

#endif // __cpp_lib_bit_cast

} // namespace celestia::compat

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

constexpr std::uint16_t bswap_16 (std::uint16_t val) noexcept
{
#ifdef  __GNUC__
    return __builtin_bswap16(val);
#else
    return ((((val) >> 8) & 0xff) | (((val) & 0xff) << 8));
#endif
}

constexpr std::uint32_t bswap_32(std::uint32_t val) noexcept
{
#ifdef  __GNUC__
    return __builtin_bswap32(val);
#else
    return (((val) & 0xff000000) >> 24) | (((val) & 0x00ff0000) >>  8) |
            (((val) & 0x0000ff00) <<  8) | (((val) & 0x000000ff) << 24);
#endif
}

constexpr std::uint64_t bswap_64(std::uint64_t val) noexcept
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

template<typename T, std::enable_if_t<std::is_integral_v<T>, bool> = false>
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
