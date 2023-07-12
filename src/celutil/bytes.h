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

#include <celcompat/bit.h>

namespace impl
{

template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = false>
constexpr T LE_TO_CPU(T val) noexcept
{
    if constexpr (celestia::compat::endian::native == celestia::compat::endian::big)
        return celestia::compat::byteswap(val);
    else
        return val;
}

template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = false>
constexpr T BE_TO_CPU(T val) noexcept
{
    if constexpr (celestia::compat::endian::native == celestia::compat::endian::little)
        return celestia::compat::byteswap(val);
    else
        return val;
}

}

#define LE_TO_CPU_INT16(ret, val) (ret = impl::LE_TO_CPU(val))
#define LE_TO_CPU_INT32(ret, val) (ret = impl::LE_TO_CPU(val))
#define LE_TO_CPU_FLOAT(ret, val) (ret = celestia::compat::bit_cast<float>(impl::LE_TO_CPU(celestia::compat::bit_cast<std::uint32_t>(val))))
#define BE_TO_CPU_INT16(ret, val) (ret = impl::BE_TO_CPU(val))
#define BE_TO_CPU_INT32(ret, val) (ret = impl::BE_TO_CPU(val))
#define BE_TO_CPU_FLOAT(ret, val) (ret = celestia::compat::bit_cast<float>(impl::BE_TO_CPU(celestia::compat::bit_cast<std::uint32_t>(val))))
