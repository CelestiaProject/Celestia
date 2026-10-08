// bit.h
//
// Copyright (C) 2023-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <bit>
#include <version>

#if __cpp_lib_byteswap >= 202110L
namespace celestia::compat { using std::byteswap; }
#else
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#ifdef _MSC_VER
#include <stdlib.h> // _byteswap builtins
#endif

namespace celestia::compat
{
template<std::integral T>
constexpr T
byteswap(T n)
{
    if constexpr (sizeof(T) == 1)
    {
        return n;
    }
    else if constexpr (sizeof(T) == 2)
    {
#ifdef __GNUC__
        return std::bit_cast<T>(__builtin_bswap16(std::bit_cast<std::uint16_t>(n)));
#else
#ifdef _MSC_VER
        // Unfortunately the MSVC builtins cannot be used in a constant expression
        if (!std::is_constant_evaluated())
        {
            return std::bit_cast<T>(_byteswap_ushort(std::bit_cast<unsigned short>(n)));
        }
        else
        {
#endif
            auto x = std::bit_cast<std::uint16_t>(n);
            x = (x << 8) | (x >> 8);
            return std::bit_cast<T>(x);
#ifdef _MSC_VER
        }
#endif
#endif
    }
    else if constexpr (sizeof(T) == 4)
    {
#ifdef __GNUC__
        return std::bit_cast<T>(__builtin_bswap32(std::bit_cast<std::uint32_t>(n)));
#else
#ifdef _MSC_VER
        if (!std::is_constant_evaluated())
        {
            return std::bit_cast<T>(_byteswap_ulong(std::bit_cast<unsigned long>(n)));
        }
        else
        {
#endif
            auto x = std::bit_cast<std::uint32_t>(n);
            x = ((x << 8) & UINT32_C(0xff00ff00)) | ((x >> 8) & UINT32_C(0x00ff00ff));
            x = (x << 16) | (x >> 16);
            return std::bit_cast<T>(x);
#ifdef _MSC_VER
        }
#endif
#endif
    }
    else if constexpr (sizeof(T) == 8)
    {
#ifdef __GNUC__
        return std::bit_cast<T>(__builtin_bswap64(std::bit_cast<std::uint64_t>(n)));
#else
#ifdef _MSC_VER
        if (!std::is_constant_evaluated())
        {
            return std::bit_cast<T>(_byteswap_uint64(std::bit_cast<unsigned __int64>(n)));
        }
        else
        {
#endif
            auto x = std::bit_cast<std::uint64_t>(n);
            x = ((x << 8) & UINT64_C(0xff00ff00ff00ff00)) | ((x >> 8) & UINT64_C(0x00ff00ff00ff00ff));
            x = ((x << 16) & UINT64_C(0xffff0000ffff0000)) | ((x >> 16) & UINT64_C(0x0000ffff0000ffff));
            x = (x << 32) | (x >> 32);
            return std::bit_cast<T>(x);
#ifdef _MSC_VER
        }
#endif
#endif
    }
    else
    {
        using U = std::make_unsigned_t<T>;
        auto x = std::bit_cast<U>(n);
        U y{0};
        constexpr auto mask = static_cast<U>(UINT8_MAX);
        for (std::size_t i = 0; i < sizeof(T); ++i)
            y |= ((x >> (i * 8)) & mask) << ((sizeof(T) - i - 1) * 8);
        return std::bit_cast<T>(y);
    }
}

} // end namespace celestia::compat
#endif
