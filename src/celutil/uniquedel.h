// uniquedel.h
//
// Copyright (C) 2023-present, Celestia Development Team.
//
// Support for unique ptr with custom delete functions.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#pragma once

#include <memory>
#include <utility>

namespace celestia::util
{

namespace detail
{

template<auto F>
struct DeleterFunctor
{
    template<typename T>
    void operator()(T* ptr) const noexcept(noexcept(F(std::declval<T*>())))
    {
        if (ptr != nullptr)
            F(ptr);
    }
};

}

template<typename T, auto F>
using UniquePtrDel = std::unique_ptr<T, detail::DeleterFunctor<F>>;

}
