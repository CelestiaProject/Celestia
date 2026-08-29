// bytes.h
//
// Copyright (C) 2023-present, the Celestia Development Team
// Copyright (C) 2001, Colin Walters <walters@verbum.org>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <bit>
#include <concepts>
#include <cstdint>

#include <celcompat/bit.h>

namespace impl
{

template <std::integral T>
constexpr T LE_TO_CPU(T val) noexcept
{
    if constexpr (std::endian::native == std::endian::big)
        return celestia::compat::byteswap(val);
    else
        return val;
}

template <std::integral T>
constexpr T BE_TO_CPU(T val) noexcept
{
    if constexpr (std::endian::native == std::endian::little)
        return celestia::compat::byteswap(val);
    else
        return val;
}

}

#define LE_TO_CPU_INT16(ret, val) (ret = impl::LE_TO_CPU(val))
#define LE_TO_CPU_INT32(ret, val) (ret = impl::LE_TO_CPU(val))
#define LE_TO_CPU_FLOAT(ret, val) (ret = std::bit_cast<float>(impl::LE_TO_CPU(std::bit_cast<std::uint32_t>(val))))
#define BE_TO_CPU_INT16(ret, val) (ret = impl::BE_TO_CPU(val))
#define BE_TO_CPU_INT32(ret, val) (ret = impl::BE_TO_CPU(val))
#define BE_TO_CPU_FLOAT(ret, val) (ret = std::bit_cast<float>(impl::BE_TO_CPU(std::bit_cast<std::uint32_t>(val))))
