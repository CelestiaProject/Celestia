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

template<typename flag, typename = typename std::enable_if_t<std::is_enum_v<flag>>>
constexpr inline flag operator|(flag f1, flag f2)
{
    using type = std::underlying_type_t<flag>;
    return flag(type(f1) | type(f2));
}

template<typename flag, typename = typename std::enable_if_t<std::is_enum_v<flag>>>
constexpr inline flag operator&(flag f1, flag f2)
{
    using type = std::underlying_type_t<flag>;
    return flag(type(f1) & type(f2));
}

template<typename flag, typename = typename std::enable_if_t<std::is_enum_v<flag>>>
constexpr inline bool is_set(flag f, flag t)
{
    using type = std::underlying_type_t<flag>;
    return ((type(f) & type(t)) != 0);
}
