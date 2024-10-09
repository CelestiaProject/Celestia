// completion.cpp
//
// Copyright (C) 2024-present, the Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "completion.h"

namespace celestia::engine
{

std::string Completion::getName() const
{
    return name;
}

Selection Completion::getSelection() const
{
    return std::visit([this](auto& arg)
    {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, Selection>)
        {
            return arg;
        }
        else
        {
            static_assert(std::is_same_v<T, std::function<Selection()>>);
            return arg();
        }
    }, selection);
}

}
