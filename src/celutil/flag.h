// flag.h
//
// Copyright (C) 2023-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <type_traits>

#define ENUM_CLASS_BITWISE_OPS(E)                                       \
constexpr E& operator|=(E& f1, E f2)                                    \
{                                                                       \
    using type = std::underlying_type_t<E>;                             \
    f1 = static_cast<E>(static_cast<type>(f1) | static_cast<type>(f2)); \
    return f1;                                                          \
}                                                                       \
constexpr E& operator&=(E& f1, E f2)                                    \
{                                                                       \
    using type = std::underlying_type_t<E>;                             \
    f1 = static_cast<E>(static_cast<type>(f1) & static_cast<type>(f2)); \
    return f1;                                                          \
}                                                                       \
constexpr E& operator^=(E& f1, E f2)                                    \
{                                                                       \
    using type = std::underlying_type_t<E>;                             \
    f1 = static_cast<E>(static_cast<type>(f1) ^ static_cast<type>(f2)); \
    return f1;                                                          \
}                                                                       \
constexpr E operator|(E f1, E f2)                                       \
{                                                                       \
    f1 |= f2;                                                           \
    return f1;                                                          \
}                                                                       \
constexpr E operator&(E f1, E f2)                                       \
{                                                                       \
    f1 &= f2;                                                           \
    return f1;                                                          \
}                                                                       \
constexpr E operator^(E f1, E f2)                                       \
{                                                                       \
    f1 ^= f2;                                                           \
    return f1;                                                          \
}                                                                       \
constexpr E operator~(E f)                                              \
{                                                                       \
    return static_cast<E>(~static_cast<std::underlying_type_t<E>>(f));  \
}

namespace celestia::util
{

template<typename E, std::enable_if_t<std::is_enum_v<E>, int> = 0>
constexpr bool is_set(E f, E t)
{
    return (f & t) != static_cast<E>(0);
}

template<typename E, std::enable_if_t<std::is_enum_v<E>, int> = 0>
constexpr void set_or_unset(E& f, E t, bool set)
{
    if (set)
        f |= t;
    else
        f &= ~t;
}

}
