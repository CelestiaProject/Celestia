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

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <celcompat/byteswap.h>

namespace celestia::util
{

static_assert(std::endian::native == std::endian::little ||
              std::endian::native == std::endian::big,
              "Mixed endian systems are not supported");

template<typename T, std::endian E>
    requires std::is_trivially_copyable_v<T>
class EndianBytes
{
private:
    using StorageType = std::array<std::byte, sizeof(T)>;
    using IntegralType = std::conditional_t<sizeof(T) == sizeof(unsigned short), unsigned short,
                         std::conditional_t<sizeof(T) == sizeof(unsigned int), unsigned int,
                         std::conditional_t<sizeof(T) == sizeof(unsigned long), unsigned long,
                         std::conditional_t<sizeof(T) == sizeof(unsigned long long), unsigned long long, void>>>>;

    static_assert(sizeof(StorageType) == sizeof(T));
    static_assert(alignof(StorageType) == 1);

    StorageType m_bytes;

    constexpr void set(T value) noexcept
    {
        if constexpr (sizeof(T) == 1 || E == std::endian::native)
        {
            m_bytes = std::bit_cast<StorageType>(value);
        }
        else if constexpr (std::is_integral_v<T>)
        {
            m_bytes = std::bit_cast<StorageType>(compat::byteswap(value));
        }
        else if constexpr (!std::is_same_v<IntegralType, void>)
        {
            m_bytes = std::bit_cast<StorageType>(compat::byteswap(std::bit_cast<IntegralType>(value)));
        }
        else
        {
            auto temp = std::bit_cast<StorageType>(value);
            for (std::size_t i = 0; i < sizeof(T); ++i)
                m_bytes[i] = temp[sizeof(T) - i - 1];
        }
    }

public:
    constexpr EndianBytes() noexcept = default;
    constexpr explicit(false) EndianBytes(T value) noexcept { set(value); }

    constexpr T get() const noexcept
    {
        if constexpr (sizeof(T) == 1 || E == std::endian::native)
        {
            return std::bit_cast<T>(m_bytes);
        }
        else if constexpr (std::is_integral_v<T>)
        {
            return compat::byteswap(std::bit_cast<T>(m_bytes));
        }
        else if constexpr (!std::is_same_v<IntegralType, void>)
        {
            return std::bit_cast<T>(compat::byteswap(std::bit_cast<IntegralType>(m_bytes)));
        }
        else
        {
            StorageType temp;
            for (std::size_t i = 0; i < sizeof(T); ++i)
                temp[i] = m_bytes[sizeof(T) - i - 1];
            return std::bit_cast<T>(temp);
        }
    }

    constexpr explicit(false) operator T() const noexcept { return get(); }

    constexpr EndianBytes& operator=(T value) noexcept { set(value); return *this; }
};

template<typename T>
    requires std::is_trivially_copyable_v<T>
using LE = EndianBytes<T, std::endian::little>;

template<typename T>
    requires std::is_trivially_copyable_v<T>
using BE = EndianBytes<T, std::endian::big>;

template<typename T>
    requires std::is_trivially_copyable_v<T>
using NE = EndianBytes<T, std::endian::native>;

} // end namespace celestia::util
